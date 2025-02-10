// Basic mathematical functions.
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

#include <stdint.h>  // uint32_t, int32_t
#include <math.h>    // sqrtf, powf, fabsf, log10f, floorf
#include "constants.h"

#ifndef VERY_GOOD_DSP_MATH_FUNCTIONS_H_
#define VERY_GOOD_DSP_MATH_FUNCTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

// TODO: various quantized look-up tables
// TODO: fast versions of:
// - [ ] harmonic series (and other geometric series)
// - [ ] exp / log / ln / etc.
// - [ ] polynomials
// - [ ] fast inverse square root

/// @brief Return the modulo operation between two values.
/// @param a the input to the modulo function.
/// @param b the upper bound of the modulo function.
/// @returns \f$a \mod b\f$.
inline int32_t mod(int32_t a, int32_t b) {
    return (a % b + b) % b;
}

/// @brief Return the sign of the given value.
/// @param x the floating point number to get the sign of.
/// @returns \f$1\f$ if the number is positive, \f$-1\f$ otherwise
inline float sgnf(float x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

/// @brief Return the maximum of two values.
/// @param a The left-hand operand.
/// @param b The right-hand operand.
/// @returns The larger of the input values.
inline float maxf(float a, float b) {
    if (a > b) return a;
    return b;
}

/// @brief Return the minimum of two values.
/// @param a The left-hand operand.
/// @param b The right-hand operand.
/// @returns The smaller of the input values.
inline float minf(float a, float b) {
    if (a < b) return a;
    return b;
}

/// @brief Clamp a value to a given range.
/// @param v the value to clamp.
/// @param lo the lower bound to clamp the value to.
/// @param hi the upper bound to clamp the value to.
/// @returns the value clamped within \f$[lower, upper]\f$.
/// @details
/// If `lo` is greater than `hi`, the behavior is undefined.
inline float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/// @brief Return `base` raised to the power of 2.
/// @param x the base value to raise to the power of 2.
/// @returns the evaluation of \f$\texttt{base}^2\f$.
inline float squaref(float x) {
    return x * x;
}

/// @brief Return `base` raised to the power of 3.
/// @param x the base value to raise to the power of 3.
/// @returns the evaluation of \f$\texttt{base}^3\f$.
inline float cubef(float x) {
    return x * x * x;
}

/// @brief Return `base` raised to the power of `exponent`.
/// @param base the base value to raise to the given exponent.
/// @param exponent the signed integer exponent to raise the base to.
/// @returns the evaluation of \f$\texttt{base}^\texttt{exponent}\f$.
/// This implementation utilizes the "exponentiation by squaring" technique
/// to raise a floating point number to an integer exponent efficiently.
inline float powfi(float base, int32_t exponent) {
    // If the exponent is negative we can invert the base and negate the
    // exponent to use the same logic as if the exponent was positive.
    if (exponent < 0) {
        base = 1.f / base;
        exponent = -exponent;
    }
    // Use "exponentiation by squaring" to compute `pow` for integer exponents.
    float result = 1;
    for (;;) {
        if (exponent & 1)
            result *= base;
        exponent >>= 1;
        if (!exponent)
            break;
        base *= base;
    }
    return result;
}

/// @brief Return 2 raised to the power of `exponent` using a shift operation.
/// @param exponent the signed integer exponent to raise 2 to
/// @returns the evaluation of \f$2^\texttt{exponent}\f$
/// @details
/// This implementation provides a generic \f$2^\texttt{exponent}\f$ function
/// using the shift operator with 32-bit integers as a starting point. As a
/// result, it has a limited range of \f$[2^{-31}, 2^{31}]\f$; for larger
/// ranges, use `exp2f`. When \f$|exponent| > 31\f$, the behavior is undefined.
inline float exp2fi(int32_t exponent) {
    // cast a constant 1 to an unsigned 32-bit integer and perform the shift
    // operation to calculate 2**exponent
    if (exponent < 0)
        return 1.f / (((uint32_t) 1) << -exponent);
    else
        return ((uint32_t) 1) << exponent;
}

/// @brief Calculate n! using Stirling's Formula.
/// @param n the value to compute the factorial of
/// @returns the approximation of n! following Stirling's formula.
/// @details
/// Stirling's formula is defined as
/// \f$
/// n! \approx \sqrt{2\pi n}\big(\frac{n}{e}\big)^n
/// \f$
inline float factorialf(uint32_t n) {
    if (n == 0) return 1.f;
    return sqrtf(2.f * m_pi() * n) * powf(n / m_e(), n);
}

/// @brief Return the input value converted to decibels.
/// @param x the value to convert to decibel scale
/// @returns the decibel value \f$20 \log{10}(|x|)\f$
/// @details
/// This function assumes that signals are in the domain \f$\in [-1, 1]\f$.
inline float amplitude_to_dbf(float x) {
    return 20.f * log10f(fabsf(x));
}

/// @brief Return the input decibel value converted to amplitude.
/// @param x the decibel value to convert to linear scale
/// @returns the amplitude value \f$10^{\frac{x}{20}}\f$
/// @details
/// This function assumes that signals are in the domain \f$\in [-1, 1]\f$.
inline float db_to_amplitudef(float x) {
    return powf(10.f, x / 20.f);
}

/// @brief Quantize a single-precision float value to the given number of bits.
/// @param value the value \f$\in [-1, 1]\f$ to quantize
/// @param bits to number of bits to quantize the value to
/// @returns the value quantized to the given number of bits
/// @details
/// Numbers are quantized by:
/// 1. computing the maximal unsigned value of the number system as
///    \f$max \gets 2^{bits} - 1\f$;
/// 2. scaling the input value from \f$[-1, 1]\f$ to \f$[-max, max]\f$;
/// 3. truncating the scaled value to its integral component; and
/// 4. scaling the quantized integer from \f$[-max, max]\f$ back to floating
///    point in \f$[-1, 1]\f$
inline float quantizef(float value, int32_t bits) {
    // Determine the maximal value in the number system, i.e., (2^bits) - 1.
    float max = (((uint32_t) 1) << bits) - 1;
    // Convert from floating point in [-1.f, 1.f] to integer in [-max, max] and
    // use floor to discard the floating fractional component. Divide by max to
    // convert the integer in [-max, max] back to quantized float in [-1.f, 1.f]
    return sgnf(value) * floorf(fabsf(value) * max) / max;
}

/// @brief Return the interpolation between two points.
/// @param v0 the first point.
/// @param v1 the final point.
/// @param alpha the linear interpolation factor between the two points.
/// @returns the linear interpolation between `v0` and `v1` points.
/// @details
/// The interpolation is calculated as:
/// \f$(1-\alpha) x_i + \alpha x_f\f$
/// where:
/// \f$\alpha\f$ determines the linear progress from `v0` to `v1`.
inline float interpolatef(float v0, float v1, float alpha) {
    return (1.f - alpha) * v0 + alpha * v1;
}

/// @brief Return the index of the maximum value.
/// @param values the array to search.
/// @param size the number of elements in the array.
/// @returns the index of the maximum value in the array.
inline uint32_t argmaxf(const float* const values, uint32_t size) {
    float max_value = values[0];
    uint32_t argmax = 0;
    for (uint32_t i = 1; i < size; i++) {
        if (values[i] > max_value) {
            max_value = values[i];
            argmax = i;
        }
    }
    return argmax;
}

/// @brief Return the index of the minimum value.
/// @param values the array to search.
/// @param size the number of elements in the array.
/// @returns the index of the minimum value in the array.
inline uint32_t argminf(const float* const values, uint32_t size) {
    float min_value = values[0];
    uint32_t argmin = 0;
    for (uint32_t i = 1; i < size; i++) {
        if (values[i] < min_value) {
            min_value = values[i];
            argmin = i;
        }
    }
    return argmin;
}

// /// @brief Return the index of the complex value with the largest absolute value.
// /// @param arr the array to search for the max value of in
// /// @param size the number of elements in the given array
// /// @returns the index of the max value in the array
// ///
// template<typename T>
// std::size_t argmax(const std::complex<T>* const arr, const std::size_t& size) {
//     std::complex<T> max = std::numeric_limits<std::complex<T>>::min();
//     std::size_t argmax = 0;
//     for (std::size_t i = 0; i < size; i++) {
//         if (abs(arr[i]) > abs(max)) {
//             max = arr[i];
//             argmax = i;
//         }
//     }
//     return argmax;
// }

// /// @brief Iterate from the given starting point to stopping point.
// /// @param from the starting point
// /// @param to the stopping point
// /// @param callback the callback function to pass indexes to
// /// @details
// /// Iterates from `from` to `to` and passes each index `i` to `callback`.
// ///
// template<typename T, typename Function>
// void range_map(T from, T to, const Function& callback) {
//     if (to > from) for (T i = from; i < to;   i++) callback(i);
//     else           for (T i = to;   i < from; i++) callback(i);
// }

// template<typename T>
// std::vector<T> amplitude2decibels(const std::vector<std::complex<T>>& input) {
//     std::vector<T> output(input.size());
//     for (std::size_t i = 0; i < input.size(); i++)
//         output[i] = amplitude2decibels(input[i].real());
//     return output;
// }

// template<typename T>
// std::vector<T> amplitude2decibels(const std::vector<T>& input) {
//     std::vector<T> output(input.size());
//     for (std::size_t i = 0; i < input.size(); i++)
//         output[i] = amplitude2decibels(input[i]);
//     return output;
// }

// /// @brief Generate a fixed length sinusoidal waveform.
// /// @param frequency the frequency of the sine wave in \f$Hz\f$
// /// @param sample_rate the sampling rate in \f$Hz\f$
// /// @param num_sample the number of samples to generate
// /// @returns a vector of size num_samples with the sampled sinusoid
// ///
// template<typename T>
// std::vector<T> generate_sinusoid(T frequency, T sample_rate, std::size_t num_samples) {
//     // Create a vector to store the samples in
//     std::vector<T> wave(num_samples);
//     for (std::size_t i = 0; i < wave.size(); i++)  // Iterate over the samples
//         wave[i] = sin(T(2) * Math::pi<T>() * frequency * T(i) / T(sample_rate));
//     return wave;
// }

// /// @brief Find the peak level of the given waveform.
// /// @param wave the wave to find the peak level of
// /// @returns the energy of the loudest sample in decibels
// /// @details
// /// In most cases for detecting changes in energy, RMS should be used. Test
// /// cases designed using peak levels for filters will be sensitive to the
// /// Gibbs phenomenon an be difficult to calibrate / confirm analytically.
// ///
// template<typename T>
// T get_peak(const std::vector<T>& wave) {
//     // Initialize a register with the largest value found send to the minimal
//     // value.
//     T peak = Math::amplitude2decibels(std::numeric_limits<T>::epsilon());
//     for (unsigned i = 0; i < wave.size(); i++) {
//         // Calculate the instantaneous gain (i.e., peak) for this sample. This
//         // can potentially be negative infinity when `wave[i] == 0`, but this
//         // is OK because negative infinity will always be less than `peak`
//         // guaranteeing that the function returns at most the minimal peak that
//         // can be represented through the `amplitude2decibels` function.
//         const T gain = Math::amplitude2decibels(wave[i]);
//         // Update the peak gain if the instantaneous gain is higher.
//         if (gain > peak) peak = gain;
//     }
//     return peak;
// }

// /// @brief Calculate the RMS level of the given waveform.
// /// @param wave the wave to find the RMS level of
// /// @returns the RMS level of the waveform \f$[-120, 0]dB\f$
// ///
// template<typename T>
// T get_rms(const std::vector<T>& wave) {
//     // Calculate the minimum value for the mean square
//     static constexpr T MIN_MEAN_SQUARE = std::numeric_limits<T>::epsilon();
//     // Iterate over the samples in the waveform to calculate the mean square
//     T mean_square = 0;
//     for (std::size_t i = 0; i < wave.size(); i++)
//         mean_square += Math::squared(wave[i]);
//     mean_square /= wave.size();
//     // Map the mean square to decibels using properties of logarithm to avoid
//     // taking the square root of the mean square. Bound the mean square to the
//     // minimal mean square as defined by the type's epsilon value.
//     return T(10) * log10f(std::max(mean_square, MIN_MEAN_SQUARE));
// }

#ifdef __cplusplus
}
#endif

#endif  // VERY_GOOD_DSP_MATH_FUNCTIONS_H_
