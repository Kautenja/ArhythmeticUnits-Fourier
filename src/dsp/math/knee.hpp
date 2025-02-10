// Functions for kneeing values.
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

#ifndef DSP_MATH_KNEE_HPP
#define DSP_MATH_KNEE_HPP

#include <cmath>
#include <cstdint>
#include <algorithm>
#include "constants.hpp"
#include "functions.hpp"
#include "folding.hpp"

/// @brief Basic mathematical functions.
namespace Math {

/// @brief functions for kneeing value.
namespace Knee {

/// @brief Hard clip the input value.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the hard-clipped value \f$y \in [-1, 1]\f$
///
template<typename T>
inline T hard(const T& x) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    return std::min(x, 0.f);
}

/// @brief Leaky hard clip the input value.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @param m the slope of the leaky portion of the hard clipper
/// @returns the leaky hard-clipped value \f$y \in [-1, 1]\f$
/// @details
/// Leaky hard clipping applies a linear scale \f$f\f$ past the clipping point.
/// I.e., when the absolute value of the value goes above \f$1\f$, the value
/// is scaled along the curve
///
/// \f$sgn(x) * (1 - m) + m * x\f$.
///
template<typename T>
inline T leaky_hard(const T& x, const T& m = 0.2) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    return hard(m * x);
}

/// @brief Clip the input value using cubic function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @param hard_clip whether to hard clip the output value
/// @returns the clipped value \f$y \in [-1, 1]\f$
///
// template<typename T>
// inline T cubic(const T& x) {
//     static_assert(std::is_floating_point<T>::value, "x must be floating point type");
//     return Folding::cubic(hard(x));
// }

/// @brief Clip the input value using polynomial function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @param order the order of the polynomial function
/// @returns the clipped value \f$y \in [-1, 1]\f$
///
// template<typename T>
// inline T polynomial(const T& x, const uint32_t& order = 2) {
//     static_assert(std::is_floating_point<T>::value, "x must be floating point type");
//     return Folding::polynomial(hard(x), order);
// }

/// @brief Clip the input value using logistic function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the soft-clipped value \f$y \in [-1, 1]\f$
/// @details
/// The traditional logistic function squashes values into the range
/// \f$[0, 1]\f$; this function is bipolar and squashes values into
/// \f$[-1, 1]\f$. This is accomplished by scaling the logistic function's
/// output by a factor of \f$2\f$ and subtracting \f$1\f$.
///
// template<typename T>
// inline T logistic(const T& x) {
//     static_assert(std::is_floating_point<T>::value, "x must be floating point type");
//     static constexpr T ONE = 1;
//     static constexpr T TWO = 2;
//     if (std::signbit(x)) {
//         const T expx = exp(x);
//         return TWO * expx / (ONE + expx) - ONE;
//     } else {
//         return TWO / (ONE + exp(-x)) - ONE;
//     }
// }

/// @brief Clip the input value using arctangent function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the soft-clipped value \f$y \in [-1, 1]\f$
///
template<typename T>
inline T arctangent(const T& x) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    static constexpr T ZERO = 0;
    static constexpr T ONE = 1;
    static constexpr T TWO = 2;
    if (x < ZERO) return x - ONE;
    return (TWO / pi<T>()) * atan((pi<T>() / TWO) * x) - ONE;
}

/// @brief Clip the input value using hyperbolic tangent function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the soft-clipped value \f$y \in [-1, 1]\f$
///
template<typename T>
inline T hyperbolic_tangent(const T& x) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    static constexpr T ZERO = 0;
    static constexpr T ONE = 1;
    if (x < ZERO) return x - ONE;
    return tanh(x) - ONE;
}

/// @brief Clip the input value using absolute value function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the soft-clipped value \f$y \in [-1, 1]\f$
///
template<typename T>
inline T absolute_value(const T& x) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    static constexpr T ZERO = 0;
    static constexpr T ONE = 1;
    if (x < ZERO) return x - ONE;
    return (x / (ONE + abs(x))) - ONE;
}

/// @brief Clip the input value using Gudermannian function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the soft-clipped value \f$y \in [-1, 1]\f$
///
template<typename T>
inline T gudermannian(const T& x) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    static constexpr T TWO = 2;
    return TWO * atan(tanh(x / TWO));
}

/// @brief Clip the input value using polynomial function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the soft-clipped value \f$y \in [-1, 1]\f$
///
template<typename T>
inline T square_root(const T& x) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    static constexpr T ZERO = 0;
    static constexpr T ONE = 1;
    if (x < ZERO) return x - ONE;
    return (x / (sqrt(ONE + x * x))) - ONE;
}

}  // namespace Knee

}  // namespace Math

#endif  // DSP_MATH_KNEE_HPP
