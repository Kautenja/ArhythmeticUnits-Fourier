// Extensions to the VCV Rack ParamQuantity class.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_PARAM_QUANTITY_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PARAM_QUANTITY_HPP_

#include <string>
#include "rack.hpp"

/// @brief A parameter quantity for a boolean switch.
struct BooleanParamQuantity : rack::ParamQuantity {
    /// @brief Return the value as a formatted string.
    inline std::string getDisplayValueString() override {
        if (getValue()) return "On";
        return "Off";
    }
};

/// @brief A parameter quantity for a trigger button.
struct TriggerParamQuantity : rack::ParamQuantity {
    /// @brief Return the parameter description instead of the value text.
    inline std::string getDisplayValueString() override {
        return rack::ParamQuantity::getLabel();
    }

    /// @brief Return the parameter description (disabled for this trigger).
    inline std::string getLabel() override { return ""; }
};

#endif  // ARHYTHMETIC_UNITS_FOURIER_PARAM_QUANTITY_HPP_
