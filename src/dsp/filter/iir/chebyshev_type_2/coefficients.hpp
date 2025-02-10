// Functions for calculating coefficients of a first-order IIR filter.
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

#ifndef DSP_FILTER_IIR_CHEBYSHEV_TYPE_2_COEFFICIENTS_HPP
#define DSP_FILTER_IIR_CHEBYSHEV_TYPE_2_COEFFICIENTS_HPP

#include <cmath>

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief Chebyshev Type 2 IIR filter designs (i.e., first-order).
namespace ChebyshevType2 {

/// @brief Calculate the parameters of a digital 1-pole low-pass filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 2 numerator coefficients of the filter, \f$b\f$
/// @param a the 1 denominator coefficient of the filter, \f$a\f$
/// @param O the cut-off frequency of the filter, \f$\Omega_c \in [0, \pi]\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]`.
///
template<typename T>
inline void lowpass(T b[2], T a[1], T O) {
    // pre-warp critical frequencies
    O = tan(O / 2);
    // cache the a_0 coefficient to normalize all coefficients
    const T a0 = O + 1;
    // calculate the coefficients
    b[0] = O       / a0;
    b[1] = O       / a0;
    a[0] = (O - 1) / a0;
}

/// @brief Calculate the parameters of a digital 1-pole high-pass filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 2 numerator coefficients of the filter, \f$b\f$
/// @param a the 1 denominator coefficient of the filter, \f$a\f$
/// @param O the cut-off frequency of the filter, \f$\Omega_c \in [0, \pi]\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]`.
///
template<typename T>
inline void highpass(T b[2], T a[1], T O) {
    // pre-warp critical frequencies
    O = tan(O / 2);
    // cache the a_0 coefficient to normalize all coefficients
    const T a0 = O + 1;
    // calculate the coefficients
    b[0] = 1       / a0;
    b[1] = -1      / a0;
    a[0] = (O - 1) / a0;
}

/// @brief Calculate the parameters of a digital 1-pole all-pass filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 2 numerator coefficients of the filter, \f$b\f$
/// @param a the 1 denominator coefficient of the filter, \f$a\f$
/// @param O the cut-off frequency of the filter, \f$\Omega_c \in [0, \pi]\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]`.
///
template<typename T>
inline void allpass(T b[2], T a[1], T O) {
    // pre-warp critical frequencies
    O = tan(O / 2);
    // cache the a_0 coefficient to normalize all coefficients
    const T a0 = O + 1;
    // calculate the coefficients
    b[0] = (1 - O)  / a0;
    b[1] = (-1 - O) / a0;
    a[0] = (O - 1)  / a0;
}

/// @brief Calculate the parameters of a digital 1-pole low-shelf filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 2 numerator coefficients of the filter, \f$b\f$
/// @param a the 1 denominator coefficient of the filter, \f$a\f$
/// @param Oz the cut-off frequency of the filter, \f$\Omega_z \in [0, \pi]\f$
/// @param O1 the transition frequency of the filter, \f$\Omega_1 \in [0, \pi]\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]`.
///
template<typename T>
inline void lowshelf(T b[2], T a[1], T Oz, T O1) {
    // pre-warp critical frequencies
    Oz = tan(Oz / 2);
    O1 = tan(O1 / 2);
    // cache the a_0 coefficient to normalize all coefficients
    const T a0 = O1 + 1;
    // calculate the coefficients
    b[0] = (Oz + 1) / a0;
    b[1] = (Oz - 1) / a0;
    a[0] = (O1 - 1) / a0;
}

/// @brief Calculate the parameters of a digital 1-pole high-shelf filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 2 numerator coefficients of the filter, \f$b\f$
/// @param a the 1 denominator coefficient of the filter, \f$a\f$
/// @param Oz the cut-off frequency of the filter, \f$\Omega_z \in [0, \pi]\f$
/// @param O1 the transition frequency of the filter, \f$\Omega_1 \in [0, \pi]\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]`.
///
template<typename T>
inline void highshelf(T b[2], T a[1], T Oz, T O1) {
    // pre-warp critical frequencies
    Oz = tan(Oz / 2);
    O1 = tan(O1 / 2);
    // cache the a_0 coefficient to normalize all coefficients
    const T a0 = O1 + 1;
    // calculate the coefficients
    b[0] = (Oz + 1) / a0;
    b[1] = (1 - Oz) / a0;
    a[0] = (1 - O1) / a0;
}

}  // namespace ChebyshevType2

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_CHEBYSHEV_TYPE_2_COEFFICIENTS_HPP
