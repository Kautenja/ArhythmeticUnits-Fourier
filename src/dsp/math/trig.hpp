// Basic mathematical functions.
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

#ifndef DSP_MATH_TRIG_HPP
#define DSP_MATH_TRIG_HPP

#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include "constants.hpp"

// TODO: fast versions of:
// - [ ] sin / cos / arcsin / arccos / etc.
//     - taylor series approximation!
//     - fast arctan option: https://www.musicdsp.org/en/latest/Effects/104-variable-hardness-clipping-function.html
//     inline double fastatan( double x ) {
//         return (x / (1.0 + 0.28 * (x * x)));
//     }
// TODO: __cospi, etc. for functions that multiply a number by \f$pi\f$
// TODO: https://stackoverflow.com/questions/507819/pi-and-accuracy-of-a-floating-point-number

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Return the output of the sine function.
///
/// @tparam the type of values to compute
/// @param x the input to the sine function in radians
/// @returns the output of the sine function for given input
/// @details
/// This implements the _Bhaskara_ approximation of the sine function:
///
/// \f$\sin(x) \approx \frac{16x(\pi - x)}{5\pi^2 - 4x(\pi - x)}\f$
///
template <typename T>
inline T sin_bhaskara(const T& x) {
    return (T(16) * x * (pi<T>() - x)) / (T(5) * squared(pi<T>()) - T(4) * x * (pi<T>() - x));
}

/// @brief Return the output of the sine function.
///
/// @tparam the type of values to compute
/// @param x the input to the sine function in radians \f$\in [0, 2\pi]\f$
/// @returns the output of the sine function for given input
/// @details
/// This implements a polynomial approximation of sine computed by Ridge
/// regression.
///
template <typename T>
inline T sin_poly3(const T& x) {
    return T(0.09093347) * cubed(x) + T(-0.85559925) * squared(x) + T(1.84028261) * x;
}

/// @brief Return the output of the cosine function.
///
/// @tparam the type of values to compute
/// @param x the input to the cosine function in radians
/// @returns the output of the cosine function for given input
/// @details
/// This implements the _Bhaskara_ approximation of the cosine function:
///
/// \f$\cos(y) \approx \frac{\pi^2 - 4y^2}{\pi^2 + y^2}\f$
///
template <typename T>
inline T cos_bhaskara(const T& x) {
    return (squared(pi<T>()) - 4 * squared(x)) / (squared(pi<T>()) + squared(x));
}

}  // namespace Math

#endif  // DSP_MATH_TRIG_HPP
