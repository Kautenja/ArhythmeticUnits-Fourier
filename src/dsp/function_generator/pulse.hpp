// A one-shot pulse generator.
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

#ifndef ENVELOPE_AD_HPP
#define ENVELOPE_AD_HPP

#include <cstdint>
#include <algorithm>
#include "../math.hpp"
#include "_common.hpp"

/// @brief Structures for generating functions.
namespace FunctionGenerator {

/// @brief A one-shot pulse generator.
struct Pulse {
 private:
    /// the amount of time in the pulse width
    float pulseTime = 0.001;

    /// the current time, only used when the trigger is pressed
    float time = 0.f;

    /// whether the pulse generator has been triggered
    bool isTriggered = false;

 public:
    /// @brief Set the pulse width (in time domain) to a new value.
    ///
    /// @param seconds the number of seconds in the pulse
    /// @details
    /// The minimal pulse time is 1ms (0.001 seconds).
    ///
    inline void setPulseTime(float pulseTime_) {
        pulseTime = std::max(pulseTime_, 0.001f);
    }

    /// @brief Return the pulse time in seconds.
    inline float getPulseTime() const { return pulseTime; }

    /// @brief Reset the pulse generator state.
    /// @details
    /// The pulseTime is not effected by this function.
    inline void reset() {
        time = 0.f;
        isTriggered = false;
    }

    /// @brief Trigger a pulse.
    inline void trigger() {
        time = 0.f;
        isTriggered = true;
    }

    /// @brief Return true if the pulse generator has been triggered.
    inline bool getIsTriggered() { return isTriggered; }

    /// @brief Process a sample from the pulse generator.
    ///
    /// @param sampleTime the amount of time that the sample consumes
    /// @returns true if the pulse generator is high, false otherwise
    ///
    inline bool process(float sampleTime) {
        if (isTriggered) {  // generating pulse
            // update the time for the pulse generator
            time += sampleTime;
            // reset the isTriggered flag based on the current time relative to
            // the pulse time
            isTriggered = time < pulseTime;
        }
        return isTriggered;
    }
};

}  // namespace FunctionGenerator

#endif  // ENVELOPE_AD_HPP
