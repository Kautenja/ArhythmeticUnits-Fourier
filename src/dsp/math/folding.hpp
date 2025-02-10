// Functions for folding values.
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

#ifndef DSP_MATH_FOLDING_HPP
#define DSP_MATH_FOLDING_HPP

#include <cmath>
#include <cstdint>
#include <algorithm>
#include "constants.hpp"
#include "functions.hpp"

/// @brief Basic mathematical functions.
namespace Math {

/// @brief functions for folding signals.
namespace Folding {

/// @brief Fold the input value using a square function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the folded value
///
template<typename T>
inline T square(const T& x) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    static constexpr T ONE = 1;
    static constexpr T TWO = 2;
    // apply the square function with 2 amplitude scale compensation to meet
    // the [-1, 1] output range specification
    return TWO * (x - (ONE / TWO) * sgn(x) * x * x);
}

/// @brief Fold the input value using a cubic function.
///
/// @tparam T the type of data to compute
/// @param x the input value \f$x \in [<-1, >1]\f$
/// @returns the folded value
///
template<typename T>
inline T cubic(const T& x) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    static constexpr T ONE = 1;
    static constexpr T TWO = 2;
    static constexpr T THR = 3;
    // apply the cubic function with 3/2 amplitude scale compensation to meet
    // the [-1, 1] output range specification
    return (THR / TWO) * (x - (ONE / THR) * x * x * x);
}

/// @brief Fold the input value using polynomial function.
///
/// @tparam T the type of data to compute
/// @param value the input value \f$\in [<-1, >1]\f$
/// @param order the order of the polynomial function
/// @returns the folded value
///
template<typename T>
inline T polynomial(const T& value, const uint32_t& order = 2) {
    static_assert(std::is_floating_point<T>::value, "x must be floating point type");
    static constexpr T ONE = 1;
    // Check the parity of the order to determine if the sign needs to be
    // calculated. Only even functions require sign correction.
    const T sign = (order & 0x1) ? ONE : sgn(value);
    // apply the polynomial function with o/(o-1) amplitude scale compensation
    // to meet the [-1, 1] output range specification
    #define o static_cast<T>(order)
    return (o / (o - ONE)) * (value - (ONE / o) * sign * pow(value, o));
    #undef o
}

}  // namespace Folding

}  // namespace Math

#endif  // DSP_MATH_FOLDING_HPP
