// Constants for math functions.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_CONSTANTS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_CONSTANTS_HPP_

#include <complex>  // std::complex

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Return the value of \f$\pi\f$ with given type.
///
/// @tparam T the type (i.e., precision) of the returned value
/// @returns \f$\pi\f$ with given precision based on type
///
template<typename T>
inline constexpr T pi() {
    return 3.1415926535897932384626433832795028841971693993751058209749445923078164062;
}

/// @brief Return the value of \f$e\f$ with given type.
///
/// @tparam T the type (i.e., precision) of the returned value
/// @returns \f$e\f$ with given precision based on type
///
template<typename T>
inline constexpr T e() {
    return 2.7182818284590452353602874713526624977572470936999595749669676277240766303;
}

/// @brief Return the imaginary number \f$j\f$ (\f$i\f$).
///
/// @tparam T the type (i.e., precision) of the returned value
/// @returns a complex object representing the imaginary number \f$j\f$
///
template<typename T>
inline constexpr std::complex<T> j() { return std::complex<T>(0, 1); }

}  // namespace Math

#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_CONSTANTS_HPP_
