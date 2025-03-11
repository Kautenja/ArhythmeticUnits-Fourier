// Structs for the plugin.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_STRUCTS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_STRUCTS_HPP_

#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// MARK: Frequency Scale
// ---------------------------------------------------------------------------

/// @brief The options for frequency scales on the display.
enum class FrequencyScale {
    /// Linear frequency rendering along a fixed offset.
    Linear = 0,
    /// Logarithmic frequency rendering along base 10 harmonics.
    Logarithmic,
    /// The total number of frequency smoothing options
    NumOptions
};

static const std::vector<std::string>& frequency_scale_names() {
    static const std::vector<std::string> names = {
        "Linear",
        "Logarithmic"
    };
    return names;
}

// ---------------------------------------------------------------------------
// MARK: Magnitude Scale
// ---------------------------------------------------------------------------

/// @brief The options for magnitude scales on the display.
enum class MagnitudeScale {
    /// Linear magnitude rendering from 0% to 400%, i.e. -inf to 12dB.
    Linear = 0,
    /// Logarithmic magnitude rendering from -60dB to 12dB.
    Logarithmic60dB,
    /// Logarithmic magnitude rendering from -120dB to 12dB.
    Logarithmic120dB,
    /// The total number of frequency smoothing options
    NumOptions
};

static const std::vector<std::string>& magnitude_scale_names() {
    static const std::vector<std::string> names = {
        "Linear",
        "Log 60dB",
        "Log 120dB"
    };
    return names;
}

// ---------------------------------------------------------------------------
// MARK: Frequency Smoothing
// ---------------------------------------------------------------------------

/// @brief The options for magnitude smoothing over frequencies.
/// @details
/// Options are listed in fractional octave increments following the format
/// `_<numerator>_<denominator>`. For instance, 1/48th octave is `_1_48`,
/// where as 1 octave would be `_1_1`. The null frequency smoothing option is
/// represented by the `None` symbol.
enum class FrequencySmoothing {
    /// No frequency smoothing.
    None = 0,
    /// 1/48th octave.
    _1_48,
    /// 1/24th octave.
    _1_24,
    /// 1/24th octave.
    _1_12,
    /// 1/9th octave.
    _1_9,
    /// 1/6th octave.
    _1_6,
    /// 1/5th octave.
    _1_5,
    /// 1/4 octave.
    _1_4,
    /// 1/3 octave.
    _1_3,
    /// 1/2 octave.
    _1_2,
    /// 2/3 octave.
    _2_3,
    /// 3/4 octave.
    _3_4,
    /// 1 octave.
    _1_1,
    /// 1.5 octaves.
    _3_2,
    /// 2 octaves.
    _2_1,
    /// 2.5 octaves.
    _5_2,
    /// The total number of frequency smoothing options
    NumOptions
};

/// @brief Convert the given frequency smoothing option to a string.
/// @param value The frequency smoothing value to serialize to a string.
/// @returns The string representation of the given frequency smoothing value.
inline const char* to_string(const FrequencySmoothing& value) {
    switch (value) {
    case FrequencySmoothing::None:  return "None";
    case FrequencySmoothing::_1_48: return "1/48 oct";
    case FrequencySmoothing::_1_24: return "1/24 oct";
    case FrequencySmoothing::_1_12: return "1/12 oct";
    case FrequencySmoothing::_1_9:  return "1/9 oct";
    case FrequencySmoothing::_1_6:  return "1/6 oct";
    case FrequencySmoothing::_1_5:  return "1/5 oct";
    case FrequencySmoothing::_1_4:  return "1/4 oct";
    case FrequencySmoothing::_1_3:  return "1/3 oct";
    case FrequencySmoothing::_1_2:  return "1/2 oct";
    case FrequencySmoothing::_2_3:  return "2/3 oct";
    case FrequencySmoothing::_3_4:  return "3/4 oct";
    case FrequencySmoothing::_1_1:  return "1 oct";
    case FrequencySmoothing::_3_2:  return "1.5 oct";
    case FrequencySmoothing::_2_1:  return "2 oct";
    case FrequencySmoothing::_5_2:  return "2.5 oct";
    default: return "";
    }
}

/// @brief Convert the given frequency smoothing option to its float value.
/// @param value The frequency smoothing value to lookup the value of.
/// @returns The value of the given frequency smoothing value.
inline float to_float(const FrequencySmoothing& value) {
    switch (value) {
    case FrequencySmoothing::_1_48: return 1.f/48.f;
    case FrequencySmoothing::_1_24: return 1.f/24.f;
    case FrequencySmoothing::_1_12: return 1.f/12.f;
    case FrequencySmoothing::_1_9:  return 1.f/9.f;
    case FrequencySmoothing::_1_6:  return 1.f/6.f;
    case FrequencySmoothing::_1_5:  return 1.f/5.f;
    case FrequencySmoothing::_1_4:  return 1.f/4.f;
    case FrequencySmoothing::_1_3:  return 1.f/3.f;
    case FrequencySmoothing::_1_2:  return 1.f/2.f;
    case FrequencySmoothing::_2_3:  return 2.f/3.f;
    case FrequencySmoothing::_3_4:  return 3.f/4.f;
    case FrequencySmoothing::_1_1:  return 1.f;
    case FrequencySmoothing::_3_2:  return 1.5f;
    case FrequencySmoothing::_2_1:  return 2.f;
    case FrequencySmoothing::_5_2:  return 2.5f;
    default: return 0.f;
    }
}

#endif  // ARHYTHMETIC_UNITS_FOURIER_STRUCTS_HPP_
