// Four-lane spectrum-to-display mapping, isolated from FFT and rendering.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "catch.hpp"
#include "../fixtures.hpp"
#include "../../src/rack_extensions/spectrum_coordinates.hpp"

TEST_CASE("Spectrum coordinates: one four-lane frame", "[coordinates]") {
    for (const size_t n : {128u, 2048u, 16384u}) {
        Fourier::SpectrumCoordinates coordinates;
        coordinates.bins = n/2+1;
        const auto signal = BenchmarkFixtures::signal<float>(coordinates.bins);
        std::vector<rack::simd::float_4> magnitudes(coordinates.bins);
        std::vector<std::array<rack::math::Vec, 4>> output(coordinates.bins);
        for (size_t k = 0; k < coordinates.bins; ++k) {
            const float value = (0.01f + std::abs(signal[k])) * coordinates.bins;
            magnitudes[k] = rack::simd::float_4(value, value/2.f, value/4.f, value/8.f);
        }
        for (const auto frequency : {FrequencyScale::Linear, FrequencyScale::Logarithmic}) {
            for (const auto magnitude : {MagnitudeScale::Linear,
                    MagnitudeScale::Logarithmic60dB, MagnitudeScale::Logarithmic120dB}) {
                for (const bool cropped : {false, true}) {
                    coordinates.frequency_scale = frequency;
                    coordinates.magnitude_scale = magnitude;
                    coordinates.low_frequency = cropped ? 100.f : 0.f;
                    coordinates.high_frequency = cropped ? 10000.f : 24000.f;
                    coordinates.slope = cropped ? 4.5f : 0.f;
                    const std::string label = "N=" + std::to_string(n)
                        + " x=" + frequency_scale_names()[static_cast<size_t>(frequency)]
                        + " y=" + magnitude_scale_names()[static_cast<size_t>(magnitude)]
                        + (cropped ? " 100-10000Hz slope=4.5" : " 0-24000Hz slope=0");
                    BENCHMARK(label + " / 4 lanes") {
                        Catch::Benchmark::keep_memory(magnitudes.data());
                        Catch::Benchmark::deoptimize_value(coordinates);
                        for (size_t k = 0; k < coordinates.bins; ++k)
                            output[k] = coordinates.map(k, magnitudes[k]);
                        Catch::Benchmark::keep_memory(output.data());
                    };
                    REQUIRE(std::isfinite(output[7][0].y));
                    REQUIRE(output[7][0].y > output[7][3].y);
                    REQUIRE(output.front()[0].x < output.back()[0].x);
                    if (cropped) {
                        REQUIRE(output.front()[0].x < 0.f);
                        REQUIRE(output.back()[0].x > 1.f);
                    }
                }
            }
        }
    }
}
