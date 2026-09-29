// Regression checks for shared Fourier spectrum-coordinate calculations.
//
// Copyright 2026 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <array>
#include "../../src/SpectrumAnalyzer.cpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

Plugin* plugin_instance = nullptr;

TEST_CASE("Spectrum coordinates preserve per-channel magnitudes across settings") {
    rack::Context context;
    rack::contextSet(&context);
    context.engine = new rack::engine::Engine;
    // Destroy the module before the context destroys its engine.
    SpectrumAnalyzer module;
    for (const float rate : {44100.f, 96000.f}) {
        context.engine->setSampleRate(rate);
        module.onSampleRateChange();
        for (const size_t length : {128u, 2048u, 16384u}) {
            module.set_window_length(length);
            module.set_window_function(Fourier::Window::Function::Boxcar);
            module.set_slope(0.f);
            module.set_frequency_scale(FrequencyScale::Linear);
            module.set_magnitude_scale(MagnitudeScale::Linear);
            module.set_time_smoothing(0.f);
            module.set_frequency_smoothing(FrequencySmoothing::None);
            module.is_ac_coupled = false;
            rack::engine::Module::ProcessArgs args = {};
            args.sampleRate = rate;
            args.sampleTime = 1.f / rate;
            // Rack's engine normally marks inputs connected; setChannels()
            // intentionally leaves a disconnected input disconnected.
            for (int lane = 0; lane < 4; ++lane)
                module.inputs[SpectrumAnalyzer::INPUT_SIGNAL + lane].channels = 1;
            for (size_t sample = 0; sample < 2 * length + 8192; ++sample) {
                for (int lane = 0; lane < 4; ++lane) {
                    // Different DC and sinusoidal components expose lane swaps.
                    const float voltage = lane == 3 ? 0.f : (lane + 1) *
                        (0.2f + std::sin(2.f * M_PI * (lane + 3) * sample / length));
                    module.inputs[SpectrumAnalyzer::INPUT_SIGNAL + lane].setVoltage(voltage);
                }
                module.process(args);
            }
            const std::array<std::vector<Vec>, 4> linear = {{
                module.raster_coeffs[0], module.raster_coeffs[1],
                module.raster_coeffs[2], module.raster_coeffs[3]}};
            REQUIRE(linear[0].size() == length / 2 + 1);
            for (size_t lane = 0; lane < 3; ++lane) {
                const auto peak = std::max_element(linear[lane].begin(), linear[lane].end(),
                    [](const Vec& a, const Vec& b) { return a.y < b.y; });
                REQUIRE(static_cast<size_t>(peak - linear[lane].begin()) == lane + 3);
            }
            for (const Vec& point : linear[3]) REQUIRE(point.y == 0.f);
            for (const float slope : {-3.f, 0.f, 3.f}) {
                for (const auto frequency : {FrequencyScale::Linear, FrequencyScale::Logarithmic}) {
                    for (const auto magnitude : {MagnitudeScale::Linear,
                            MagnitudeScale::Logarithmic60dB, MagnitudeScale::Logarithmic120dB}) {
                        module.set_slope(slope);
                        module.set_frequency_scale(frequency);
                        module.set_magnitude_scale(magnitude);
                        module.set_low_frequency(100.f);
                        module.set_high_frequency(10000.f);
                        module.make_points();
                        const float bins = length / 2.f + 1.f;
                        for (size_t bin = 0; bin < linear[0].size(); ++bin) {
                            const float normalized = bin / bins;
                            const float gain = Fourier::decibels2amplitude(slope *
                                std::log2(normalized * rate / 2000.f + std::numeric_limits<float>::epsilon()));
                            float expected_x = (normalized - 200.f / rate) / (19800.f / rate);
                            if (frequency == FrequencyScale::Logarithmic)
                                expected_x = std::copysign(std::sqrt(std::abs(expected_x)), expected_x);
                            for (size_t lane = 0; lane < 4; ++lane) {
                                float expected_y = linear[lane][bin].y * gain;
                                if (magnitude != MagnitudeScale::Linear) {
                                    const float range = magnitude == MagnitudeScale::Logarithmic60dB ? 72.f : 132.f;
                                    expected_y = Fourier::amplitude2decibels(expected_y) / range + 1.f;
                                }
                                const Vec actual = module.raster_coeffs[lane][bin];
                                REQUIRE(actual.x == Approx(expected_x).margin(1e-6f));
                                REQUIRE(actual.y == Approx(expected_y).epsilon(1e-5f).margin(1e-6f));
                            }
                        }
                    }
                }
            }
        }
    }
}
