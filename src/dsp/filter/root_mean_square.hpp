// A structure for calculating a moving average.
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

#ifndef DSP_FILTER_ROOT_MEAN_SQUARE_HPP
#define DSP_FILTER_ROOT_MEAN_SQUARE_HPP

#include "moving_average.hpp"

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief A meter for calculating the RMS of a signal.
struct RootMeanSquare : public MovingAverageFIR {
 public:
    /// @brief Process the input value.
    ///
    /// @param value the value to process with the integrator
    /// @returns the current value of the moving average
    ///
    float process(float value) {
        // Update the moving average on parent using the squared value.
        const float average = MovingAverageFIR::process(Math::squared(value));
        // Return the RMS value based on the mean square from parent.
        // The minimum average is -60dB, i.e., the lowest audible value.
        static constexpr float MIN_AVERAGE = 1e-6;
        // This calculation avoids the square root of the mean square by
        // squaring the log and pulling out a factor of 0.5 (equivalently).
        return 10.f * std::log10(std::max(average, MIN_AVERAGE));
    }
};

}  // namespace Filter

#endif  // DSP_FILTER_ROOT_MEAN_SQUARE_HPP
