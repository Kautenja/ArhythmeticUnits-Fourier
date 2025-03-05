// Constants defined by the eurorack standard.
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

#ifndef DSP_MATH_EURORACK_HPP
#define DSP_MATH_EURORACK_HPP

#include "functions.hpp"

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Constants defined by the Eurorack standard.
namespace Eurorack {

/// @brief Normalize a DC voltage into the range \f$[-1, 1]\f$.
///
/// @param voltage the DC voltage to normalize to \f$[-1, 1]\f$
/// @returns the input DC voltage scaled to the range \f$[-1, 1]\f$
/// @details
/// If the DC voltage exceed the saturation range of Eurorack
/// (\f$[-10, 10]V\f$), the function _will not_ clip the voltage.
///
template<typename T>
inline T fromDC(const T& voltage) { return voltage / T(10); }

/// @brief Return a DC voltage from the normalized range of \f$[-1, 1]\f$.
///
/// @param value the value in the normalized range \f$[-1, 1]\f$
/// @returns the output DC voltage scaled to the range \f$[-10, 10]\f$
/// @details
/// If the DC voltage exceed the saturation range of Eurorack
/// (\f$[-10, 10]V\f$), the function _will not_ clip the voltage.
///
template<typename T>
inline T toDC(const T& value) { return value * T(10); }

/// @brief Return a AC voltage normalized into the range \f$[-1, 1]\f$.
///
/// @param voltage the AC voltage to normalize to \f$[-1, 1]\f$
/// @returns the input AC voltage scaled to the range \f$[-1, 1]\f$
/// @details
/// If the AC voltage exceed the saturation range of Eurorack (\f$[-5, 5]V\f$),
/// the function _will not_ clip the voltage.
///
template<typename T>
inline T fromAC(const T& voltage) { return voltage / T(5); }

/// @brief Return an AC voltage from the normalized range of \f$[-1, 1]\f$.
///
/// @param value the value in the normalized range \f$[-1, 1]\f$
/// @returns the output AC voltage scaled to the range \f$[-10, 10]\f$
/// @details
/// If the AC voltage exceed the saturation range of Eurorack
/// (\f$[-5, 5]V\f$), the function _will not_ clip the voltage.
///
template<typename T>
inline T toAC(const T& value) { return value * T(5); }

}  // namespace Eurorack

}  // namespace Math

#endif  // DSP_MATH_EURORACK_HPP
