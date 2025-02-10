// A trigger that detects a threshold value.
// Copyright 2020 Christian Kauten
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

#ifndef DSP_TRIGGER_THRESHOLD_HPP
#define DSP_TRIGGER_THRESHOLD_HPP

/// @brief A collection of structures for detecting trigger events.
namespace Trigger {

/// @brief A trigger that detects a threshold value.
/// @tparam T the data type for processing input signals
template<typename T>
struct Threshold {
 private:
    /// the current value of the trigger's signal
    bool state = false;

 public:
    /// @brief Return the state of the boolean trigger. The state will go true
    /// after processing an input signal of \f$1.0\f$, and will stay high until
    /// the signal reaches \f$0.0\f$.
    ///
    inline const bool& isHigh() const { return state; }

    /// @brief Reset the trigger to its default state
    inline void reset() { state = false; }

    /// @brief Process a step of the signal.
    ///
    /// @param signal a sample of an arbitrary signal
    /// @returns true if the trigger goes above \f$1.0\f$ and false if it goes
    /// below \f$0.0\f$. The trigger goes high once per cycle and must return
    /// to \f$0.0\f$ before firing again, .i.e, `isHigh` will go true at
    /// \f$1.0\f$ and stay high until the signal reaches \f$0.0\f$.
    ///
    inline bool process(const T& signal) {
        if (state)  // HIGH to LOW
            state = signal > 0;
        else if (signal >= 1)  // LOW to HIGH (trigger event, return true)
            return state = true;
        // no trigger event, return false
        return false;
    }
};

}  // namespace Trigger

#endif  // DSP_TRIGGER_THRESHOLD_HPP
