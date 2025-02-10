// A first-order IIR filter in direct form 2 configuration.
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

#ifndef DSP_FILTER_IIR_BILINEAR_DIRECT_FORM_2_HPP
#define DSP_FILTER_IIR_BILINEAR_DIRECT_FORM_2_HPP

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief Bilinear IIR filter designs (i.e., first-order).
namespace Bilinear {

/// @brief Process a sample using a direct form 2 IIR design.
///
/// @param b the numerator coefficients of the filter \f$b\f$
/// @param a the denominator coefficients of the filter \f$a\f$
/// @param h the delay line for the network \f$h\f$
/// @param value the value to process with the filter
/// @returns the output from the filter
///
template<typename T>
inline T direct_form_2(T b[2], T a[1], T& h, const T& value) {
    const T feedback = value - a[0] * h;
    const T feedforward = b[0] * feedback + b[1] * h;
    h = feedback;
    return feedforward;
}

/// @brief A first-order IIR filter in direct form 2 configuration.
/// @tparam T the data type to process and use for coefficients
/// @details
/// The network structure of the filter looks like:
///
/// ```
/// x[n] -+----------•-- *b0 -->+--> y[n]
///       |          |          |
///       |         z-1         |
///       |          |          |
///       +<- *-a0 --•-- *b1 -->+
/// ```
///
template<typename T>
struct DirectForm2 {
    /// the coefficients of the numerator of the filter \f$b\f$
    T b[2] = {0, 0};
    /// the coefficients of the denominator of the filter \f$a\f$
    T a[1] = {0};

    /// the delay line for the network \f$h\f$
    T h = 0;

    /// @brief Clear the contents of the delay line.
    inline void reset() { h = 0; }

    /// @brief Pass a value through the filter and return the output.
    ///
    /// @param value the value to process with the filter
    /// @returns the current output from the filter
    ///
    inline T process(const T& value) {
        return direct_form_2(b, a, h, value);
    }
};

}  // namespace Bilinear

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_BILINEAR_DIRECT_FORM_2_HPP
