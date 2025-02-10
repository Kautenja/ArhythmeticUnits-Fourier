// An oscillator that generates a pulse wave for clock.
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

#ifndef DSP_CLOCK_GENERATOR_HPP
#define DSP_CLOCK_GENERATOR_HPP

#include <algorithm>
#include "../math.hpp"

/// @brief Structures for working with time.
namespace Clock {

/// @brief A pulse-wave clock signal generator.
struct Generator {
 private:
    /// the current phase of the clock around the unit circle, i.e., in [0, 1]
    float phase = 0.f;
    /// the current frequency of the clock measured in Hertz
    float frequency = 120.f;
    /// the current width of the pulse, i.e., in (0, 1)
    float pulseWidth = 0.5f;

 public:
    /// @brief true if the clock is advancing, false otherwise
    bool isRunning = false;

    /// @brief Reset the phase of the clock.
    /// @details
    /// Neither the pulse width nor the frequency will be effected; only the
    /// phase of the clock is reset to \f$0\f$.
    inline void reset() { phase = 0.f; }

    /// @brief Set the frequency of the clock to a new value.
    ///
    /// @param value the new frequency to set the clock to, measured in Hertz
    /// \f$\in [0.0001, 20000]\f$
    /// @details
    /// The frequency is clamped to positive values above \f$0.0001Hz\f$ and
    /// below \f$20000Hz\f$.
    ///
    inline void setFrequency(float value) {
        frequency = Math::clip(value, 0.0001f, 20000.f);
    }

    /// @brief Return the frequency of the clock measured in Hertz.
    ///
    /// @returns the frequency of the clock, measured in Hertz
    /// \f$\in [0.0001, 20000]\f$
    ///
    inline float getFrequency() const { return frequency; }

    /// @brief Set the period of the clock to a new value.
    ///
    /// @param value the new period to set the clock to, measured in seconds
    /// \f$\in [0.00005, 10000]\f$
    /// @details
    /// The period is clamped to positive values above \f$0.00005s\f$ and below
    /// \f$10000s\f$.
    ///
    inline void setPeriod(float value) {
        setFrequency(1.f / std::max(0.00005f, value));
    }

    /// @brief Return the period of the clock measured in seconds.
    ///
    /// @returns the number of seconds in a full cycle of the clock pulse wave
    ///
    inline float getPeriod() const { return 1.f / frequency; }

    /// @brief Set the width of the pulse wave to a new value.
    ///
    /// @param value the new width for the pulse wave oscillator, measured in
    ///        percent \f$\in [0.01, 0.99]\f$
    /// @details
    /// The minimal pulse width is \f$1\%\f$, and the maximal pulse width is
    /// \f$99\%\f$.
    ///
    inline void setPulseWidth(float value) {
        pulseWidth = Math::clip(value, 0.01f, 0.99f);
    }

    /// @brief Return the pulse width of the clock.
    ///
    /// @returns the pulse width, measured in percent \f$\in [0.01, 0.99]\f$
    ///
    inline float getPulseWidth() const { return pulseWidth; }

    /// @brief Return the phase of the clock.
    ///
    /// @returns the phase of the pulse wave oscillator \f$\in [0, 1]\f$
    ///
    inline float getPhase() const { return phase; }

    /// @brief Return the amount of time until the next rising edge.
    ///
    /// @returns the number of seconds until the next rising edge event
    ///
    inline float getTime() const {return (1.f - phase) / frequency; }

    /// @brief Return the value of the pulse wave.
    ///
    /// @returns true if the pulse wave is high, false otherwise
    ///
    inline bool getGate() const {
        // - the clock is low if it isn't running; use this knowledge to
        //   terminate the logic early (with &&).
        // - the clock's pulse is high when the phase is less than the width
        // - use a small epsilon value to resolve weird issues with float logic
        return isRunning && phase < (pulseWidth - std::numeric_limits<float>::epsilon());
    }

    /// @brief Process a sample for given change in time.
    ///
    /// @param deltaTime the change in time between samples, i.e., \f$T_s\f$
    /// @details
    /// If `isRunning` is false, no change in phase is processed.
    ///
    inline void process(float deltaTime) {
        // if not running, don't process
        if (!isRunning) return;
        // increment the phase counter bounded to the precision of single
        // precision floating point numbers, and the Nyquist frequency
        phase += Math::clip(frequency * deltaTime, std::numeric_limits<float>::epsilon(), 0.5f);
        // wrap phase around circularly
        if (phase >= 1.f - std::numeric_limits<float>::epsilon()) phase -= 1.f;
    }
};

}  // namespace Clock

#endif  // DSP_CLOCK_GENERATOR_HPP
