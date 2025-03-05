// A trigger that detects when a signal goes from negative to positive.
//
// Copyright 2024 Arhythmetic Units
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

#ifndef DSP_TRIGGER_ZERO_HPP
#define DSP_TRIGGER_ZERO_HPP

#include "common.hpp"

/// @brief A collection of structures for detecting trigger events.
namespace Trigger {

/// @brief A trigger that detects when a signal goes from negative to positive.
/// @tparam T the data type for processing input signals
template<typename T>
struct Zero {
 private:
    /// the value of the signal on the last sample, i.e., last call to process
    T input_last = 0;

    /// @brief Detect an edge based on signal input.
    ///
    /// @tparam direction the direction of the edge to detect
    /// @param input the input signal to compare against the last sample
    /// @returns true if there is an edge between this sample and the last
    ///
    template<Direction direction>
    bool detect_edge(const T& input) const;
    // Rising Edge
    template<>
    inline bool detect_edge<Direction::Rise>(const T& input) const {
        return input_last < T(0) && input >= T(0);
    }
    // Falling Edge
    template<>
    inline bool detect_edge<Direction::Fall>(const T& input) const {
        return input_last > T(0) && input <= T(0);
    }

 public:
    /// @brief Reset the trigger to its default state
    inline void reset() { input_last = T(0); }

    /// @brief Process a step of the boolean signal.
    ///
    /// @tparam direction the direction of the edge to detect
    /// @param input a sample of a boolean signal
    /// @param offset an optional offset control for shifting away from zero
    /// @returns true if the state changes according to the edge direction
    ///
    template<Direction direction>
    inline bool process(T input, const T& offset = T(0)) {
        // Remove the offset to center the zero trigger position back at zero.
        input -= offset;
        // Detect a trigger event by looking for a change in sign. This assumes
        // that the signal is zero centered.
        const bool triggered = detect_edge<direction>(input);
        // Update the reference to the previous signal for detecting the next
        // trigger event.
        input_last = input;
        // Return the trigger event as a boolean value.
        return triggered;
    }
};

}  // namespace Trigger

#endif  // DSP_TRIGGER_ZERO_HPP
