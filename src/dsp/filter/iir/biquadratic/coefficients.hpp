// Functions for calculating coefficients of a Biquad IIR filter.
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

#ifndef DSP_FILTER_IIR_BIQUADRATIC_COEFFICIENTS_HPP
#define DSP_FILTER_IIR_BIQUADRATIC_COEFFICIENTS_HPP

#include <cmath>

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief Biquadratic IIR filter designs (i.e., second-order).
namespace Biquadratic {

/// @brief Calculate the parameters of a digital Sallen-Key low-pass filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 3 numerator coefficients of the filter, \f$b\f$
/// @param a the 2 denominator coefficients of the filter, \f$a\f$
/// @param O the cut-off frequency of the filter, \f$\Omega_c \in [0, \pi]\f$
/// @param Q the quality-factor of the filter, \f$Q\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]` and \f$a_2 =\f$ `a[1]`.
///
template<typename T>
inline void sallen_key_lowpass(T b[3], T a[2], T O, T Q) {
    // pre-warp critical frequencies
    O = tan(O / 2);
    // cache O**2 * Q
    const T O2Q = O * O * Q;
    // cache the a_0 coefficient to normalize all coefficients
    const T a0 = O2Q + O + Q;
    // calculate the coefficients
    b[0] = O2Q             / a0;
    b[1] = (2 * O2Q)       / a0;
    b[2] = O2Q             / a0;
    a[0] = (2 * (O2Q - Q)) / a0;
    a[1] = (O2Q - O + Q)   / a0;
}

/// @brief Calculate the parameters of a digital Sallen-Key high-pass filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 3 numerator coefficients of the filter, \f$b\f$
/// @param a the 2 denominator coefficients of the filter, \f$a\f$
/// @param O the cut-off frequency of the filter, \f$\Omega_c \in [0, \pi]\f$
/// @param Q the quality-factor of the filter, \f$Q\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]` and \f$a_2 =\f$ `a[1]`.
///
template<typename T>
inline void sallen_key_highpass(T b[3], T a[2], T O, T Q) {
    // pre-warp critical frequencies
    O = tan(O / 2);
    // cache O**2 * Q
    const T O2Q = O * O * Q;
    // cache the a_0 coefficient to normalize all coefficients
    const T a0 = O2Q + O + Q;
    // calculate the coefficients
    b[0] = Q               / a0;
    b[1] = (-2 * Q)        / a0;
    b[2] = Q               / a0;
    a[0] = (2 * (O2Q - Q)) / a0;
    a[1] = (O2Q - O + Q)   / a0;
}

/// @brief Calculate the parameters of a digital Sallen-Key band-pass filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 3 numerator coefficients of the filter, \f$b\f$
/// @param a the 2 denominator coefficients of the filter, \f$a\f$
/// @param O the cut-off frequency of the filter, \f$\Omega_c \in [0, \pi]\f$
/// @param Q the quality-factor of the filter, \f$Q\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]` and \f$a_2 =\f$ `a[1]`.
///
template<typename T>
inline void sallen_key_bandpass(T b[3], T a[2], T O, T Q) {
    // pre-warp critical frequencies
    O = tan(O / 2);
    // cache O**2 * Q
    const T O2Q = O * O * Q;
    // cache the a_0 coefficient to normalize all coefficients
    const T a0 = O2Q + O + Q;
    // calculate the coefficients
    b[0] = Q * O           / a0;
    b[1] = 0               / a0;
    b[2] = -Q * O          / a0;
    a[0] = (2 * (O2Q - Q)) / a0;
    a[1] = (O2Q - O + Q)   / a0;
}

/// @brief Calculate the parameters of a digital Bainter band-stop filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 3 numerator coefficients of the filter, \f$b\f$
/// @param a the 2 denominator coefficients of the filter, \f$a\f$
/// @param Oz the central frequency of the zeros, \f$\Omega_z \in [0, \pi]\f$
/// @param Op the central frequency of the poles, \f$\Omega_p \in [0, \pi]\f$
/// @param Q the quality-factor of the filter, \f$Q\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]` and \f$a_2 =\f$ `a[1]`.
///
template<typename T>
inline void bainter_bandstop(T b[3], T a[2], T Oz, T Op, T Q) {
    // pre-warp critical frequencies
    Oz = tan(Oz / 2);
    Op = tan(Op / 2);
    // cache Op^2 and Oz^2
    const T Op2 = Op * Op;
    const T Oz2 = Oz * Oz;
    // cache the a0 coefficient to normalize all coefficients
    const T a0 = Q * Op2 + Q + Op;
    // calculate the coefficients
    b[0] = (Q * (Oz2 + 1))       / a0;
    b[1] = (Q * (2 * Oz2 - 2))   / a0;
    b[2] = (Q * (Oz2 + 1))       / a0;
    a[0] = (2 * Q * Op2 - 2 * Q) / a0;
    a[1] = (Q * Op2 + Q - Op)    / a0;
}

/// @brief Calculate the parameters of a digital Bainter filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 3 numerator coefficients of the filter, \f$b\f$
/// @param a the 2 denominator coefficients of the filter, \f$a\f$
/// @param Oz the central frequency of the zeros, \f$\Omega_z \in [0, \pi]\f$
/// @param Op the central frequency of the poles, \f$\Omega_p \in [0, \pi]\f$
/// @param Qz the quality-factor of the zeros, \f$Q_z\f$
/// @param Qp the quality-factor of the zeros, \f$Q_p\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]` and \f$a_2 =\f$ `a[1]`.
///
template<typename T>
inline void bainter(T b[3], T a[2], T Oz, T Op, T Qz, T Qp) {
    // pre-warp critical frequencies
    Oz = tan(Oz / 2);
    Op = tan(Op / 2);
    // cache Op^2 and Oz^2
    const T Op2 = Op * Op;
    const T Oz2 = Oz * Oz;
    // cache the a0 coefficient to normalize all coefficients
    const T a0 = Qz * (Qp * Op2 + Qp + Op);
    // calculate the coefficients
    b[0] = Qp * (Qz * Oz2 + Qz + Oz)    / a0;
    b[1] = Qp * (2 * Qz * Oz2 - 2 * Qz) / a0;
    b[2] = Qp * (Qz * Oz2 + Qz - Oz)    / a0;
    a[0] = Qz * (2 * Qp * Op2 - 2 * Qp) / a0;
    a[1] = Qz * (Qp * Op2 + Qp - Op)    / a0;
}

/// @brief Calculate the parameters of a digital all-pass filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 3 numerator coefficients of the filter, \f$b\f$
/// @param a the 2 denominator coefficients of the filter, \f$a\f$
/// @param O the cut-off frequency of the filter, \f$\Omega_c \in [0, \pi]\f$
/// @param Q the quality-factor of the filter, \f$Q\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]` and \f$a_2 =\f$ `a[1]`.
///
template<typename T>
inline void allpass(T b[3], T a[2], T O, T Q) {
    // pre-warp critical frequencies
    O = tan(O / 2);
    // cache O**2 * Q
    const T O2Q = O * O * Q;
    const T a0 = O2Q + Q + O;
    // calculate the coefficients
    b[0] = (O2Q + Q - O)     / a0;
    b[1] = (2 * O2Q - 2 * Q) / a0;
    b[2] = (O2Q + Q + O)     / a0;
    a[0] = (2 * O2Q - 2 * Q) / a0;
    a[1] = (O2Q + Q - O)     / a0;
}

/// @brief Calculate the parameters of a digital band-pass filter.
///
/// @tparam T the data-type for storing and computing the parameters
/// @param b the 3 numerator coefficients of the filter, \f$b\f$
/// @param a the 2 denominator coefficients of the filter, \f$a\f$
/// @param O the cut-off frequency of the filter, \f$\Omega_c \in [0, \pi]\f$
/// @param Q the quality-factor of the filter, \f$Q\f$
/// @details
/// Parameters are normalized to assume \f$a_0\f$ = 1.
/// As such, \f$a_0\f$ is omitted and \f$a_1 =\f$ `a[0]` and \f$a_2 =\f$ `a[1]`.
///
template<typename T>
inline void bandpass(T b[3], T a[2], T O, T Q) {
    // pre-warp critical frequencies
    O = tan(O / 2);
    // cache O**2 * Q
    const T O2Q = O * O * Q;
    const T a0 = O2Q + Q + O;
    // calculate the coefficients
    b[0] = O  / a0;
    b[1] = 0  / a0;
    b[2] = -O / a0;
    a[0] = (2 * O2Q - 2 * Q) / a0;
    a[1] = (O2Q + Q - O)     / a0;
}

}  // namespace Biquadratic

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_BIQUADRATIC_COEFFICIENTS_HPP
