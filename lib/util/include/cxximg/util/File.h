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

#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>

namespace cxximg {

class FileNotFoundError : public std::exception {
public:
    explicit FileNotFoundError(const std::string &path) : mErrorMessage("File not found: " + path) {}

    const char *what() const noexcept override { return mErrorMessage.c_str(); }

private:
    std::string mErrorMessage;
};

struct FileDeleter final {
    void operator()(FILE *fp) const { fclose(fp); }
};

namespace file {

/// Returns the file extension converted in lower case.
inline std::string extension(const std::string &path) {
    std::string ext = path.substr(path.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext;
}

/// Reads the first bytes from a file into a buffer.
inline std::vector<uint8_t> readBinary(const std::string &path, int readSize) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw FileNotFoundError(path);
    }
    std::vector<uint8_t> header(readSize);
    file.rdbuf()->sgetn(reinterpret_cast<char *>(header.data()), readSize);
    return header;
}

/// Reads an entire file into a buffer.
inline std::vector<uint8_t> readBinary(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw FileNotFoundError(path);
    }
    return {std::istreambuf_iterator<char>(file), {}};
}

/// Reads an entire file into a string.
inline std::string readContent(const std::string &path) {
    std::ifstream file(path);
    if (!file) {
        throw FileNotFoundError(path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/// Gets file size.
inline int64_t fileSize(const std::string &path) {
    struct stat sb{};
    const int rc = stat(path.c_str(), &sb);
    if (rc != 0) {
        throw FileNotFoundError(path);
    }
    return sb.st_size;
}

} // namespace file

} // namespace cxximg
