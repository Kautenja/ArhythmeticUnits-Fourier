// An emulation of a VTL5C3 Vactrol.
// Copyright 2020 Christian Kauten
// Copyright 2019 Joshua Cearley
// Creative Commons: CC-BY-SA
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

#ifndef DSP_FILTER_VACTROL_HPP
#define DSP_FILTER_VACTROL_HPP

#include <cmath>

/// @brief Structure for digital filtering.
namespace Filter {

/// @brief Vactrol emulations.
namespace Vactrol {

/// @brief an emulation of a VTL5C3 vactrol.
template<typename T>
struct VTL5C3 {
 private:
    /// the rate of rise (alpha parameter for EMA during rise stage)
    T rise = 0.0;
    /// the rate of fall (alpha parameter for EMA during fall stage)
    T fall = 0.0;
    /// the exponential moving average of the input signal
    T average = 0.0;

 public:
    /// @brief Emulate the response curve of the VTL51C vactrol.
    ///
    /// @param x the value to curve
    /// @returns x curved along the emulated response curve of a VTL51C vactrol
    /// @details
    /// This was derived by curve fitting against the spec sheet provided by
    /// the manufacturer. Output values are computed using single precision
    /// floating point arithmetic.
    ///
    static inline T curve(T x) {
        static constexpr T a = 19977.0579;
        static constexpr T b = 4.72586603;
        static constexpr T c = 22.9420751;
        static constexpr T base = 2.71828;
        static constexpr T _1 = 1.0;
        static constexpr T _20000 = 20000.0;
        const T resistance = (a * (pow(base, (-b * x)))) + c;
        return (_1 - (resistance / _20000));
    }

    /// @brief Set the sample rate to a new value.
    ///
    /// @param sample_rate the sample rate to use, \f$f_s \in [8, 728]KHz\f$
    ///
    inline void set_sample_rate(T sample_rate) {
        const T period = static_cast<T>(1.0) / sample_rate;
        {
            static constexpr T a = 209.616712;
            static constexpr T b = 0.000880319056;
            static constexpr T c = 48113.5069;
            fall = (period * period * c) + (period * a) + b;
        } {
            static constexpr T a = 2746.38887;
            static constexpr T b = 0.000319227063;
            static constexpr T c = -3665711.27;
            rise = (period * period * c) + (period * a) + b;
        }
    }

    /// @brief Process a sample using the filter.
    ///
    /// @param value the value to process with the filter
    /// @returns the current output from the filter
    ///
    inline T process(T value) {
        // update the exponential moving average of the input signal
        average += (value > average ? rise : fall) * (value - average);
        return curve(average);
    }
};

}  // namespace Vactrol

}  // namespace Filter

#endif  // DSP_FILTER_VACTROL_HPP
