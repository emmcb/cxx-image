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

#include "BmpIO.h"
#include "CfaIO.h"
#include "MipiRawIO.h"
#include "PlainIO.h"

#ifdef HAVE_DNG
#include "DngIO.h"
#endif

#ifdef HAVE_JPEG
#include "JpegIO.h"
#endif

#ifdef HAVE_PNG
#include "PngIO.h"
#endif

#ifdef HAVE_RAWLER
#include "RawlerIO.h"
#endif

#ifdef HAVE_TIFF
#include "TiffIO.h"
#endif

#include <fstream>

namespace cxximg {

namespace io {

std::unique_ptr<ImageReader> makeReader(const std::string &path, const ImageReader::Options &options) {
    return makeReader(path, nullptr, options);
}

std::unique_ptr<ImageReader> makeReader(std::istream *stream, const ImageReader::Options &options) {
    return makeReader("<data>", stream, options);
}

std::unique_ptr<ImageReader> makeReader(const std::string &path,
                                        std::istream *stream,
                                        const ImageReader::Options &options) {
    auto reader = [&]() -> std::unique_ptr<ImageReader> {
        // First: formats that need an extension to be identified
        if (MipiRaw10Reader::accept(path)) {
            return std::make_unique<MipiRaw10Reader>(path, stream, options);
        }

        if (MipiRaw12Reader::accept(path)) {
            return std::make_unique<MipiRaw12Reader>(path, stream, options);
        }

        if (PlainReader::accept(path)) {
            return std::make_unique<PlainReader>(path, stream, options);
        }

#ifdef HAVE_RAWLER
        if (RawlerReader::accept(path)) {
            return std::make_unique<RawlerReader>(path, stream, options);
        }
#endif

        // Read file signature
        uint8_t signature[8] = {0};
        bool signatureValid = false;

        const auto readSignature = [&signature, &signatureValid](std::istream *stream) {
            stream->read(reinterpret_cast<char *>(signature), sizeof(signature));
            signatureValid = !stream->fail();
            stream->seekg(0);
        };

        if (stream) {
            readSignature(stream);
        } else {
            std::ifstream ifs(path, std::ios::binary);
            if (ifs) {
                readSignature(&ifs);
            }
        }

#ifdef HAVE_DNG
        // Special case: DNG check requires tiff magic number along with dng extension
        if (DngReader::accept(path, signature, signatureValid)) {
            return std::make_unique<DngReader>(path, stream, options);
        }
#endif

        // Second: formats that have a magic number
        if (BmpReader::accept(path, signature, signatureValid)) {
            return std::make_unique<BmpReader>(path, stream, options);
        }

        if (CfaReader::accept(path, signature, signatureValid)) {
            return std::make_unique<CfaReader>(path, stream, options);
        }

#ifdef HAVE_JPEG
        if (JpegReader::accept(path, signature, signatureValid)) {
            return std::make_unique<JpegReader>(path, stream, options);
        }
#endif

#ifdef HAVE_PNG
        if (PngReader::accept(path, signature, signatureValid)) {
            return std::make_unique<PngReader>(path, stream, options);
        }
#endif

#ifdef HAVE_TIFF
        if (TiffReader::accept(path, signature, signatureValid)) {
            return std::make_unique<TiffReader>(path, stream, options);
        }
#endif

        // Third: formats that can be identified by format options
        if (options.fileInfo.fileFormat == FileFormat::PLAIN) {
            return std::make_unique<PlainReader>(path, stream, options);
        }

        if (options.fileInfo.fileFormat == FileFormat::RAW10) {
            return std::make_unique<MipiRaw10Reader>(path, stream, options);
        }

        if (options.fileInfo.fileFormat == FileFormat::RAW12) {
            return std::make_unique<MipiRaw12Reader>(path, stream, options);
        }

        // Fourth: plain formats handled with imageLayout and pixelType
        if (options.fileInfo.imageLayout || options.fileInfo.pixelType) {
            return std::make_unique<PlainReader>(path, stream, options);
        }

        throw IOError("No reader available for " + path);
    }();

    // Initialize reader
    reader->initialize();

    return reader;
}

std::unique_ptr<ImageWriter> makeWriter(const std::string &path, const ImageWriter::Options &options) {
    return makeWriter(path, nullptr, options);
}

std::unique_ptr<ImageWriter> makeWriter(std::ostream *stream, const ImageWriter::Options &options) {
    return makeWriter("<data>", stream, options);
}

std::unique_ptr<ImageWriter> makeWriter(const std::string &path,
                                        std::ostream *stream,
                                        const ImageWriter::Options &options) {
    if (BmpWriter::accept(path)) {
        return std::make_unique<BmpWriter>(path, stream, options);
    }

    if (CfaWriter::accept(path)) {
        return std::make_unique<CfaWriter>(path, stream, options);
    }

#ifdef HAVE_DNG
    if (DngWriter::accept(path)) {
        return std::make_unique<DngWriter>(path, stream, options);
    }
#endif

#ifdef HAVE_JPEG
    if (JpegWriter::accept(path)) {
        return std::make_unique<JpegWriter>(path, stream, options);
    }
#endif

    if (MipiRaw10Writer::accept(path)) {
        return std::make_unique<MipiRaw10Writer>(path, stream, options);
    }

    if (MipiRaw12Writer::accept(path)) {
        return std::make_unique<MipiRaw12Writer>(path, stream, options);
    }

#ifdef HAVE_PNG
    if (PngWriter::accept(path)) {
        return std::make_unique<PngWriter>(path, stream, options);
    }
#endif

#ifdef HAVE_TIFF
    if (TiffWriter::accept(path)) {
        return std::make_unique<TiffWriter>(path, stream, options);
    }
#endif

    if (options.fileFormat == FileFormat::PLAIN || PlainWriter::accept(path)) {
        return std::make_unique<PlainWriter>(path, stream, options);
    }

    if (options.fileFormat == FileFormat::RAW10) {
        return std::make_unique<MipiRaw10Writer>(path, stream, options);
    }

    if (options.fileFormat == FileFormat::RAW12) {
        return std::make_unique<MipiRaw12Writer>(path, stream, options);
    }

    throw IOError("No writer available for " + path);
}

} // namespace io

} // namespace cxximg
