// Functions for calculating the Discrete Fourier Transform (DFT.)
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_DFT_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_DFT_HPP_

#include <cmath>          // exp
#include <cstdlib>        // size_t
#include <complex>        // complex
#include <vector>         // vector
#include "constants.hpp"  // pi, j
#include "window.hpp"     // window, coherent_gain

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Compute the Discrete Fourier Transform (DFT).
///
/// @param input Input sequence to compute the DFT of, \f$x[n]\f$
/// @param output Output sequence the store DFT coefficients in, \f$X[k]\f$
/// @param N Length of the input sequence / output DFT coefficients, \f$N\f$
/// @param window Windowing function to use when calculating coefficients.
/// @details
/// The Discrete Fourier Transform (DFT) for coefficient \f$k\f$ is calculated
/// as:
///
/// \f$X[k] = \sum_{n = 0}^{N - 1} x[n] e^{-j \frac{2 \pi k n}{N}}\f$
///
template<typename T>
void dft(
    const T* const input,
    std::complex<T>* const output,
    size_t N,
    const Window::Function& window = Window::Function::Boxcar
) {
    for (size_t k = 0; k < N; k++) {  // Iterate over coefficients.
        output[k] = 0;
        for (size_t n = 0; n < N; n++) {  // Iterate over samples.
            auto wn = Window::window<T>(window, n, N, false);
            auto phase = T(2) * pi<T>() * k * n / static_cast<T>(N);
            output[k] += wn * input[n] * exp(-j<T>() * phase);
        }
        output[k] /= Window::coherent_gain(window);  // Normalize window gain.
    }
}

/// @brief Compute the Discrete Fourier Transform (DFT).
///
/// @param input Input sequence to compute the DFT of, \f$x[n]\f$
/// @param window Windowing function to use when calculating coefficients.
/// @returns The DFT coefficients, \f$X[k]\f$
/// @details
/// The Discrete Fourier Transform (DFT) for coefficient \f$k\f$ is calculated
/// as:
///
/// \f$X[k] = \sum_{n = 0}^{N - 1} x[n] e^{-j \frac{2 \pi k n}{N}}\f$
///
template<typename T>
std::vector<std::complex<T>> dft(
    const std::vector<T>& input,
    const Window::Function& window = Window::Function::Boxcar
) {
    std::vector<std::complex<T>> output(input.size());
    dft<T>(input.data(), output.data(), input.size(), window);
    return output;
}

/// @brief Compute the Inverse Discrete Fourier Transform (IDFT).
///
/// @param input Input energies to compute the IDFT of, \f$X[k]\f$
/// @param output Output sequence to store time-domain samples in, \f$x[n]\f$
/// @param N Length of the input DFT coefficients / output sequence, \f$N\f$
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
void idft(const std::complex<T>* const input, T* const output, size_t N) {
    for (size_t k = 0; k < N; k++) {  // Iterate over coefficients.
        std::complex<T> accum = {0, 0};
        for (size_t n = 0; n < N; n++) {  // Iterate over samples.
            auto phase = T(2) * pi<T>() * k * n / static_cast<T>(N);
            accum += input[n] * exp(j<T>() * phase);
        }
        output[k] = accum.real() / static_cast<T>(N);
    }
}

/// @brief Compute the Inverse Discrete Fourier Transform (IDFT).
///
/// @param input Input energies to compute the IDFT of, \f$X[k]\f$
/// @returns output Sequence of time-domain samples, \f$x[n]\f$
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
std::vector<T> idft(const std::vector<std::complex<T>>& input) {
    std::vector<T> output(input.size());
    idft(input.data(), output.data(), input.size());
    return output;
}

}  // namespace Math

#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_DFT_HPP_
