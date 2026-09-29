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
#include <cstdlib>
#include <new>
#include "../../src/SpectrumAnalyzer.cpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

Plugin* plugin_instance = nullptr;

namespace {
thread_local bool track_allocations = false;
thread_local size_t allocation_count = 0;
}
void* operator new(size_t bytes) {
    if (track_allocations) ++allocation_count;
    if (void* value = std::malloc(bytes ? bytes : 1)) return value;
    throw std::bad_alloc();
}
void* operator new[](size_t bytes) { return ::operator new(bytes); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }

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
            const auto& snapshot = module.consume_display_spectrum();
            REQUIRE(snapshot.count == length / 2 + 1);
            std::array<std::vector<Vec>, 4> linear;
            for (size_t lane = 0; lane < 4; ++lane)
                linear[lane].assign(snapshot.points[lane].begin(),
                    snapshot.points[lane].begin() + snapshot.count);
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
                        Fourier::SpectrumCoordinates coordinates;
                        coordinates.bins = length / 2 + 1;
                        coordinates.sample_rate = rate;
                        coordinates.slope = slope;
                        coordinates.low_frequency = 100.f;
                        coordinates.high_frequency = 10000.f;
                        coordinates.frequency_scale = frequency;
                        coordinates.magnitude_scale = magnitude;
                        const float bins = length / 2.f + 1.f;
                        for (size_t bin = 0; bin < linear[0].size(); ++bin) {
                            const float normalized = bin / bins;
                            const float gain = Fourier::decibels2amplitude(slope *
                                std::log2(normalized * rate / 2000.f + std::numeric_limits<float>::epsilon()));
                            float expected_x = (normalized - 200.f / rate) / (19800.f / rate);
                            if (frequency == FrequencyScale::Logarithmic)
                                expected_x = std::copysign(std::sqrt(std::abs(expected_x)), expected_x);
                            simd::float_4 magnitudes;
                            for (size_t lane = 0; lane < 4; ++lane)
                                magnitudes.s[lane] = linear[lane][bin].y *
                                    Fourier::decibels2amplitude(12.f) * bins;
                            const auto points = coordinates.map(bin, magnitudes);
                            for (size_t lane = 0; lane < 4; ++lane) {
                                float expected_y = linear[lane][bin].y * gain;
                                if (magnitude != MagnitudeScale::Linear) {
                                    const float range = magnitude == MagnitudeScale::Logarithmic60dB ? 72.f : 132.f;
                                    expected_y = Fourier::amplitude2decibels(expected_y) / range + 1.f;
                                }
                                const Vec actual = points[lane];
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

TEST_CASE("Scheduled SIMD spectra agree with independent scalar lanes") {
    Fourier::SpectrumAnalysis<simd::float_4> vector(16384, 32768);
    std::array<Fourier::SpectrumAnalysis<float>, 4> scalar{{
        Fourier::SpectrumAnalysis<float>(16384, 32768),
        Fourier::SpectrumAnalysis<float>(16384, 32768),
        Fourier::SpectrumAnalysis<float>(16384, 32768),
        Fourier::SpectrumAnalysis<float>(16384, 32768)}};
    for (size_t n : {4u, 128u, 2048u, 16384u}) {
        for (float octave : {0.f, 1.f/3.f, 2.5f}) {
            Fourier::SpectrumSettings settings;
            settings.length = n;
            settings.hop = n/2+3;
            settings.window = Fourier::Window::Function::Flattop;
            settings.sample_rate = 96000.f;
            settings.alpha = 0.7f;
            settings.octave = octave;
            vector.reset();
            REQUIRE(vector.configure(settings));
            for (auto& lane : scalar) { lane.reset(); REQUIRE(lane.configure(settings)); }
            std::vector<simd::float_4> actual(n/2+1);
            std::array<std::vector<float>, 4> expected;
            for (auto& lane : expected) lane.resize(n/2+1);
            for (size_t i = 0; i < 5*settings.hop; ++i) {
                simd::float_4 input;
                for (size_t lane = 0; lane < 4; ++lane)
                    input.s[lane] = lane == 3 ? 0.f : std::sin((lane+1)*0.07f*i)+0.1f*lane;
                const bool ready = vector.process(input, [&](size_t k, simd::float_4 value) { actual[k] = value; });
                for (size_t lane = 0; lane < 4; ++lane)
                    REQUIRE(scalar[lane].process(input.s[lane],
                        [&](size_t k, float value) { expected[lane][k] = value; }) == ready);
                if (ready) {
                    for (size_t k = 0; k < actual.size(); ++k)
                        for (size_t lane = 0; lane < 4; ++lane) {
                            CAPTURE(n, octave, i, k, lane);
                            // Unnormalized sums scale with N. Rack fast-math may
                            // round scalar/SIMD products and magnitudes differently;
                            // compare per-sample amplitudes, including near-zero bins.
                            REQUIRE(actual[k].s[lane]/n == Approx(expected[lane][k]/n)
                                .margin(32*std::numeric_limits<float>::epsilon()));
                        }
                }
            }
        }
    }
}

TEST_CASE("Fourier publishes only complete curves and retains a held snapshot") {
    rack::Context context;
    rack::contextSet(&context);
    context.engine = new rack::engine::Engine;
    SpectrumAnalyzer module;
    module.set_window_length(128);
    module.set_hop_length(300);
    module.set_window_function(Fourier::Window::Function::Boxcar);
    module.set_slope(0.f);
    module.set_magnitude_scale(MagnitudeScale::Linear);
    module.is_ac_coupled = false;
    module.inputs[SpectrumAnalyzer::INPUT_SIGNAL].channels = 1;
    module.inputs[SpectrumAnalyzer::INPUT_SIGNAL].setVoltage(1.f);
    const auto hop = module.get_hop_length();
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = module.get_sample_rate();
    args.sampleTime = 1.f/args.sampleRate;
    REQUIRE(module.consume_display_spectrum().count == 0);
    for (size_t i = 0; i < hop-1; ++i) module.process(args);
    REQUIRE(module.consume_display_spectrum().count == 0);
    module.process(args);
    const auto& held = module.consume_display_spectrum();
    REQUIRE(held.count == 65);
    const auto copy = held.points;
    module.set_window_length(2048);
    module.set_window_function(Fourier::Window::Function::Hann);
    for (size_t i = 0; i < 20*hop; ++i) module.process(args);
    REQUIRE(held.count == 65);
    for (size_t lane = 0; lane < 4; ++lane)
        for (size_t i = 0; i < held.count; ++i) {
            CHECK(held.points[lane][i].x == copy[lane][i].x);
            CHECK(held.points[lane][i].y == copy[lane][i].y);
        }
    REQUIRE(module.consume_display_spectrum().count == 1025);
    module.onReset();
    REQUIRE(module.consume_display_spectrum().count == 0);
}

TEST_CASE("Fourier processing and live analysis controls do not allocate") {
    rack::Context context;
    rack::contextSet(&context);
    context.engine = new rack::engine::Engine;
    SpectrumAnalyzer module;
    module.set_hop_length(512);
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = module.get_sample_rate();
    args.sampleTime = 1.f/args.sampleRate;
    allocation_count = 0;
    track_allocations = true;
    for (size_t n : {128u, 16384u, 2048u}) {
        module.set_window_length(n);
        for (auto window : {Fourier::Window::Function::Boxcar, Fourier::Window::Function::Flattop}) {
            module.set_window_function(window);
            module.set_frequency_smoothing(window == Fourier::Window::Function::Boxcar
                ? FrequencySmoothing::None : FrequencySmoothing::_5_2);
            for (size_t i = 0; i < 3*module.get_hop_length(); ++i) module.process(args);
        }
    }
    track_allocations = false;
    CHECK(allocation_count == 0);
}
