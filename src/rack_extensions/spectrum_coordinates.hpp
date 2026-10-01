// Frame-latched mapping from spectrum magnitudes to display coordinates.
//
// Copyright 2026 Arhythmetic Units
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SPECTRUM_COORDINATES_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SPECTRUM_COORDINATES_HPP_

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <rack.hpp>
#include "../dsp/math.hpp"
#include "../structs.hpp"

namespace Fourier {

/// @brief A frame's display settings, shared across four spectrum lanes.
/// @details Preserves the module's K=N/2+1 amplitude and frequency convention.
/// Capture once per analysis frame; map one bin inside the scheduled output
/// callback. Results may lie outside [0,1] when bounds exclude a bin.
struct SpectrumCoordinates {
    size_t bins = 0;
    float sample_rate = 48000.f;
    float low_frequency = 0.f;
    float high_frequency = 24000.f;
    float slope = 0.f;
    FrequencyScale frequency_scale = FrequencyScale::Linear;
    MagnitudeScale magnitude_scale = MagnitudeScale::Linear;

    /// @brief Frequency coordinate and slope gain for one bin.
    rack::math::Vec geometry(size_t bin) const {
        const float nyquist = sample_rate / 2.f;
        float x = bin / static_cast<float>(bins);
        const float gain = slope == 0.f ? 1.f : decibels2amplitude(slope * std::log2(
            x * nyquist / 1000.f + std::numeric_limits<float>::epsilon()));
        const float extent = (high_frequency-low_frequency) / nyquist;
        x = (x-low_frequency/nyquist) / (extent == 0.f ? 1.f : extent);
        if (frequency_scale == FrequencyScale::Logarithmic)
            x = std::copysign(std::sqrt(std::abs(x)), x);
        return rack::math::Vec(x, gain);
    }

    /// @brief Map magnitudes using the exact geometry of the same settings.
    std::array<rack::math::Vec, 4> map_geometry(rack::math::Vec geometry,
            rack::simd::float_4 magnitude) const {
        const float maximum = decibels2amplitude(12.f) * bins;
        std::array<rack::math::Vec, 4> points;
        for (size_t lane = 0; lane < points.size(); ++lane) {
            float y = magnitude.s[lane] / maximum * geometry.y;
            if (magnitude_scale != MagnitudeScale::Linear) {
                const float range = magnitude_scale == MagnitudeScale::Logarithmic60dB ? 72.f : 132.f;
                y = amplitude2decibels(y) / range + 1.f;
            }
            points[lane] = rack::math::Vec(geometry.x, y);
        }
        return points;
    }

    /// @brief Map one four-lane magnitude value to four plot coordinates.
    std::array<rack::math::Vec, 4> map(size_t bin, rack::simd::float_4 magnitude) const {
        return map_geometry(geometry(bin), magnitude);
    }
};

/// @brief Reuse exact geometry; rebuild at most one entry per scheduled output.
/// @details Engine-owned. Configuration invalidates in constant time. A prefix
/// tracks initialized entries so cancellation and out-of-order calls cannot
/// expose stale geometry. Sequential output completes the cache in one frame.
template<size_t Capacity>
class CachedSpectrumCoordinates {
    SpectrumCoordinates settings;
    std::array<rack::math::Vec, Capacity> entries;
    size_t valid = 0;

 public:
    /// @brief Latch frame settings without allocation or an all-bin rebuild.
    void configure(const SpectrumCoordinates& value) {
        if (settings.bins != value.bins || settings.sample_rate != value.sample_rate
            || settings.low_frequency != value.low_frequency
            || settings.high_frequency != value.high_frequency
            || settings.slope != value.slope || settings.frequency_scale != value.frequency_scale)
            valid = 0;
        settings = value;
    }

    /// @brief Map a bin below Capacity, preserving amplitude arithmetic.
    std::array<rack::math::Vec, 4> map(size_t bin, rack::simd::float_4 magnitude) {
        if (bin >= valid) {
            entries[bin] = settings.geometry(bin);
            if (bin == valid) ++valid;
        }
        return settings.map_geometry(entries[bin], magnitude);
    }
};

}  // namespace Fourier
#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SPECTRUM_COORDINATES_HPP_
