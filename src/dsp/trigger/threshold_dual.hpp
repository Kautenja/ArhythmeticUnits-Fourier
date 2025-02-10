// A trigger that has two inputs.
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

#ifndef DSP_TRIGGER_THRESHOLD_DUAL
#define DSP_TRIGGER_THRESHOLD_DUAL

#include "threshold.hpp"

// TODO: replace `rescale` call with simpler logic.

/// @brief A collection of structures for detecting trigger events.
namespace Trigger {

/// a trigger for a button with a CV input.
/// @tparam T the data type for processing input signals
template<typename T>
struct ThresholdDual {
    /// the trigger for the button
    Trigger::Threshold<T> buttonTrigger;
    /// the trigger for the CV
    Trigger::Threshold<T> cvTrigger;

    /// Process the input signals.
    ///
    /// @param button the value of the button signal [0, 1]
    /// @param cv the value of the CV signal [-10, 10]
    /// @returns true if either signal crossed a rising edge
    ///
    inline bool process(const T& button, const T& cv) {
        bool buttonPress = buttonTrigger.process(button);
        bool cvGate = cvTrigger.process(rescale(cv, T(0.1), T(2), T(0), T(1)));
        return buttonPress or cvGate;
    }

    /// Return a boolean determining if either the button or CV gate is high.
    inline const bool& isHigh() {
        return buttonTrigger.isHigh() or cvTrigger.isHigh();
    }
};

}  // namespace Trigger

#endif  // DSP_TRIGGER_THRESHOLD_DUAL
