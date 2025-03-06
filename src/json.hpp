// JSON interaction functions.
//
// Copyright 2025 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef ARHYTHMETIC_UNITS_FOURIER_JSON_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_JSON_HPP_

#include <jansson.h>
#include <string>
#include <cstdint>
#include <functional>

/// @brief JSON interaction functions.
namespace JSON {

// ---------------------------------------------------------------------------
// MARK: Setters
// ---------------------------------------------------------------------------

#define JANSSON_SET(json_T, value) \
    json_object_set_new(rootJ, key.c_str(), json_T(value));

/// @brief Save the given JSON parameter.
///
/// @param rootJ the root JSON to save data into
/// @param key the string key to save into the JSON object
/// @param value the value to save at the given key
///
template<typename T>
inline void set(json_t* const rootJ, const std::string& key, const T& value);
template<> inline void set<unsigned>(json_t* const rootJ, const std::string& key, const unsigned& value) { JANSSON_SET(json_integer, value) }
template<> inline void set<int>(json_t* const rootJ, const std::string& key, const int& value) { JANSSON_SET(json_integer, value) }
template<> inline void set<float>(json_t* const rootJ, const std::string& key, const float& value) { JANSSON_SET(json_real, value) }
template<> inline void set<double>(json_t* const rootJ, const std::string& key, const double& value) { JANSSON_SET(json_real, value) }
template<> inline void set<bool>(json_t* const rootJ, const std::string& key, const bool& value) { JANSSON_SET(json_boolean, value) }
template<> inline void set<std::string>(json_t* const rootJ, const std::string& key, const std::string& value) { JANSSON_SET(json_string, value.c_str()) }

#undef JANSSON_SET

// ---------------------------------------------------------------------------
// MARK: Getters
// TODO: error handling for type using, i.e., `json_is_real`?
// ---------------------------------------------------------------------------

#define JANSSON_GET(json_T_value) \
    json_t* const value = json_object_get(rootJ, key.c_str());\
    if (value) callback(json_T_value(value));

/// @brief Load the given JSON parameter.
///
/// @param rootJ the root JSON to look for the key in
/// @param key the string key to lookup in the JSON object
/// @param callback a callback for if the key is found
///
template<typename T>
inline void get(json_t* const rootJ, const std::string& key, std::function<void(T)> callback);
template<> inline void get<unsigned>(json_t* const rootJ, const std::string& key, std::function<void(unsigned)> callback) { JANSSON_GET(json_integer_value) }
template<> inline void get<int>(json_t* const rootJ, const std::string& key, std::function<void(int)> callback) { JANSSON_GET(json_integer_value) }
template<> inline void get<float>(json_t* const rootJ, const std::string& key, std::function<void(float)> callback) { JANSSON_GET(json_real_value) }
template<> inline void get<double>(json_t* const rootJ, const std::string& key, std::function<void(double)> callback) { JANSSON_GET(json_real_value) }
template<> inline void get<bool>(json_t* const rootJ, const std::string& key, std::function<void(bool)> callback) { JANSSON_GET(json_boolean_value) }
template<> inline void get<std::string>(json_t* const rootJ, const std::string& key, std::function<void(std::string)> callback) { JANSSON_GET(json_string_value) }

#undef JANSSON_GET

// ---------------------------------------------------------------------------
// MARK: Array Setters
// ---------------------------------------------------------------------------

#define JANSSON_ARRAY_SET(json_T) \
    auto* array = json_array();\
    for (std::size_t i = 0; i < num_elements; i++)\
        json_array_append_new(array, json_T(value[i]));\
    json_object_set_new(rootJ, key.c_str(), array);

/// @brief Save the given JSON parameter.
///
/// @param rootJ the root JSON to save data into
/// @param key the string key to save into the JSON object
/// @param value the value to save at the given key
///
template<typename T>
inline void set(json_t* const rootJ, const std::string& key, std::size_t num_elements, const T* value);
template<> inline void set<unsigned>(json_t* const rootJ, const std::string& key, std::size_t num_elements, const unsigned* value) { JANSSON_ARRAY_SET(json_integer) }
template<> inline void set<int>(json_t* const rootJ, const std::string& key, std::size_t num_elements, const int* value) { JANSSON_ARRAY_SET(json_integer) }
template<> inline void set<float>(json_t* const rootJ, const std::string& key, std::size_t num_elements, const float* value) { JANSSON_ARRAY_SET(json_real) }
template<> inline void set<double>(json_t* const rootJ, const std::string& key, std::size_t num_elements, const double* value) { JANSSON_ARRAY_SET(json_real) }
template<> inline void set<bool>(json_t* const rootJ, const std::string& key, std::size_t num_elements, const bool* value) { JANSSON_ARRAY_SET(json_boolean) }
template<> inline void set<std::string>(json_t* const rootJ, const std::string& key, std::size_t num_elements, const std::string* value) {
    auto* array = json_array();
    for (std::size_t i = 0; i < num_elements; i++)
        json_array_append_new(array, json_string(value[i].c_str()));
    json_object_set_new(rootJ, key.c_str(), array);
}

#undef JANSSON_ARRAY_SET

// ---------------------------------------------------------------------------
// MARK: Array Getters
// TODO: error handling for type using, i.e., `json_is_real`?
// TODO: use enumeration error codes instead of esoteric integer codes
// ---------------------------------------------------------------------------

#define JANSSON_ARRAY_GET(json_T_value) \
    json_t* array = json_object_get(rootJ, key.c_str());\
    if (!array) return 1;\
    if (json_array_size(array) != num_elements) return 2;\
    for (std::size_t i = 0; i < num_elements; i++) {\
        json_t* value = json_array_get(array, i);\
        if (value) callback(i, json_T_value(value));\
    }\
    return 0;

/// @brief Load the given JSON array parameter.
///
/// @param rootJ the root JSON to look for the key in
/// @param key the string key to lookup in the JSON object
/// @param num_elements the number of expected elements in the array
/// @param callback a callback for if the key is found
/// @returns
///
template<typename T>
inline int get(json_t* const rootJ, const std::string& key, std::size_t num_elements, std::function<void(std::size_t, T)> callback);
template<> inline int get<unsigned>(json_t* const rootJ, const std::string& key, std::size_t num_elements, std::function<void(std::size_t, unsigned)> callback) { JANSSON_ARRAY_GET(json_integer_value) }
template<> inline int get<int>(json_t* const rootJ, const std::string& key, std::size_t num_elements, std::function<void(std::size_t, int)> callback) { JANSSON_ARRAY_GET(json_integer_value) }
template<> inline int get<float>(json_t* const rootJ, const std::string& key, std::size_t num_elements, std::function<void(std::size_t, float)> callback) { JANSSON_ARRAY_GET(json_real_value) }
template<> inline int get<double>(json_t* const rootJ, const std::string& key, std::size_t num_elements, std::function<void(std::size_t, double)> callback) { JANSSON_ARRAY_GET(json_real_value) }
template<> inline int get<bool>(json_t* const rootJ, const std::string& key, std::size_t num_elements, std::function<void(std::size_t, bool)> callback) { JANSSON_ARRAY_GET(json_boolean_value) }
template<> inline int get<std::string>(json_t* const rootJ, const std::string& key, std::size_t num_elements, std::function<void(std::size_t, std::string)> callback) { JANSSON_ARRAY_GET(json_string_value) }

#undef JANSSON_ARRAY_GET

}  // namespace JSON

#endif  // ARHYTHMETIC_UNITS_FOURIER_JSON_HPP_
