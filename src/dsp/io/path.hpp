// Functions for handling file IO.
// Copyright 2020 Christian Kauten
//
// Author: Christian Kauten (kautenja@auburn.edu)
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

#ifndef DSP_IO_PATH_HPP
#define DSP_IO_PATH_HPP

#include <string>

/// @brief Functions for handling file IO.
namespace IO {

/// @brief Functions for working with string filesystem paths.
namespace Path {

/// @brief Extract the file extension from the given path.
///
/// @param path the path to extract the extension from
/// @returns the extension of the file.
/// @details
/// If no extension is found, returns an empty string.
///
inline std::string getExtension(const std::string& path) {
    // Lookup the index of the last dot in the path.
    const auto index = path.find_last_of(".");
    // If no dot was found, return an empty string
    if (index == std::string::npos) return "";
    // Return a sub-string starting past the index of the dot
    return path.substr(index + 1);
}

/// @brief Extract the base name from the given path.
///
/// @param path the path to extract the base name from
/// @returns the base name of the file.
/// @details
/// If no base name is found, returns an empty string.
///
inline std::string getBasename(const std::string& path) {
    // Lookup the index of the last dot in the path.
    const auto index = path.find_last_of("/\\");
    // Return a sub-string starting past the index of the dot
    return path.substr(index + 1);
}

/// @brief Extract the directory name from the given path.
///
/// @param path the path to extract the directory name from
/// @returns the directory name of the file.
/// @details
/// If no directory name is found, returns an empty string.
///
inline std::string getDirname(const std::string& path) {
    // Lookup the index of the last dot in the path.
    const auto index = path.find_last_of("/\\");
    // If no delimiter was found, return an empty string
    if (index == std::string::npos) return "";
    // Return a sub-string starting past the index of the dot
    return path.substr(0, index);
}

}  // namespace Path

}  // namespace IO

#endif  // DSP_IO_PATH_HPP
