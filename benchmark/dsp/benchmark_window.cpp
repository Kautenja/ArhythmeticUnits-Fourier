// Window evaluation, cached application and cache replacement.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <cmath>
#include <numeric>
#include <string>
#include <vector>
#include "catch.hpp"
#include "../fixtures.hpp"
#include "dsp/window.hpp"

TEMPLATE_TEST_CASE("Windows: 2048 coefficients", "[window]", float, double) {
    using T = TestType;
    const size_t n = 2048;
    std::vector<T> positions(n), output(n);
    std::iota(positions.begin(), positions.end(), T(0));
    for (size_t index = 0; index < Fourier::Window::names().size(); ++index) {
        const auto function = static_cast<Fourier::Window::Function>(index);
        BENCHMARK("periodic " + Fourier::Window::names()[index] + " / 2048 coefficients") {
            BenchmarkFixtures::map(positions, output, [&](T i) {
                return Fourier::Window::window(function, i, T(n), false);
            });
        };
        REQUIRE(std::isfinite(output[n/2]));
        REQUIRE(output[n/2] > T(0));
    }
    BENCHMARK("exponential alpha=0.5 / 2048 coefficients") {
        BenchmarkFixtures::map(positions, output, [&](T i) {
            return Fourier::Window::exponential(i, T(n), false, T(0.5));
        });
    };
    BENCHMARK("Hann-Poisson alpha=0.5 / 2048 coefficients") {
        BenchmarkFixtures::map(positions, output, [&](T i) {
            return Fourier::Window::hannpoisson(i, T(n), false, T(0.5));
        });
    };
    BENCHMARK("Gaussian sigma=0.25 / 2048 coefficients") {
        BenchmarkFixtures::map(positions, output, [&](T i) {
            return Fourier::Window::gaussian(i, T(n), false, T(0.25));
        });
    };
    BENCHMARK("Tukey alpha=0.5 / 2048 coefficients") {
        BenchmarkFixtures::map(positions, output, [&](T i) {
            return Fourier::Window::tukey(i, T(n), false, T(0.5));
        });
    };
    BENCHMARK("Kaiser beta=8 / 2048 coefficients") {
        BenchmarkFixtures::map(positions, output, [&](T i) {
            return Fourier::Window::Kaiser::window(i, T(n), T(8));
        });
    };
}

TEST_CASE("Cached windows: application and forced rebuild", "[window][cache]") {
    for (const size_t n : {128u, 2048u, 16384u}) {
        const auto input = BenchmarkFixtures::signal<float>(n);
        std::vector<float> output(n);
        Fourier::Window::CachedWindow<float> window(Fourier::Window::Function::Hann,
            n, false, true);
        BENCHMARK("cached Hann multiply N=" + std::to_string(n)) {
            Catch::Benchmark::keep_memory(input.data());
            Catch::Benchmark::keep_memory(window.get_samples().data());
            for (size_t i = 0; i < n; ++i) output[i] = input[i] * window[i];
            Catch::Benchmark::keep_memory(output.data());
        };
        BENCHMARK("cache rebuild Hann/Blackman-Harris N=" + std::to_string(n)) {
            const auto function = window.get_function() == Fourier::Window::Function::Hann
                ? Fourier::Window::Function::BlackmanHarris : Fourier::Window::Function::Hann;
            window.set_window(function, n, false, true);
            Catch::Benchmark::keep_memory(window.get_samples().data());
        };
        REQUIRE(window.get_samples().size() == n);
        REQUIRE(window[n/2] > 1.f);
    }
}
