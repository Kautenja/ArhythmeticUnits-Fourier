// A diode ladder filter based on cascaded 1-pole filters.
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

#ifndef DSP_FILTER_IIR_CASCADE_DIODE_LADDER_HPP
#define DSP_FILTER_IIR_CASCADE_DIODE_LADDER_HPP

#include <random>
#include "../../../math.hpp"
#include "../bilinear/direct_form_2_transpose.hpp"

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief A diode ladder filter as a cascade of 1-pole filters.
/// @tparam T the data type to process and use for coefficients
/// @details
/// Each pole of the filter is implemented as a 1-pole filter in a direct form 2
/// transpose network structure. The network structure of the individual 1-pole
/// low-pass filters looks like:
///
/// ```
/// x[n] -•-- *b0 -->+----------•--> y[n]
///       |          |          |
///       |         z-1         |
///       |          |          |
///       •-- *b1 -->+<- *-a0 --•
/// ```
///
/// The diode ladder differs from the transistor ladder by introducing a more
/// complex feedback structure between the four 1-pole filters. The ladder
/// structure, as a whole, looks like:
///
/// ```
///                                      •<--------------------------------•
///                                      |                                 |
///                   •<----------------------------------•                |
///                   |                  |                |                |
///         •<-------------------------• |                |                |
///         |         |                | |                |                |
///         | |-----| |        |-----| | |        |-----| |        |-----| |
/// x[n] -+-+-| LPF |-+- *0.5 -| LPF |-•-+- *0.5 -| LPF |-•- *0.5 -| LPF |-•-> y[n]
///       |   |-----|          |-----|            |-----|          |-----| |
///       |                                                                |
///       •<-------------------------------- *-Q --------------------------•
/// ```
///
/// The cascade is depicted as a cascade of first-order low-pass filters, but
/// can also be configured using high-pass filters. Delay-free loops in the
/// block diagram are resolved by unit-delays that slightly change the
/// frequency response relative to the hardware circuit.
///
template<typename T>
struct DiodeLadder {
    /// The coefficients of the numerator of the 1-pole filters \f$b\f$. Each
    /// filter in the cascade shares the same coefficients.
    T b[2] = {0, 0};
    /// The coefficients of the denominator of the 1-pole filters \f$a\f$. Each
    /// filter in the cascade shares the same coefficients.
    T a[1] = {0};

    /// The quality factor that controls the amount of feedback into the ladder.
    T Q = 0;

    /// The delay lines for the 1-pole filters in the network \f$h\f$. Each
    /// index of the vector represents the single sample delay for one of the
    /// filters in the cascade, i.e., this is NOT a delay line of the previous
    /// four samples.
    T h[4] = {0, 0, 0, 0};

    /// The outputs from individual 1-pole filter stages. This is NOT a delay
    /// line of the previous four sample outputs.
    T y[4] = {0, 0, 0, 0};

    /// @brief Clear the contents of the delay line.
    inline void reset() {
        h[0] = h[1] = h[2] = h[3] = 0;
        y[0] = y[1] = y[2] = y[3] = 0;
    }

    /// @brief Pass a value through the filter and return the output.
    ///
    /// @param value the value to process with the filter
    /// @returns the current output from the filter
    ///
    inline T process(const T& value) {
        static auto generator = std::default_random_engine();
        static auto distribution = std::normal_distribution<T>(T(0), std::numeric_limits<T>::epsilon());
        // calculate the input to the network by clipping the feedback after
        // applying the Q gain. Add a small random value to the input to allow
        // the resonance to kick in when Q is sufficiently high, even when the
        // input signal to the filter is 0.
        const T x = value - Math::Clipping::hyperbolic_tangent(Q * y[3]) + distribution(generator);
        y[0] = Bilinear::direct_form_2_transpose(b, a, h[0], x + y[1]);
        y[1] = Bilinear::direct_form_2_transpose(b, a, h[1], 0.5f * (y[0] + y[2]));
        y[2] = Bilinear::direct_form_2_transpose(b, a, h[2], 0.5f * (y[1] + y[3]));
        y[3] = Bilinear::direct_form_2_transpose(b, a, h[3], 0.5f * y[2]);
        return y[3];
    }
};

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_CASCADE_DIODE_LADDER_HPP
