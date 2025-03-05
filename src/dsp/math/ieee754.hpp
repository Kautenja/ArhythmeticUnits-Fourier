// The IEEE 754 standard for floating point numbers.
//
// Copyright 2024 Arhythmetic Units
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

#ifndef DSP_MATH_IEEE754_HPP
#define DSP_MATH_IEEE754_HPP

#include <stdlib.h>
#include <complex>
#include <cstdint>
#include <limits>
#include <type_traits>

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Structures for manipulating IEEE754 floating point representations.
namespace IEEE754 {

/// @brief Return true if the floating point values are equal wrt. epsilon.
///
/// @tparam T the number type that determines the minimal difference, epsilon
/// @param a the first operand
/// @param b the second operand
/// @param epsilon the smallest difference to register a discrete difference
/// @returns true if the absolute difference between a and b is less than epsilon
///
template<typename T>
inline bool approx_equal(const T& a, const T& b, const T& epsilon) {
    static_assert(std::is_floating_point<T>(), "T should be a floating point type!");
    return abs(a - b) <= epsilon;
}

/// @brief Return true if the floating point values are equal wrt. epsilon.
///
/// @tparam T the number type that determines the minimal difference, epsilon
/// @param a the first operand
/// @param b the second operand
/// @returns true if the absolute difference between a and b is less than epsilon
///
template<typename T>
inline bool epsilon_equal(const T& a, const T& b) {
    return approx_equal(a, b, std::numeric_limits<T>::epsilon());
}

/// @brief Return true if the floating point values are equal wrt. epsilon.
///
/// @tparam T the number type that determines the minimal difference, epsilon
/// @param a the first operand
/// @param b the second operand
/// @param epsilon the smallest difference to register a discrete difference
/// @returns true if the absolute difference between a and b is less than epsilon
///
template<typename T>
inline bool approx_equal(const std::complex<T>& a, const std::complex<T>& b, const T& epsilon) {
    static_assert(std::is_floating_point<T>(), "T should be a floating point type!");
    return abs(a - b) <= epsilon;
}

/// @brief Return true if the floating point complex values are equal wrt. epsilon.
///
/// @tparam T the number type that determines the minimal difference, epsilon
/// @param a the first operand
/// @param b the second operand
/// @returns true if the absolute difference between a and b is less than epsilon
///
template<typename T>
inline bool epsilon_equal(const std::complex<T>& a, const std::complex<T>& b) {
    return approx_equal(a, b, std::numeric_limits<T>::epsilon());
}

// ---------------------------------------------------------------------------
// MARK: `float`
// ---------------------------------------------------------------------------

/// @brief the IEEE single-precision floating-point number data structure.
/// @details
/// the IEEE754 floating point standard defines 32-bit floating point numbers
/// with a 23-bit mantissa, 8-bit exponent, and a sign bit. Numbers are
/// interpreted as:
///
/// \f$(-1)^{\texttt{sign}} * 1.\texttt{mantissa} * 2^{\texttt{exponent} - 127}\f$
///
struct Float {
    /// value of the mantissa, i.e., \f$1.\texttt{mantissa}\f$
    uint32_t mantissa : 23;
    /// value of the exponent, i.e., \f$2^{\texttt{exponent} - 127}\f$
    uint32_t exponent : 8;
    /// sign of the number, i.e., \f$(-1)^{\texttt{sign}}\f$
    uint32_t sign : 1;
};

/// @brief a union of a Float structure and a C++ standard `float`.
union FloatUnion {
    /// the bitwise components of the floating point standard
    Float components;
    /// the C++ value in a float data structure
    float value;
};

// ---------------------------------------------------------------------------
// MARK: `double`
// ---------------------------------------------------------------------------

/// @brief the IEEE double-precision floating-point number data structure.
/// @details
/// the IEEE754 floating point standard defines 64-bit floating point numbers
/// with a 53-bit mantissa, 11-bit exponent, and a sign bit. Numbers are
/// interpreted as:
///
/// \f$(-1)^{\texttt{sign}} * 1.\texttt{mantissa} * 2^{\texttt{exponent} - 1023}\f$
///
struct Double {
    /// value of the mantissa, i.e., \f$1.\texttt{mantissa}\f$
    uint64_t mantissa : 52;
    /// value of the exponent, i.e., \f$2^{\texttt{exponent} - 1023}\f$
    uint64_t exponent : 11;
    /// sign of the number, i.e., \f$(-1)^{\texttt{sign}}\f$
    uint64_t sign : 1;
};

/// @brief a union of a Double structure and a C++ standard `double`.
union DoubleUnion {
    /// the bitwise components of the floating point standard
    Double components;
    /// the C++ value in a double data structure
    double value;
};

// ---------------------------------------------------------------------------
// MARK: `exp2`
// ---------------------------------------------------------------------------

/// @brief Return 2 raised to the power of `exponent` using IEEE754 exponent.
///
/// @param exponent the signed integer exponent to raise 2 to
/// @returns the evaluation of \f$2^\texttt{exponent}\f$
/// @details
/// This implementation provides a generic \f$2^\texttt{exponent}\f$ function
/// using the design characteristics of IEEE floating point numbers. this
/// provides a wider range of values, i.e., \f$[2^{-64}, 2^{127}]\f$ for single
/// precision and TODO for double precision.
///
inline float exp2(const int32_t& exponent) {
    if (exponent < -64 || exponent > 127)
        throw "exponent must be in the range [-64, 127]";
    float value = 2.f;
    reinterpret_cast<FloatUnion&>(value).components.exponent += (exponent - 1);
    return value;
}

}  // namespace IEEE754

}  // namespace Math

#endif  // DSP_MATH_IEEE754_HPP
