// A first-order IIR filter in transposed direct form 1 configuration.
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

#ifndef DSP_FILTER_IIR_BILINEAR_DIRECT_FORM_1_TRANSPOSE_HPP
#define DSP_FILTER_IIR_BILINEAR_DIRECT_FORM_1_TRANSPOSE_HPP

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief Bilinear IIR filter designs (i.e., first-order).
namespace Bilinear {

/// @brief Process a sample using a transposed direct form 1 IIR design.
///
/// @param b the numerator coefficients of the filter \f$b\f$
/// @param a the denominator coefficients of the filter \f$a\f$
/// @param h_b the delay line for the feed-forward network \f$h_b\f$
/// @param h_a the delay line for the feedback network \f$h_a\f$
/// @param value the value to process with the filter
/// @returns the output from the filter
///
template<typename T>
inline T direct_form_1_transpose(T b[2], T a[1], T& h_b, T& h_a, const T& value) {
    const T feedback = value + h_a;
    h_a = -a[0] * feedback;
    const T feedforward = b[0] * feedback + h_b;
    h_b = b[1] * feedback;
    return feedforward;
}

/// @brief A first-order IIR filter in transposed direct form 1 configuration.
/// @tparam T the data type to process and use for coefficients
/// @details
/// The network structure of the filter looks like:
///
/// ```
/// x[n] -+----------•-- *b0 -->+--> y[n]
///       |          |          |
///      z-1         |         z-1
///       |          |          |
///       +<- *-a0 --•-- *b1 -->+
/// ```
///
template<typename T>
struct DirectForm1Transpose {
    /// the coefficients of the numerator of the filter \f$b\f$
    T b[2] = {0, 0};
    /// the coefficients of the denominator of the filter \f$a\f$
    T a[1] = {0};

    /// the delay line for the feed-forward network \f$h_b\f$
    T h_b = 0;
    /// the delay line for the feedback network \f$h_a\f$
    T h_a = 0;

    /// @brief Clear the contents of the delay lines.
    inline void reset() { h_a = h_b = 0; }

    /// @brief Pass a value through the filter and return the output.
    ///
    /// @param value the value to process with the filter
    /// @returns the current output from the filter
    ///
    inline T process(const T& value) {
        return direct_form_1_transpose(b, a, h_b, h_a, value);
    }
};

}  // namespace Bilinear

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_BILINEAR_DIRECT_FORM_1_TRANSPOSE_HPP
