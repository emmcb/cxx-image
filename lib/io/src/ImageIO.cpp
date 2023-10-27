// Copyright 2023 Emmanuel Chaboud
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

#include "io/ImageIO.h"

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

#ifdef HAVE_TIFF
#include "TiffIO.h"
#endif

namespace cxximg {

namespace io {

std::unique_ptr<ImageReader> makeReader(const std::string &path, const ImageReader::Options &options) {
    // First: formats that need an extension to be identified
    if (MipiRaw10Reader::accept(path)) {
        return std::make_unique<MipiRaw10Reader>(path, options);
    }

    if (MipiRaw12Reader::accept(path)) {
        return std::make_unique<MipiRaw12Reader>(path, options);
    }

    if (PlainReader::accept(path)) {
        return std::make_unique<PlainReader>(path, options);
    }

    // Second: formats that have a magic number
    if (BmpReader::accept(path)) {
        return std::make_unique<BmpReader>(path, options);
    }

#ifdef HAVE_DNG
    if (DngReader::accept(path)) {
        return std::make_unique<DngReader>(path, options);
    }
#endif

#ifdef HAVE_JPEG
    if (JpegReader::accept(path)) {
        return std::make_unique<JpegReader>(path, options);
    }
#endif

#ifdef HAVE_PNG
    if (PngReader::accept(path)) {
        return std::make_unique<PngReader>(path, options);
    }
#endif

#ifdef HAVE_TIFF
    if (TiffReader::accept(path)) {
        return std::make_unique<TiffReader>(path, options);
    }
#endif

    // Third: formats that can be identified by format options
    if (options.fileInfo.fileFormat == FileFormat::PLAIN) {
        return std::make_unique<PlainReader>(path, options);
    }

    if (options.fileInfo.fileFormat == FileFormat::RAW10) {
        return std::make_unique<MipiRaw10Reader>(path, options);
    }

    if (options.fileInfo.fileFormat == FileFormat::RAW12) {
        return std::make_unique<MipiRaw12Reader>(path, options);
    }

    // Fourth: plain formats handled with imageLayout and pixelType
    if (options.fileInfo.imageLayout || options.fileInfo.pixelType) {
        return std::make_unique<PlainReader>(path, options);
    }

    throw IOError("No reader available for " + path);
}

std::unique_ptr<ImageWriter> makeWriter(const std::string &path, const ImageWriter::Options &options) {
    if (CfaWriter::accept(path)) {
        return std::make_unique<CfaWriter>(path, options);
    }

    if (BmpWriter::accept(path)) {
        return std::make_unique<BmpWriter>(path, options);
    }

#ifdef HAVE_DNG
    if (DngWriter::accept(path)) {
        return std::make_unique<DngWriter>(path, options);
    }
#endif

#ifdef HAVE_JPEG
    if (JpegWriter::accept(path)) {
        return std::make_unique<JpegWriter>(path, options);
    }
#endif

    if (MipiRaw10Writer::accept(path)) {
        return std::make_unique<MipiRaw10Writer>(path, options);
    }

    if (MipiRaw12Writer::accept(path)) {
        return std::make_unique<MipiRaw12Writer>(path, options);
    }

#ifdef HAVE_PNG
    if (PngWriter::accept(path)) {
        return std::make_unique<PngWriter>(path, options);
    }
#endif

#ifdef HAVE_TIFF
    if (TiffWriter::accept(path)) {
        return std::make_unique<TiffWriter>(path, options);
    }
#endif

    if (options.fileFormat == FileFormat::PLAIN || PlainWriter::accept(path)) {
        return std::make_unique<PlainWriter>(path, options);
    }

    if (options.fileFormat == FileFormat::RAW10) {
        return std::make_unique<MipiRaw10Writer>(path, options);
    }

    if (options.fileFormat == FileFormat::RAW12) {
        return std::make_unique<MipiRaw12Writer>(path, options);
    }

    throw IOError("No writer available for " + path);
}

} // namespace io

} // namespace cxximg
