// Functions for calculating the Discrete Fourier Transform (DFT.)
//
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

#ifndef DSP_MATH_DFT_HPP
#define DSP_MATH_DFT_HPP

#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <vector>
#include "constants.hpp"
#include "window.hpp"

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Compute the Discrete Fourier Transform using the exponential calculation.
///
/// @param input the input sequence to compute the DFT of, \f$x[n]\f$
/// @param output the output sequence the store DFT coefficients in, \f$X[k]\f$
/// @param length the length of the input sequence / output DFT coefficients, \f$N\f$
/// @param window a windowing function to use when calculating coefficients.
/// @details
/// The Discrete Fourier Transform (DFT) for coefficient \f$k\f$ is calculated
/// as:
///
/// \f$X[k] = \sum_{n = 0}^{N - 1} x[n] e^{-j \frac{2 \pi k n}{N}}\f$
///
template<typename T>
void dft_exp(
    const T* const input,
    std::complex<T>* const output,
    std::size_t length,
    const Math::Window::Function& window = Math::Window::Function::Boxcar
) {
    for (std::size_t k = 0; k < length; k++) {  // Iterate over coefficients.
        output[k] = 0;
        for (std::size_t n = 0; n < length; n++) {  // Iterate over samples.
            const std::complex<T> phase = T(2) * pi<T>() * k * n / static_cast<T>(length);
            output[k] += Math::Window::window<T>(window, n, length, false) * input[n] * exp(-j<T>() * phase);
        }
        output[k] /= Math::Window::coherent_gain(window);  // Normalize window gain.
    }
}

/// @brief Compute the Discrete Fourier Transform using the exponential calculation.
///
/// @param input the input vector to compute the DFT of, \f$x[n]\f$
/// @param window a windowing function to use when calculating coefficients.
/// @returns a vector containing the DFT coefficients, \f$X[k]\f$
/// @details
/// The Discrete Fourier Transform (DFT) for coefficient \f$k\f$ is calculated
/// as:
///
/// \f$X[k] = \sum_{n = 0}^{N - 1} x[n] e^{-j \frac{2 \pi k n}{N}}\f$
///
template<typename T>
std::vector<std::complex<T>> dft_exp(
    const std::vector<T>& input,
    const Math::Window::Function& window = Math::Window::Function::Boxcar
) {
    std::vector<std::complex<T>> output(input.size());
    dft_exp<T>(input.data(), output.data(), input.size(), window);
    return output;
}

/// @brief Compute the Discrete Fourier Transform using the exponential calculation.
///
/// @param input the input sequence to compute the DFT of, \f$x[n]\f$
/// @param output the output sequence the store DFT coefficients in, \f$X[k]\f$
/// @param length the length of the input sequence / output DFT coefficients, \f$N\f$
/// @param window a windowing function to use when calculating coefficients
/// @details
/// The Discrete Fourier Transform (DFT) for coefficient \f$k\f$ is calculated
/// as:
///
/// \f$X[k] = \sum_{n = 0}^{N - 1} x[n] e^{-j \frac{2 \pi k n}{N}}\f$
///
template<typename T>
void dft_trig(
    const T* const input,
    std::complex<T>* const output,
    std::size_t length,
    const Math::Window::Function& window = Math::Window::Function::Boxcar
) {
    for (std::size_t k = 0; k < length; k++) {  // Iterate over coefficients.
        output[k] = 0;
        for (std::size_t n = 0; n < length; n++) {  // Iterate over samples.
            const std::complex<T> phase = T(2) * pi<T>() * k * n / static_cast<T>(length);
            output[k] += Math::Window::window<T>(window, n, length, false) * input[n] * exp(-j<T>() * phase);
        }
        output[k] /= Math::Window::coherent_gain(window);  // Normalize window gain.
    }
}

/// @brief Compute the Discrete Fourier Transform using the trigonometric calculation.
///
/// @param input the input vector to compute the DFT of, \f$x[n]\f$
/// @param output a vector containing the DFT coefficients, \f$X[k]\f$
/// The Discrete Fourier Transform (DFT) for coefficient \f$k\f$ is calculated
/// as:
///
/// \f$X[k] = \sum_{n = 0}^{N - 1} x[n] \big( \cos(\frac{2 \pi k n}{N}) - j \sin(\frac{2 \pi k n}{N}) \big)\f$
///
template<typename T>
std::vector<std::complex<T>> dft_trig(const std::vector<T>& input) {
    std::vector<std::complex<T>> output(input.size());
    dft_trig<T>(input.data(), output.data(), input.size(), Math::Window::Function::Boxcar);
    return output;
}







/// @brief Compute the Discrete Fourier Transform using the exponential calculation.
///
/// @param input the input sequence to compute the IDFT of, \f$X[k]\f$
/// @param output the output sequence to store time-domain samples in, \f$x[n]\f$
/// @param length the length of the input DFT coefficients / output sequence, \f$N\f$
/// @details
/// The Inverse Discrete Fourier Transform (IDFT) for sample \f$n\f$ is
/// calculated as:
///
/// \f$x[n] = \frac{1}{N} \sum_{k = 0}^{N - 1} X[k] e^{j \frac{2 \pi k n}{N}}\f$
///
/// An assumption is made here that the target sequence in the time domain was
/// real, i.e., the real component is returned and the phase is discarded.
///
template<typename T>
void idft_exp(const std::complex<T>* const input, T* const output, std::size_t length) {
    for (std::size_t k = 0; k < length; k++) {  // Iterate over coefficients.
        std::complex<T> accum = {0, 0};
        for (std::size_t n = 0; n < length; n++) {  // Iterate over samples.
            const std::complex<T> phase = T(2) * pi<T>() * k * n / static_cast<T>(length);
            accum += input[n] * exp(j<T>() * phase);
        }
        output[k] = accum.real() / static_cast<T>(length);
    }
}

/// @brief Compute the Discrete Fourier Transform using the exponential calculation.
///
/// @param input the input vector to compute the DFT of, \f$x[n]\f$
/// @param output a vector containing the DFT coefficients, \f$X[k]\f$
/// @details
/// The Inverse Discrete Fourier Transform (IDFT) for sample \f$n\f$ is
/// calculated as:
///
/// \f$x[n] = \frac{1}{N} \sum_{k = 0}^{N - 1} X[k] e^{j \frac{2 \pi k n}{N}}\f$
///
/// An assumption is made here that the target sequence in the time domain was
/// real, i.e., the real component is returned and the phase is discarded.
///
template<typename T>
std::vector<T> idft_exp(const std::vector<std::complex<T>>& input) {
    std::vector<T> output(input.size());
    idft_exp(input.data(), output.data(), input.size());
    return output;
}

/// @brief Compute the Discrete Fourier Transform using the trigonometric calculation.
///
/// @param input the input sequence to compute the DFT of, \f$x[n]\f$
/// @param output the output sequence the store DFT coefficients in, \f$X[k]\f$
/// @param length the length of the input sequence / output DFT coefficients, \f$N\f$
/// @details
/// The Inverse Discrete Fourier Transform (IDFT) for sample \f$n\f$ is
/// calculated as:
///
/// \f$x[n] = \sum_{k = 0}^{N - 1} X[k] \big( \cos(\frac{2 \pi k n}{N}) + j \sin(\frac{2 \pi k n}{N}) \big)\f$
///
/// An assumption is made here that the target sequence in the time domain was
/// real, i.e., the real component is returned and the phase is discarded.
///
template<typename T>
void idft_trig(const std::complex<T>* const input, T* const output, std::size_t length) {
    for (std::size_t k = 0; k < length; k++) {  // Iterate over coefficients.
        std::complex<T> accum = {0, 0};
        for (std::size_t n = 0; n < length; n++) {  // Iterate over samples.
            const T phase = T(2) * pi<T>() * k * n / static_cast<T>(length);
            accum += input[n] * (std::cos(phase) + j<T>() * std::sin(phase));
        }
        output[k] = accum.real() / static_cast<T>(length);
    }
}

/// @brief Compute the Discrete Fourier Transform using the trigonometric calculation.
///
/// @param input the input vector to compute the DFT of, \f$x[n]\f$
/// @param output a vector containing the DFT coefficients, \f$X[k]\f$
/// The Inverse Discrete Fourier Transform (IDFT) for sample \f$n\f$ is
/// calculated as:
///
/// \f$x[n] = \sum_{k = 0}^{N - 1} X[k] \big( \cos(\frac{2 \pi k n}{N}) + j \sin(\frac{2 \pi k n}{N}) \big)\f$
///
/// An assumption is made here that the target sequence in the time domain was
/// real, i.e., the real component is returned and the phase is discarded.
///
template<typename T>
std::vector<T> idft_trig(const std::vector<std::complex<T>>& input) {
    std::vector<T> output(input.size());
    idft_trig(input.data(), output.data(), input.size());
    return output;
}

}  // namespace Math

#endif  // DSP_MATH_DFT_HPP
