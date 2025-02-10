// Constants for math functions.
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

#ifndef DSP_MATH_CONSTANTS_HPP
#define DSP_MATH_CONSTANTS_HPP

#include <complex>
#include <limits>

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

/// @brief Return the amount of time for the Haas effect in milliseconds.
/// @details
/// The Haas effect determines the minimal time difference between left and
/// right channels where human perception is capable of separating the streams.
///
template<typename T>
inline constexpr T haas_time() { return 0.040; }

/// @brief Return the ratio of the audible spectrum to the Nyquist frequency.
///
/// @tparam T the type of data to work with
/// @param f_s the Nyquist rate (sample rate) \f$f_s\f$, measured in \f$Hz\f$
/// @returns the ratio of the audible band to the Nyquist band
/// @details
/// The output of audible spectrum is unit-less, it should be treated as a ratio
/// of audible units to sample-able units. The ratio is calculated as:
///
/// \f$\omega_{max} \ \pi \gets 20000 / (f_s / 2)\f$
///
template<typename T>
inline T audible_spectrum(const T& f_s) {
    return static_cast<T>(20000) / (f_s / static_cast<T>(2));
}

/// @brief Return the RMS of a signal at max peak energy.
///
/// @tparam T the type (i.e., precision) of the returned value
/// @returns the max RMS for a signal with amplitude domain \f$\in [-1, 1]\f$
///
template<typename T>
inline constexpr T maxRMS() { return T(20) * std::log10(sqrt(T(0.5))); }

}  // namespace Math

#endif  // DSP_MATH_CONSTANTS_HPP
