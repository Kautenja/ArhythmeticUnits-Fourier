// A clone of the Shlappi Engineering 100 Grit module.
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

#ifndef DSP_FILTER_IIR_CASCADE_HUNDRED_GRIT_HPP
#define DSP_FILTER_IIR_CASCADE_HUNDRED_GRIT_HPP

#include <random>
#include "../../../math.hpp"
#include "../bilinear/direct_form_2_transpose.hpp"

// TODO: random noise source is massively audible in downstream filters.

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief a Transistor ladder filter as a cascade of 1-pole filters.
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
/// The ladder filter is computed by cascading these four low-pass filters
/// together, whom have a phase response of \f$-45\f$ degrees at the cut-off,
/// and feeding back the inverted output into the ladder with a scaling
/// variable \f$-Q\f$ to invert the phase and control the resonance. The
/// filter _should_ self-oscillate when \f$Q > 4\f$. The ladder structure, as
/// a whole, looks like:
///
/// ```
///         |-----|  |-----|  |-----|  |-----|
/// x[n] -+-| LPF |--| LPF |--| LPF |--| LPF |-•--> y[n]
///       | |-----|  |-----|  |-----|  |-----| |
///       |                                    |
///       •---------------- *-Q ---------------•
/// ```
///
/// The cascade is depicted as a cascade of first-order low-pass filters, but
/// can also be configured using high-pass filters. The delay-free loop in the
/// block diagram (i.e., from the \f$Q\f$ feedback) are resolved by unit-delays
/// that slightly change the frequency response relative to the hardware
/// circuit. Most notably, self resonance begins at much smaller
/// \f$Q\f$ values of \f$\approx 2.2\f$.
///
template<typename T>
struct HundredGrit {
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

    /// The last output from the ladder filter \f$y\f$
    T y = 0;

    /// @brief Clear the contents of the delay line.
    inline void reset() { h[0] = h[1] = h[2] = h[3] = y = 0; }

    /// @brief Pass a value through the filter and return the output.
    ///
    /// @param value the value to process with the filter
    /// @returns the current output from the filter
    ///
    inline T process(const T& value) {
        static auto generator = std::default_random_engine();
        static auto distribution = std::normal_distribution<T>(0, 1e-6);
        // calculate the input to the network by clipping the feedback after
        // applying the Q gain. Add a small random value to the input to allow
        // the resonance to kick in when Q is sufficiently high, even when the
        // input signal to the filter is 0.
        const T x0 = value - Math::Clipping::cubic(Q * y) + distribution(generator);
        // the input to the cascade is the sum of the current value (x[n]) and
        // the inverted last output from the filter -y scaled by the quality
        // factor Q to produce a resonance at the cut-off frequency
        const T x1 = Bilinear::direct_form_2_transpose(b, a, h[0], x0);
        const T x2 = Bilinear::direct_form_2_transpose(b, a, h[1], x1);
        const T x3 = Bilinear::direct_form_2_transpose(b, a, h[2], x2);
                 y = Bilinear::direct_form_2_transpose(b, a, h[3], x3);
        return y;
    }

    inline T get_pole1() const { return h[0]; }

    inline T get_pole2() const { return h[1]; }

    inline T get_pole3() const { return h[2]; }

    inline T get_pole4() const { return h[3]; }
};

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_CASCADE_HUNDRED_GRIT_HPP
