// A Korg35 low-pass filter (i.e., from the MS-10 and MS-20).
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

#ifndef DSP_FILTER_IIR_CASCADE_KORG35_LPF_HPP
#define DSP_FILTER_IIR_CASCADE_KORG35_LPF_HPP

#include "../../../math.hpp"
#include "../bilinear/direct_form_2_transpose.hpp"

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief A Korg35 low-pass filter (i.e., from the MS-10 and MS-20).
/// @tparam T the data type to process and use for coefficients
template<typename T>
struct Korg35LPF {
    /// Coefficients of the numerator of the 1-pole low-pass filters \f$b\f$.
    /// Both filters in the cascade shares the same coefficients.
    T b_lowpass[2] = {0, 0};
    /// Coefficients of the denominator of the 1-pole low-pass filters \f$a\f$.
    /// Both filters in the cascade shares the same coefficients.
    T a_lowpass[1] = {0};

    /// Coefficients of the numerator of the 1-pole high-pass filters \f$b\f$.
    T b_highpass[2] = {0, 0};
    /// Coefficients of the denominator of the 1-pole high-pass filters \f$a\f$.
    T a_highpass[1] = {0};

    /// @brief quality factor that controls the feedback into the ladder.
    /// @details
    /// Stable values for Q are normally in the range of [0.01, 2]
    T Q = 0;

    /// The delay lines for the 1-pole filters in the network \f$h\f$. Each
    /// index of the vector represents the single sample delay for one of the
    /// filters in the cascade, i.e., this is NOT a delay line of the previous
    /// four samples.
    T h[3] = {0, 0, 0};

    /// The last output from the ladder filter \f$y\f$
    T y = 0;

    /// @brief Clear the contents of the delay line.
    inline void reset() { h[0] = h[1] = h[2] = y = 0; }

    /// @brief Pass a value through the filter and return the output.
    ///
    /// @param value the value to process with the filter
    /// @returns the current output from the filter
    ///
    inline T process(const T& value) {
        const T x1 =     Bilinear::direct_form_2_transpose(b_lowpass, a_lowpass, h[0], value);
        const T x2 =     Bilinear::direct_form_2_transpose(b_lowpass, a_lowpass, h[1], y + x1);
                 y = Q * Bilinear::direct_form_2_transpose(b_highpass, a_highpass, h[2], x2);
        return x2;
    }
};

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_CASCADE_KORG35_LPF_HPP
