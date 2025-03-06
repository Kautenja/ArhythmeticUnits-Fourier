// A trigger that detects a threshold value held for a period of time.
//
// Copyright 2025 Arhythmetic Units
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

#ifndef DSP_TRIGGER_HOLD_HPP
#define DSP_TRIGGER_HOLD_HPP

/// @brief A collection of structures for detecting trigger events.
namespace Trigger {

/// @brief A trigger that detects a threshold value held for a period of time.
struct Hold {
    /// @brief states that the trigger can be in
    enum class State {
        /// indicates the trigger is idle (neither pressed nor held)
        Off = 0,
        /// indicates the trigger is being pressed, but not held
        Pressed,
        /// indicates the trigger is being held longer than `HOLD_TIME`
        Held
    };

    /// @brief events that can fire from the trigger
    enum class Event {
        /// indicates no trigger event occurred during a sample
        None = 0,
        /// indicates the trigger was released during the pressed state
        Press,
        /// indicates the trigger has been pressed for longer than `HOLD_TIME`
        BeginHold,
        /// indicates the trigger is being released from the held state
        ReleaseHold
    };

    /// @brief the number of seconds to wait for detecting a hold
    static constexpr float HOLD_TIME = 0.100;

 private:
    /// the current state of the trigger
    State state = State::Off;

    /// the current time, only used when the trigger is pressed
    float time = 0.f;

 public:
    /// @brief Return the current state of the trigger.
    ///
    /// @returns the current state of the trigger:
    ///          `State::Off`, `State::Pressed`, or  `State::Held`
    ///
    inline State getState() const { return state; }

    /// @brief Return true if the trigger is neither pressed, not held.
    ///
    /// @returns true if the state is `State::Off`
    ///
    inline bool isOff() const { return state == State::Off; }

    /// @brief Return true if the trigger is being pressed, but not yet held.
    ///
    /// @returns true if the state is `State::Pressed`
    ///
    inline bool isPressed() const { return state == State::Pressed; }

    /// @brief Return true if the trigger is being held, opposed to triggered.
    ///
    /// @returns true if the state is `State::Held`
    ///
    inline bool isHeld() const { return state == State::Held; }

    /// @brief Reset the trigger to the default state.
    inline void reset() { state = State::Off; }

    /// @brief Process a sample of the input signal.
    ///
    /// @param signal the input signal to process
    /// @param sampleTime the amount of time between samples, i.e.,
    /// \f$T_s = \frac{1}{f_s}\f$
    /// @returns the event that occurred during processing, i.e., `Event::None`
    ///          when the trigger is not firing, `Event::Press` when the
    ///          trigger detects a press event, `Event::BeginHold` when the
    ///          trigger is held past `HOLD_TIME`, and `Event::ReleaseHold`
    ///          when the trigger is exiting a hold
    /// @details
    /// When the input signal first goes high the trigger does not fire. The
    /// trigger only fires if the input signal goes low before `HOLD_TIME`
    /// elapses. If the input signal stays high past `HOLD_TIME`, the trigger
    /// enters the `State::Held` state, accessed using either `getState()` or
    /// `isHeld()`.
    ///
    inline Event process(float signal, float sampleTime) {
        switch (state) {
        case State::Off:  // off; detect initial press event
            if (signal >= 1.f) {  // initial press event; reset timer
                state = State::Pressed;
                time = 0.f;
            }
            break;
        case State::Pressed:  // pressing; might be holding
            if (signal <= 0.f) {  // stopped pressing before hold time
                state = State::Off;
                return Event::Press;
            } else {  // still pressing
                time += sampleTime;
                if (time >= HOLD_TIME) {  // entering hold state
                    state = State::Held;
                    return Event::BeginHold;
                }
            }
            break;
        case State::Held:  // holding; might be releasing
            if (signal <= 0.f) {  // turning off, enter off and release hold
                state = State::Off;
                return Event::ReleaseHold;
            }
            break;
        }
        return Event::None;
    }
};

}  // namespace Trigger

#endif  // DSP_TRIGGER_HOLD_HPP
