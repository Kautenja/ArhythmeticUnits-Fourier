// Extensions to the VCV Rack components library.
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

#ifndef COMPONENTS_COMPONENTS_HPP
#define COMPONENTS_COMPONENTS_HPP

#include "rack.hpp"

/// @brief A modified Befaco slider with horizontal rendering and control.
struct BefacoSlidePotHorizontal : app::SvgSlider {
    BefacoSlidePotHorizontal() {
        math::Vec margin = math::Vec(3.5, 3.5);
        minHandlePos = math::Vec(-2, -1).plus(margin);
        maxHandlePos = math::Vec(87, -1).plus(margin);
        setBackgroundSvg(Svg::load(asset::plugin(plugin_instance, "res/ComponentLibrary/BefacoSlidePotHorizontal.svg")));
        setHandleSvg(Svg::load(asset::plugin(plugin_instance, "res/ComponentLibrary/BefacoSlidePotHandleHorizontal.svg")));
        background->box.pos = margin;
        box.size = background->box.size.plus(margin.mult(2));
        horizontal = true;
    }
};

#endif  // COMPONENTS_COMPONENTS_HPP
