// Functions for working with order-n IIR filter coefficients.
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

#ifndef DSP_FILTER_IIR_BIPOLYNOMIAL_FREQZ_HPP
#define DSP_FILTER_IIR_BIPOLYNOMIAL_FREQZ_HPP

#include <complex>

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief Bipolynomial IIR filter designs (i.e., N'th-order).
namespace Bipolynomial {

// TODO: test cases
/// @brief Return the frequency response for an \f$n\f$-pole filter.
///
/// @param b the numerator coefficients in reverse order \f$[z^n, ..., z^0]\f$
/// @param a the denominator coefficients in reverse order \f$[z^{n - 1}, ..., z^0]\f$
/// @param o the scalar frequency \f$\in [0, \pi]\f$
/// @returns the complex frequency response
///
template<typename T, int n>
std::complex<T> freqz(const T b[n], const T a[n - 1], T o) {
    static constexpr auto J = std::complex<T>(0, 1);
    const auto Z = exp(J * o);
    auto z = Z;
    T B = b[n - 1];
    T A = a[n - 2];
    for (int i = n - 2; i >= 0; i--) {
        B += z * b[i];
        A += z * b[i - 1];
        z *= Z;
    }
    return B / A;
}

}  // namespace Bipolynomial

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_BIPOLYNOMIAL_FREQZ_HPP
