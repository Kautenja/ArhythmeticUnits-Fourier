// A circular buffer based on std::vector.
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

#ifndef DSP_MATH_CIRCULAR_BUFFER_HPP
#define DSP_MATH_CIRCULAR_BUFFER_HPP

#include <cstddef>    // size_t
#include <algorithm>  // std::fill
#include <vector>     // std::vector
#include "functions.hpp"

/// @brief Basic mathematical functions.
namespace Math {

/// @brief A circular buffer based on std::vector.
/// @tparam T the data type to store in the circular buffer
template<typename T>
struct CircularBuffer {
 protected:
    /// the buffer that houses the data.
    std::vector<T> buffer;

    /// the head of the buffer.
    size_t head = 0;

 public:
    /// @brief Initialize a new circular buffer.
    ///
    /// @param size the number of elements to store in the circular buffer
    ///
    explicit CircularBuffer(const size_t& size = 1) : buffer(size) { }

    /// @brief Return a pointer to the front of the underlying data structure.
    ///
    /// @returns a pointer to the array data structure used as a circular buffer
    ///
    inline T* data() const { return buffer.data(); }

    /// @brief Return a reference to the element at specified location `pos`.
    ///
    /// @param pos the index of the value to return in the buffer
    /// @details
    /// If `pos` is not within the range of the container, an exception of
    /// type `std::out_of_range` will throw.
    ///
    inline T& data_at(const size_t& pos) { return buffer.at(pos); }

    /// @brief Return a reference to the element at relative index `index`.
    ///
    /// @param index the circular index of the value to return in the buffer
    /// @details
    /// `index` refers to the sample relative to the head index, i.e., -1
    /// would be the last sample, +1 would be the oldest sample in the buffer.
    /// The index is circular,
    inline T& at(const int32_t& index) {
        return buffer[mod<int32_t>(index + head, buffer.size())];
    }

    /// @brief Set the size of the circular buffer to a new value.
    ///
    /// @param size_ the number of elements to store in the circular buffer
    ///
    inline void resize(size_t size_) { buffer.resize(size_); }

    /// @brief Return the size of the buffer.
    ///
    /// @returns the number of elements that the buffer can store
    ///
    inline size_t size() const { return buffer.size(); }

    /// @brief Clear the contents of the buffer.
    inline void clear() { std::fill(buffer.begin(), buffer.end(), 0); }

    /// @brief Insert a new value into the circular buffer.
    ///
    /// @param value the value to insert into the buffer
    /// @details
    /// Inserting values into the buffer iterates the head and overwrites
    /// any existing data at the head position.
    ///
    inline void insert(const T& value) {
        head = (head + 1) % buffer.size();
        buffer[head] = value;
    }
};

/// @brief A circular buffer based on std::vector with contiguous access.
/// @tparam T the data type to store in the circular buffer
template<typename T>
struct ContiguousCircularBuffer {
 protected:
    /// the circular buffer that houses two sequential copies of the data.
    std::vector<T> buffer;

    /// the head of the buffer
    size_t head = 0;

 public:
    /// @brief Initialize a new circular buffer.
    ///
    /// @param size the number of elements to store in the circular buffer
    ///
    explicit ContiguousCircularBuffer(const size_t& size = 1) : buffer(2 * size) { }

    /// @brief Return the underlying contiguous data buffer.
    const std::vector<T> get_buffer() {
        auto pointer = buffer.data() + head + 1;
        return std::vector<T>(pointer, pointer + size());
    }

    /// @brief Return a pointer to the front of the underlying data structure.
    ///
    /// @returns a pointer to the array data structure used as a circular buffer
    ///
    inline T* data() const { return buffer.data(); }

    /// @brief Return a reference to the element at specified location `pos`.
    ///
    /// @param pos the index of the value to return in the buffer
    /// @details
    /// If `pos` is not within the range of the container, an exception of
    /// type `std::out_of_range` will throw.
    ///
    inline T& data_at(const size_t& pos) { return buffer.at(pos); }

    /// @brief Return a reference to the element at relative index `index`.
    ///
    /// @param index the circular index of the value to return in the buffer
    /// @details
    /// `index` refers to the sample relative to the head index, i.e., -1
    /// would be the last sample, +1 would be the oldest sample in the buffer.
    /// The index is circular,
    inline T& at(const int32_t& index) {
        return buffer[mod<int32_t>(index + head, buffer.size())];
    }

    /// @brief Set the size of the circular buffer to a new value.
    ///
    /// @param size_ the number of elements to store in the circular buffer
    /// @details
    /// The internal double ring buffer will be resized to 2 * size.
    ///
    inline void resize(size_t size_) { buffer.resize(2 * size_); }

    /// @brief Return the number of items that can be stored in the buffer.
    ///
    /// @returns the number of elements that the buffer can store
    /// @details
    /// The size of the internal double ring buffer is 2 * size().
    ///
    inline size_t size() const { return buffer.size() / 2; }

    /// @brief Clear the contents of the buffer.
    inline void clear() { std::fill(buffer.begin(), buffer.end(), 0); }

    /// @brief Insert a new value into the circular buffer.
    ///
    /// @param value the value to insert into the buffer
    /// @details
    /// Inserting values into the buffer iterates the head and overwrites
    /// any existing data at the head position.
    ///
    inline void insert(const T& value) {
        head = (head + 1) % size();
        buffer[head] = value;
        buffer[head + size()] = value;
    }

    /// @brief Return a pointer to the front of the underlying data structure.
    ///
    /// @returns a pointer to the array data structure used as a circular buffer
    ///
    inline const T* contiguous() const { return &buffer[head + 1]; }
};

}  // namespace Math

#endif  // DSP_MATH_CIRCULAR_BUFFER_HPP
