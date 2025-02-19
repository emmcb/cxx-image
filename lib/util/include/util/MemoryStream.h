#pragma once

#include <cstring>
#include <iostream>
#include <vector>

namespace cxximg {

/// Custom stream buffer for reading from a memory buffer
class MemoryStreamBuf : public std::streambuf {
public:
    MemoryStreamBuf(const char* buffer, std::size_t size) {
        setg(const_cast<char*>(buffer), const_cast<char*>(buffer), const_cast<char*>(buffer + size));
    }

    MemoryStreamBuf(char* buffer, std::size_t size) {
        setg(buffer, buffer, buffer + size);
        setp(buffer, buffer + size);
    }
};

/// Custom stream buffer for writing to a dynamic vector
class VectorStreamBuf : public std::streambuf {
public:
    const std::vector<char>& vec() const { return mData; }

    std::streamsize xsputn(const char* s, std::streamsize count) override {
        const std::size_t oldSize = mData.size();
        const std::size_t newSize = oldSize + count;

        mData.resize(newSize);
        char* dataPtr = mData.data();

        memcpy(dataPtr + oldSize, s, count);

        setp(dataPtr, dataPtr + newSize);
        pbump(static_cast<int>(newSize));

        return count;
    }

private:
    std::vector<char> mData;
};

/// Input stream using MemoryStreamBuf
class MemoryInputStream : public std::istream {
public:
    MemoryInputStream(const char* buffer, std::size_t size) : std::istream(nullptr), mBuffer(buffer, size) {
        this->init(&mBuffer);
    }

private:
    MemoryStreamBuf mBuffer;
};

/// Output stream using VectorStreamBuf
class VectorOutputStream : public std::ostream {
public:
    VectorOutputStream() : std::ostream(&mBuffer) {}

    const std::vector<char>& vec() const { return mBuffer.vec(); }

private:
    VectorStreamBuf mBuffer;
};

} // namespace cxximg
