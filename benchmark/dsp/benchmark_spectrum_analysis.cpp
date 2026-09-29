// Steady-state and live cache rebuild costs of the production scalar pipeline.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <cmath>
#include <string>
#include <vector>
#include "catch.hpp"
#include "../fixtures.hpp"
#include "dsp/spectrum_analysis.hpp"

TEMPLATE_TEST_CASE("One-hop spectrum: time per hop", "[spectrum]", float, double) {
    using T = TestType;
    for (const size_t n : {128u, 2048u, 16384u}) {
        // H=257 exercises quotient/remainder scheduling; H=1024 matches Spectre.
        for (const size_t hop : {257u, 1024u}) {
            for (const bool smooth : {false, true}) {
                Fourier::SpectrumAnalysis<T> analysis(16384, 16384);
                Fourier::SpectrumSettings settings;
                settings.length = n;
                settings.hop = hop;
                settings.window = Fourier::Window::Function::Hann;
                settings.octave = smooth ? 1.f/3.f : 0.f;
                settings.alpha = smooth ? 0.8f : 0.f;
                REQUIRE(analysis.configure(settings));
                const auto input = BenchmarkFixtures::signal<T>(hop);
                std::vector<T> output(n/2+1);
                auto emit = [&](size_t bin, T magnitude) { output[bin] = magnitude; };
                auto process_hop = [&]() {
                    Catch::Benchmark::keep_memory(input.data());
                    size_t frames = 0;
                    for (const auto value : input) frames += analysis.process(value, emit);
                    Catch::Benchmark::keep_memory(output.data());
                    return frames;
                };
                // Fill retained input, complete cache preparation, settle EMA.
                for (size_t i = 0; i < n/hop + 64; ++i) REQUIRE(process_hop() == 1);
                REQUIRE(output[0] > T(0));
                const std::string label = " N=" + std::to_string(n)
                    + " H=" + std::to_string(hop)
                    + (smooth ? " octave=1/3 alpha=0.8" : " octave=0 alpha=0");
                BENCHMARK("steady Hann" + label) { return process_hop(); };
                BENCHMARK("live window+band rebuild" + label) {
                    // Alternation invalidates both caches on EVERY iteration,
                    // including Catch2 calibration. Construction stays outside.
                    const bool hann = settings.window == Fourier::Window::Function::Hann;
                    settings.window = hann ? Fourier::Window::Function::BlackmanHarris
                                           : Fourier::Window::Function::Hann;
                    settings.sample_rate = hann ? 96000.f : 48000.f;
                    analysis.configure(settings);
                    return process_hop();
                };
                REQUIRE(analysis.is_frame_start());
                for (const auto value : output) REQUIRE(std::isfinite(value));
            }
        }
    }
}
