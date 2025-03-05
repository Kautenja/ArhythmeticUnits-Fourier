// Helper functions for testing.
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

#include "dsp/math/functions.hpp"

#ifndef ARHYTHMETIC_UNITS_TEST_FUNCTIONS_HPP_
#define ARHYTHMETIC_UNITS_TEST_FUNCTIONS_HPP_

/// @brief Generate a fixed length sinusoidal waveform.
///
/// @tparam the data type for representing samples
/// @param frequency the frequency of the sine wave in \f$Hz\f$
/// @param sample_rate the sampling rate in \f$Hz\f$
/// @param num_sample the number of samples to generate
/// @returns a vector of size num_samples with the sampled sinusoid
///
template<typename T>
std::vector<T> generate_sinusoid(T frequency, T sample_rate, size_t num_samples) {
    // Create a vector to store the samples in
    std::vector<T> wave(num_samples);
    for (size_t i = 0; i < wave.size(); i++)  // Iterate over the samples
        wave[i] = sin(T(2) * Math::pi<T>() * frequency * T(i) / T(sample_rate));
    return wave;
}

/// @brief Return the index of the value with the largest value.
///
/// @tparam T the type of data contained in the array
/// @param arr the array to search for the max value of in
/// @param size the number of elements in the given array
/// @returns the index of the max value in the array
///
template<typename T>
size_t argmax(const T* const arr, const size_t& size) {
    T max = std::numeric_limits<T>::min();
    size_t argmax = 0;
    for (size_t i = 0; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
            argmax = i;
        }
    }
    return argmax;
}

/// @brief Return the index of the complex value with the largest absolute value.
///
/// @tparam T the type of data contained in the array
/// @param arr the array to search for the max value of in
/// @param size the number of elements in the given array
/// @returns the index of the max value in the array
///
template<typename T>
size_t argmax(const std::complex<T>* const arr, const size_t& size) {
    std::complex<T> max = std::numeric_limits<std::complex<T>>::min();
    size_t argmax = 0;
    for (size_t i = 0; i < size; i++) {
        if (abs(arr[i]) > abs(max)) {
            max = arr[i];
            argmax = i;
        }
    }
    return argmax;
}

template<typename T>
std::vector<T> amplitude2decibels(const std::vector<std::complex<T>>& input) {
    std::vector<T> output(input.size());
    for (std::size_t i = 0; i < input.size(); i++)
        output[i] = Math::amplitude2decibels(input[i].real());
    return output;
}

template<typename T>
std::vector<T> amplitude2decibels(const std::vector<T>& input) {
    std::vector<T> output(input.size());
    for (std::size_t i = 0; i < input.size(); i++)
        output[i] = Math::amplitude2decibels(input[i]);
    return output;
}

#endif  // ARHYTHMETIC_UNITS_TEST_FUNCTIONS_HPP_
