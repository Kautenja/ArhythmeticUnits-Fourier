// Functions for working with first-order IIR filter coefficients.
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

#ifndef DSP_FILTER_IIR_CHEBYSHEV_TYPE_2_FREQZ_HPP
#define DSP_FILTER_IIR_CHEBYSHEV_TYPE_2_FREQZ_HPP

#include <complex>

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief Chebyshev Type 2 IIR filter designs (i.e., first-order).
namespace ChebyshevType2 {

/// @brief Return the frequency response for a 1-pole filter.
///
/// @param b the numerator coefficients in reverse order \f$[z^1, z^0]\f$
/// @param a the denominator constant \f$a_1\f$
/// @param o the scalar frequency \f$\in [0, \pi]\f$
/// @returns the complex frequency response
///
template<typename T>
std::complex<T> freqz(const T b[2], const T a[1], T o) {
    static constexpr auto J = std::complex<T>(0, 1);
    const auto z = exp(J * o);
    const auto B = b[0] * z + b[1];
    const auto A = z + a[0];
    return B / A;
}

}  // namespace ChebyshevType2

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_CHEBYSHEV_TYPE_2_FREQZ_HPP
