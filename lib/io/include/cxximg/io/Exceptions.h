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

#pragma once

#include <exception>
#include <string>

namespace cxximg {

class IOError : public std::exception {
public:
    explicit IOError(const std::string &message) : mErrorMessage("IO error: " + message) {}
    IOError(const std::string &module, const std::string &message) : mErrorMessage(module + " error: " + message) {}

    const char *what() const noexcept override { return mErrorMessage.c_str(); }

private:
    std::string mErrorMessage;
};

} // namespace cxximg
