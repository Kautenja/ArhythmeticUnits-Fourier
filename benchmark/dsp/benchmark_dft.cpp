// Small direct transforms with caller-owned output storage.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <cmath>
#include <complex>
#include <string>
#include <vector>
#include "catch.hpp"
#include "../fixtures.hpp"
#include "dsp/dft.hpp"

TEMPLATE_TEST_CASE("Direct transforms: one frame", "[dft]", float, double) {
    using T = TestType;
    // Quadratic reference transforms deliberately use smaller lengths.
    for (const size_t n : {32u, 128u}) {
        const auto input = BenchmarkFixtures::signal<T>(n);
        std::vector<std::complex<T>> output(n);
        std::vector<T> inverse(n);
        Fourier::dft(input.data(), output.data(), n);
        const auto spectrum = output;
        Fourier::idft(spectrum.data(), inverse.data(), n);
        REQUIRE(std::abs(inverse[7] - input[7]) < T(1e-4));
        for (const auto window : {Fourier::Window::Function::Boxcar,
                                  Fourier::Window::Function::Hann}) {
            BENCHMARK("DFT " + Fourier::Window::names()[static_cast<size_t>(window)]
                    + " N=" + std::to_string(n)) {
                Catch::Benchmark::keep_memory(input.data());
                Fourier::dft(input.data(), output.data(), n, window);
                Catch::Benchmark::keep_memory(output.data());
            };
        }
        BENCHMARK("IDFT N=" + std::to_string(n)) {
            Catch::Benchmark::keep_memory(spectrum.data());
            Fourier::idft(spectrum.data(), inverse.data(), n);
            Catch::Benchmark::keep_memory(inverse.data());
        };
    }
}
