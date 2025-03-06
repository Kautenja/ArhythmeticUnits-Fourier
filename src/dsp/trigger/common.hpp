// Common structures, methods, and data for detecting trigger events.
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

#ifndef DSP_TRIGGER_COMMON_HPP
#define DSP_TRIGGER_COMMON_HPP

/// @brief A collection of structures for detecting trigger events.
namespace Trigger {

/// @brief Directions for detecting triggers based on thresholds.
enum class Direction {
    /// Detect rising edges, i.e., increasing from negative to positive
    Rise = 0,
    /// Detect falling edges, i.e., decreasing from positive to negative
    Fall
};

}  // namespace Trigger

#endif  // DSP_TRIGGER_COMMON_HPP
