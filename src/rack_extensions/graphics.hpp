// Extensions for graphical functions in VCV Rack.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_GRAPHICS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_GRAPHICS_HPP_

#include "rack.hpp"

/// @brief Compute control points for a Catmull-Rom segment from p1 to p2
/// with tangents based on neighbors p0 and p3.
/// @param points The points to interpolate
/// @param control The control points to populate.
/// @param tension Tension parameter for Catmull–Rom (typically 0.0 ~ 0.5)
static inline void catmull_rom_to_bezier(Vec points[4], Vec control[2], float tension = 0.5f) {
    // One common approach is to set control points based on tangents
    // at p1 and p2. The standard formula can vary, but typically:
    float t = (1.0f - tension) / 6.0f;
    control[0].x = points[1].x + (points[2].x - points[0].x) * t;
    control[0].y = points[1].y + (points[2].y - points[0].y) * t;
    control[1].x = points[2].x - (points[3].x - points[1].x) * t;
    control[1].y = points[2].y - (points[3].y - points[1].y) * t;
}

#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_GRAPHICS_HPP_
