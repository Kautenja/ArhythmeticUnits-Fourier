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

/// @brief A parameter quantity for window function selection.
struct WindowFunctionParamQuantity : ParamQuantity {
    /// @brief Return the value as a formatted string.
    inline std::string getDisplayValueString() final {
        return Math::Window::name(static_cast<Math::Window::Function>(getValue()));
    }
};

#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_PARAM_QUANTITY_HPP_
