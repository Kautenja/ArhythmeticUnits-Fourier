// Basic mathematical functions.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_HPP_

#include <cmath>      // signbit, log10, abs, pow, log2f, powf, roundf
#include <algorithm>  // max, min
#include <complex>    // complex
#include <limits>
#include <type_traits>

/// @brief Reusable Fourier DSP and utilities.
namespace Fourier {

/// @brief Finite complex magnitude without squaring the input scale.
/// @details Supports scalar and SIMD types through ADL. A normal denominator
/// keeps zero lanes safe under Rack's flush-to-zero policy. Subnormal inputs
/// still follow the caller's FPU mode; this does not promise gradual underflow.
template<typename T>
inline T complex_magnitude(const std::complex<T>& value) {
    using Scalar = typename std::conditional<std::is_floating_point<T>::value, T, float>::type;
    using std::abs;
    using std::fmax;
    using std::sqrt;
    const T a = abs(value.real()), b = abs(value.imag());
    const T scale = fmax(fmax(a, b), T(std::numeric_limits<Scalar>::min()));
    const T real = a / scale, imaginary = b / scale;
    return scale * sqrt(real * real + imaginary * imaginary);
}

/// @brief Clip the given value within the given limits.
///
/// @tparam T the type of values to clamp
/// @param x the value to clamp
/// @param lower the lower bound to clamp the value to
/// @param upper the upper bound to clamp the value to
/// @returns the value clamped within \f$[lower, upper]\f$
///
template <typename T>
inline T clip(const T& x, const T& lower, const T& upper) {
    return std::max(lower, std::min(x, upper));
}

/// @brief Return the sign of the given value.
///
/// @tparam T the type of the value to return the sign of
/// @param x the floating point number to get the sign of
/// @returns \f$-1\f$ if the sign bit is set, \f$1\f$ otherwise
/// (including signed zero: -0 returns -1 and +0 returns +1)
///
template<typename T>
inline T sgn(const T& x) {
    return std::signbit(x) ? -1 : 1;
}

/// @brief Return the modulo operation between two values.
///
/// @param a the input to the modulo function
/// @param b the upper bound of the modulo function
/// @returns \f$a \mod b\f$
///
template<typename T>
inline T mod(const T& a, const T& b) {
    return (a % b + b) % b;
}

/// @brief Return `base` raised to the power of 2.
///
/// @param x the base value to raise to the power of 2
/// @returns the evaluation of \f$\texttt{base}^2\f$
///
template<typename T>
inline T squared(const T& x) {
    return x * x;
}

/// @brief Return `base` raised to the power of 3.
///
/// @param x the base value to raise to the power of 3
/// @returns the evaluation of \f$\texttt{base}^3\f$
///
template<typename T>
inline T cubed(const T& x) {
    return x * x * x;
}

/// @brief Generic static function to multiply two complex numbers manually.
/// @param a The left-hand operand.
/// @param b The right-hand operand.
/// @returns The complex product of `a` and `b`.
/// @details
/// Uses component-wise arithmetic so SIMD types do not need the scalar
/// comparisons used by standard-library complex multiplication.
template<typename T>
inline std::complex<T> complex_multiply(const std::complex<T>& a, const std::complex<T>& b) {
    T realPart = a.real() * b.real() - a.imag() * b.imag();
    T imagPart = a.real() * b.imag() + a.imag() * b.real();
    return std::complex<T>(realPart, imagPart);
}

/// @brief Return the input value converted to decibels.
///
/// @tparam T the type of number to convert to decibels
/// @param x the value to convert to decibel scale
/// @returns the decibel value \f$20 \log_{10}(|x|)\f$
/// @details
/// Unit magnitude is 0 dB. Values are not clipped; zero maps to negative
/// infinity for IEEE-754 floating-point types.
///
template<typename T>
inline T amplitude2decibels(const T& x) {
    using std::abs;
    return T(20) * log10(abs(x));
}

/// @brief Return the input decibel value converted to amplitude.
///
/// @tparam T the type of number to convert to amplitude
/// @param x the decibel value to convert to linear scale
/// @returns the amplitude value \f$10^{\frac{x}{20}}\f$
/// @details
/// Returns a nonnegative amplitude ratio; 0 dB corresponds to unit amplitude.
///
template<typename T>
inline T decibels2amplitude(const T& x) {
    return pow(T(10), x / T(20));
}

}  // namespace Fourier

#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_HPP_
