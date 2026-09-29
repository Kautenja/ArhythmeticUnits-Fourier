// UI-only Spectre intensity conversion and validation.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SPECTROGRAM_INTENSITY_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SPECTROGRAM_INTENSITY_HPP_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include "../dsp/fft.hpp"

namespace Fourier {

/// @brief UI-only mapping of unnormalized spectral magnitudes to palette positions.
struct SpectrogramIntensity {
    enum class Scale { Decibels, LegacyLinear };

    /// @brief IEEE-754 classification survives unsafe floating-point optimization flags.
    static uint32_t magnitude_bits(float value) {
        static_assert(sizeof(float) == sizeof(uint32_t), "32-bit floats required");
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        return bits & 0x7fffffffU;
    }
    static bool finite(float value) { return magnitude_bits(value) < 0x7f800000U; }
    static bool invalid(float value) { return magnitude_bits(value) > 0x7f800000U; }

    /// @brief Default non-finite controls, then clamp without changing other knobs.
    static float endpoint(float value, bool ceiling) {
        if (!finite(value)) return ceiling ? 0.f : -90.f;
        return ceiling ? std::max(0.f, std::min(24.f, value)) :
            std::max(-120.f, std::min(-1.f, value));
    }

    /// @brief Spectral amplitude reference N/2; no DC/Nyquist endpoint doubling.
    static float decibels(float magnitude, float divisor) {
        const auto bits = magnitude_bits(magnitude);
        if (bits > 0x7f800000U) return std::numeric_limits<float>::quiet_NaN();
        if (bits == 0x7f800000U) return std::numeric_limits<float>::infinity();
        if (bits == 0) return -std::numeric_limits<float>::infinity();
        return 20.f * (std::log10(std::abs(magnitude)) - std::log10(divisor));
    }

    /// @brief Physical-frequency weighting in dB, explicitly unweighted at DC.
    static float slope_db(float frequency, float slope) {
        return frequency > 0.f ? slope * std::log2(frequency / 1000.f) : 0.f;
    }

    /// @brief Sample linear magnitudes before dB conversion; integer bins avoid
    /// multiplying an unused non-finite neighbor by zero. History stores real magnitudes.
    static float interpolate(const DFTCoefficients& column, float bin) {
        const size_t lower = static_cast<size_t>(bin);
        const float fraction = bin - lower;
        const float first = column[lower].real();
        if (fraction == 0.f) return first;
        const float second = column[lower + 1].real();
        if (invalid(first) || invalid(second)) return std::numeric_limits<float>::quiet_NaN();
        if (!finite(first) || !finite(second)) return std::numeric_limits<float>::infinity();
        return (1.f - fraction) * first + fraction * second;
    }

    /// @brief Fractional bin sampled by an image row, on either frequency scale.
    static float row_bin(float row, float height, bool logarithmic) {
        return logarithmic ? row * row / height : row;
    }

    /// @brief Unclamped weighted pixel level for both rendering and inspection.
    static float color_db(const DFTCoefficients& column, float bin,
            float sample_rate, float length, float slope) {
        const float raw = decibels(interpolate(column, bin), length / 2.f);
        return finite(raw) ? raw + slope_db(bin * sample_rate / length, slope) : raw;
    }

    /// @brief Always return a finite coordinate safe for every palette table.
    static float position(float db, float floor, float ceiling) {
        if (invalid(db)) return 0.f;
        floor = endpoint(floor, false);
        ceiling = endpoint(ceiling, true);
        if (db <= floor) return 0.f;
        if (db >= ceiling) return 1.f;
        return (db - floor) / (ceiling - floor);
    }

    /// @brief One decimal place, with explicit silence and invalid-data readouts.
    static std::string format(float db) {
        if (invalid(db)) return "--";
        if (!finite(db)) return db < 0.f ? "-inf" : "+inf";
        std::ostringstream text;
        text << std::fixed << std::setprecision(1) << db;
        return text.str();
    }
};

}  // namespace Fourier
#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SPECTROGRAM_INTENSITY_HPP_
