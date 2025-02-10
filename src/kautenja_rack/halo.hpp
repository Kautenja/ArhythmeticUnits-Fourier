// Extensions to the VCV Rack helper functions.
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

#ifndef KAUTENJA_RACK_HALO_HPP
#define KAUTENJA_RACK_HALO_HPP

#include "rack.hpp"

// ---------------------------------------------------------------------------
// MARK: Rogan P2 Knob
// ---------------------------------------------------------------------------

/// The number of LEDs in a halo around a Rogan P2 knob.
static const std::size_t NUM_HALO_POINTS_ROGAN_P2 = 15;

/// @brief Return the halo point for the given halo coordinate and LED index.
///
/// @param x the x pixel of the halo group
/// @param y the y pixel of the halo group
/// @param i the index of the halo in the LED ring
/// @returns the coordinates of LED i for the halo of LEDs at origin (x, y)
/// @details
/// \f$(x, y)\f$ describes the top-left-most pixel of the square region that
/// encapsulates the halo around a Rogan P2 knob.
///
inline rack::Vec halo_point_rogan_p2(const float& x, const float& y, const unsigned& i) {
    static constexpr struct { float x; float y; } points[15] = {
        {1.9,  38.7},
        {0,    30.9},
        {0.2,  23  },
        {2.5,  15.3},
        {7,    8.7 },
        {13,   3.7 },
        {20.4, 0.75},
        {28,   0   },
        {35.9, 1.1 },
        {43.2, 4.5 },
        {49.2, 9.8 },
        {53.6, 16.6},
        {55.8, 24.4},
        {55.6, 32.3},
        {53.1, 40  }
    };
    return rack::Vec(x + points[i].x, y + points[i].y);
}

// ---------------------------------------------------------------------------
// MARK: PJ301M Port
// ---------------------------------------------------------------------------

/// The number of LEDs in a halo around a PJ301M port.
static const std::size_t NUM_HALO_POINTS_PJ301M_PORT = 11;

/// @brief Return the halo point for the given halo coordinate and LED index.
///
/// @param x the x coordinate of the port's top left corner.
/// @param y the y coordinate of the port's top left corner.
/// @param i the index of the halo in the LED ring.
/// @returns the coordinates of LED i for the halo of LEDs at origin (x, y)
/// @details
/// \f$(x, y)\f$ describes the top-left-most pixel of the square region that
/// encapsulates the halo around a PJ301M port.
///
inline rack::Vec halo_point_pj301m_port(const float& x, const float& y, const unsigned& i) {
    // base = np.array([[475, 30]])
    // points = np.array([
    //     [472, 52],
    //     [468, 45],
    //     [467, 37],
    //     [470, 29],
    //     [476, 23],
    //     [484, 21],
    //     [492, 23],
    //     [498, 29],
    //     [501, 37],
    //     [500, 45],
    //     [496, 52],
    // ])
    // print(str((points - base).tolist()).replace('[', '{').replace(']', '}'))
    static constexpr struct { float x; float y; } points[NUM_HALO_POINTS_PJ301M_PORT] = {
        {-3, 22},
        {-7, 15},
        {-8, 7},
        {-5, -1},
        {1, -7},
        {9, -9},
        {17, -7},
        {23, -1},
        {26, 7},
        {25, 15},
        {21, 22}
    };
    return rack::Vec(x + points[i].x, y + points[i].y);
}

#endif  // KAUTENJA_RACK_HALO_HPP
