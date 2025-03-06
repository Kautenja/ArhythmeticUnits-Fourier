// Extensions to the VCV Rack ParamQuantity class.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_PARAM_QUANTITY_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_PARAM_QUANTITY_HPP_

#include <string>
#include "rack.hpp"
#include "../structs.hpp"
#include "../dsp/math.hpp"

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

/// @brief A parameter quantity for window function selection.
struct WindowFunctionParamQuantity : rack::ParamQuantity {
    /// @brief Return the value as a formatted string.
    inline std::string getDisplayValueString() final {
        return Math::Window::name(static_cast<Math::Window::Function>(getValue()));
    }
};

/// @brief A parameter quantity for magnitude scale selection.
struct MagnitudeScaleParamQuantity : rack::ParamQuantity {
    /// @brief Return the value as a formatted string.
    inline std::string getDisplayValueString() override {
        return to_string(static_cast<MagnitudeScale>(getValue()));
    }
};

/// @brief A parameter quantity for frequency scale selection.
struct FrequencyScaleParamQuantity : rack::ParamQuantity {
    /// @brief Return the value as a formatted string.
    inline std::string getDisplayValueString() final {
        return to_string(static_cast<FrequencyScale>(getValue()));
    }
};

/// @brief A parameter quantity for frequency smoothing selection.
struct FrequencySmoothingParamQuantity : rack::ParamQuantity {
    /// @brief Return the value as a formatted string.
    inline std::string getDisplayValueString() final {
        return to_string(static_cast<FrequencySmoothing>(getValue()));
    }
};

#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_PARAM_QUANTITY_HPP_
