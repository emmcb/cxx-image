// Copyright 2023-2025 Emmanuel Chaboud
//
/// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cxximg/io/ImageIO.h"
#include "cxximg/parser/MetadataParser.h"
#include "cxximg/util/Version.h"

#include <cxxopts.hpp>
#include <loguru.hpp>

#include <iostream>
#include <string>

using namespace cxximg;

static constexpr const char* APP_NAME = "convert-image";

namespace cxximg {

inline void parse_value(const std::string& text, ImageWriter::TiffCompression& value) {
    const auto parsed = ImageWriter::parseTiffCompression(text);
    if (!parsed) {
        throw cxxopts::exceptions::incorrect_argument_type(text);
    }
    value = *parsed;
}

} // namespace cxximg

static cxxopts::ParseResult handleArguments(int argc, char* argv[]) {
    cxxopts::Options options(APP_NAME, "Image conversion tool");
    options.positional_help("input image");
    options.add_options("",
                        {{"i,input", "Input image path.", cxxopts::value<std::string>(), "PATH"},
                         {"o,output", "Output image path.", cxxopts::value<std::string>(), "PATH"},
                         {"m,metadata",
                          "Path to metadata file. (default: use sidecar if any)",
                          cxxopts::value<std::string>(),
                          "PATH"},
                         {"jpeg-quality", "JPEG output quality.", cxxopts::value<int>()->default_value("95")},
                         {"tiff-compression",
                          "TIFF output compression.",
                          cxxopts::value<ImageWriter::TiffCompression>()->default_value("deflate"),
                          "deflate|none"},
                         {"v,verbosity",
                          "Verbosity level.",
                          cxxopts::value<std::string>()->default_value("WARNING"),
                          "OFF|FATAL|ERROR|WARNING|INFO"},
                         {"help", "Print help and exits."},
                         {"version", "Print version number and exits."}});
    try {
        auto result = options.parse(argc, argv);

        if (result.count("help") > 0) {
            std::cout << options.help() << std::endl;
            exit(0);
        }

        if (result.count("version") > 0) {
            std::cout << APP_NAME << " version " << version::longVersionString() << std::endl;
            exit(0);
        }

        if (result.count("input") == 0) {
            throw cxxopts::exceptions::requested_option_not_present("input");
        }

        if (result.count("output") == 0) {
            throw cxxopts::exceptions::requested_option_not_present("output");
        }

        return result;
    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << e.what() << std::endl << std::endl << options.help() << std::endl;
        exit(1);
    }
}

static void run(const std::string& inputPath,
                const std::optional<std::string>& metadataPath,
                const std::string& outputPath,
                int jpegQuality,
                ImageWriter::TiffCompression tiffCompression) {
    // Input
    std::optional<ImageMetadata> metadata = parser::readMetadata(inputPath, metadataPath);

    std::unique_ptr<ImageReader> imageReader = io::makeReader(inputPath, ImageReader::Options(metadata));
    imageReader->readMetadata(metadata);

    // Output
    ImageWriter::Options writeOptions(metadata);
    writeOptions.jpegQuality = jpegQuality;
    writeOptions.tiffCompression = tiffCompression;

    std::unique_ptr<ImageWriter> imageWriter = io::makeWriter(outputPath, writeOptions);
    if (!imageWriter->acceptDescriptor(imageReader->layoutDescriptor())) {
        ABORT_S() << "Not supported output type: input image is not convertible to output";
    }

    // Read then write the image
    switch (imageReader->pixelRepresentation()) {
        case PixelRepresentation::UINT8: {
            Image8u input = imageReader->read8u();
            imageWriter->write(input);
        } break;

        case PixelRepresentation::UINT16: {
            Image16u input = imageReader->read16u();
            imageWriter->write(input);
        } break;

        case PixelRepresentation::FLOAT: {
            Imagef input = imageReader->readf();
            imageWriter->write(input);
        } break;

        default:
            ABORT_S() << "Unsupported pixel representation " << toString(imageReader->pixelRepresentation());
    }
}

int main(int argc, char* argv[]) {
    auto args = handleArguments(argc, argv);

    loguru::g_stderr_verbosity = loguru::Verbosity_WARNING;
    loguru::g_preamble_thread = false;
    loguru::g_preamble_file = false;
    loguru::init(argc, argv);

    LOG_S(INFO) << APP_NAME << " version " << version::longVersionString();

    const auto& inputPath = args["input"].as<std::string>();
    const auto& outputPath = args["output"].as<std::string>();
    const auto& jpegQuality = args["jpeg-quality"].as<int>();
    const auto& tiffCompression = args["tiff-compression"].as<ImageWriter::TiffCompression>();

    auto metadataPath = args.as_optional<std::string>("metadata");

    try {
        run(inputPath, metadataPath, outputPath, jpegQuality, tiffCompression);
    } catch (const std::exception& e) {
        ABORT_S() << e.what();
    }

    return 0;
}
