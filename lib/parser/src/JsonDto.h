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

#include <rapidjson/document.h>

#include <variant>

namespace json_dto {

// this must be declared before json_dto is included

template <typename... Ts>
void read_json_value(std::variant<Ts...>& var, const rapidjson::Value& object);

template <typename... Ts>
void write_json_value(const std::variant<Ts...>& var,
                      rapidjson::Value& object,
                      rapidjson::MemoryPoolAllocator<>& allocator);

} // namespace json_dto

#include <json_dto/pub.hpp>

namespace json_dto {

namespace details {

template <std::size_t I, typename... Ts>
std::variant<Ts...> read_variant_value(const rapidjson::Value& object) {
    using V = std::variant<Ts...>;
    if constexpr (I < std::variant_size_v<V>) {
        try {
            std::variant_alternative_t<I, V> value;
            read_json_value(value, object);

            return value;
        } catch (ex_t&) {
            return read_variant_value<I + 1, Ts...>(object);
        }
    }
    throw ex_t("value can't be deserialized into std::variant");
}

} // namespace details

template <typename... Ts>
void read_json_value(std::variant<Ts...>& var, const rapidjson::Value& object) {
    var = details::read_variant_value<0, Ts...>(object);
}

template <typename... Ts>
void write_json_value(const std::variant<Ts...>& var,
                      rapidjson::Value& object,
                      rapidjson::MemoryPoolAllocator<>& allocator) {
    std::visit([&](auto&& arg) { write_json_value(arg, object, allocator); }, var);
}

} // namespace json_dto
