#pragma once

#include "cxximg/io/ImageReader.h"

#include "cxximg/util/File.h"

namespace rawler {

class RawImage;

} // namespace rawler

namespace cxximg {

class RawlerReader final : public ImageReader {
public:
    static bool accept(const std::string &path) {
        // Accept files with common raw extensions
        const std::string ext = file::extension(path);
        return ext == "cr2" || ext == "cr3" || ext == "crw" || ext == "nef" || ext == "arw" || ext == "orf" ||
               ext == "rw2" || ext == "pef" || ext == "raf" || ext == "srw";
    }

    RawlerReader(const std::string &path, std::istream *stream, const Options &options);
    ~RawlerReader() override;

    void initialize() override;

    Image16u read16u() override;
    Imagef readf() override;

    std::optional<ExifMetadata> readExif() const override;
    void readMetadata(std::optional<ImageMetadata> &metadata) const override;

private:
    template <typename T>
    Image<T> read();

    rawler::RawImage *mRawImage = nullptr;
};

} // namespace cxximg
