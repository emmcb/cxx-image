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

protected:
    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
        return seekoff(pos, std::ios_base::beg, which);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override {
        if (which != std::ios_base::in) {
            return -1;
        }

        if (dir == std::ios_base::beg) {
            setg(eback(), eback() + off, egptr());
        } else if (dir == std::ios_base::cur) {
            setg(eback(), gptr() + off, egptr());
        } else if (dir == std::ios_base::end) {
            setg(eback(), egptr() + off, egptr());
        }

        if (gptr() < eback() || gptr() > egptr()) {
            return -1; // invalid position
        }

        return gptr() - eback();
    }
};

/// Custom stream buffer for writing to a dynamic vector
class VectorStreamBuf : public std::streambuf {
public:
    const std::vector<char>& vec() const { return mData; }

    std::streamsize xsputn(const char* s, std::streamsize count) override {
        const std::size_t currentPutPos = pptr() - pbase();
        const std::size_t requiredSize = currentPutPos + count;

        if (requiredSize > mData.size()) {
            mData.resize(requiredSize);
        }

        char* dataPtr = mData.data();
        memcpy(dataPtr + currentPutPos, s, count);

        setp(dataPtr, dataPtr + mData.size());
        pbump(static_cast<int>(requiredSize));

        return count;
    }

protected:
    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
        return seekoff(pos, std::ios_base::beg, which);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override {
        if (which != std::ios_base::out) {
            return -1;
        }

        std::streamoff newOff = -1;

        if (dir == std::ios_base::beg) {
            newOff = off;
        } else if (dir == std::ios_base::cur) {
            newOff = (pptr() - pbase()) + off;
        } else if (dir == std::ios_base::end) {
            // end does not make sense as we don't know it yet
        }

        if (newOff < 0) {
            return -1; // invalid position
        }

        const auto newSize = static_cast<std::size_t>(newOff);
        if (newSize > mData.size()) {
            mData.resize(newSize);
        }

        char* dataPtr = mData.data();
        setp(dataPtr, dataPtr + mData.size());
        pbump(static_cast<int>(newOff));

        return newOff;
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
