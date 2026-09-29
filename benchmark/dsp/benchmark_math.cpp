// Numeric, voltage, pitch and color helpers over observable input blocks.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <cmath>
#include <complex>
#include <numeric>
#include <string>
#include <vector>
#include "catch.hpp"
#include "../fixtures.hpp"
#include "dsp/color_map.hpp"
#include "dsp/eurorack.hpp"
#include "dsp/fft.hpp"
#include "dsp/math.hpp"
#include "dsp/western_scale.hpp"

TEMPLATE_TEST_CASE("Numeric helpers: 1024 values", "[math]", float, double) {
    using T = TestType;
    auto input = BenchmarkFixtures::signal<T>(1024);
    for (auto& value : input) value *= T(4);
    std::vector<T> output(input.size());
    BENCHMARK("clip [-1,1] / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::clip(x, T(-1), T(1)); });
    };
    BENCHMARK("sign / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::sgn(x); });
    };
    BENCHMARK("square / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::squared(x); });
    };
    BENCHMARK("cube / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::cubed(x); });
    };
    BENCHMARK("amplitude to dB / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::amplitude2decibels(x); });
    };
    BENCHMARK("dB to amplitude / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::decibels2amplitude(T(24)*x); });
    };
    BENCHMARK("volts from AC / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::Eurorack::fromAC(x); });
    };
    BENCHMARK("volts to AC / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::Eurorack::toAC(x); });
    };
    BENCHMARK("volts from DC / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::Eurorack::fromDC(x); });
    };
    BENCHMARK("volts to DC / 1024 values") {
        BenchmarkFixtures::map(input, output, [](T x) { return Fourier::Eurorack::toDC(x); });
    };
    std::vector<std::complex<T>> complex_input(input.size()), complex_output(input.size());
    for (size_t i = 0; i < input.size(); ++i)
        complex_input[i] = {input[i], input[(i+17)%input.size()]};
    BENCHMARK("complex multiply / 1024 values") {
        BenchmarkFixtures::map(complex_input, complex_output, [](std::complex<T> x) {
            return Fourier::complex_multiply(x, std::complex<T>(T(0.6), T(0.8)));
        });
    };
    REQUIRE(std::isfinite(complex_output[0].real()));
}

TEST_CASE("Modulo and coefficient interpolation: 1024 values", "[math]") {
    std::vector<int> indices(1024), output(1024);
    std::iota(indices.begin(), indices.end(), -512);
    BENCHMARK("signed modulo 127 / 1024 values") {
        BenchmarkFixtures::map(indices, output, [](int x) { return Fourier::mod(x, 127); });
    };
    auto positions = BenchmarkFixtures::signal<float>(1024);
    std::vector<std::complex<float>> coefficients(2048), interpolated(1024);
    for (size_t i = 0; i < coefficients.size(); ++i)
        coefficients[i] = {float(i%17), float(i%31)};
    for (auto& value : positions) value = 512.f + 256.f * value;
    BENCHMARK("complex coefficient interpolation / 1024 values") {
        Catch::Benchmark::keep_memory(coefficients.data());
        BenchmarkFixtures::map(positions, interpolated, [&](float x) {
            return Fourier::interpolate_coefficients(coefficients, x);
        });
    };
    REQUIRE(std::isfinite(interpolated[0].real()));
}

TEST_CASE("Pitch and colormaps: 1024 values", "[pitch][color]") {
    auto input = BenchmarkFixtures::signal<float>(1024);
    std::vector<Fourier::TunedNote> notes(input.size());
    std::vector<Fourier::ColorMap::Color> colors(input.size());
    auto frequencies = input;
    for (auto& value : frequencies) value = 440.f * std::pow(2.f, value * 4.f);
    BENCHMARK("frequency to note+octave+cents / 1024 values") {
        BenchmarkFixtures::map(frequencies, notes, [](float hz) { return Fourier::TunedNote(hz); });
    };
    REQUIRE(std::isfinite(notes[0].cents));
    std::vector<std::string> labels(input.size());
    BENCHMARK("note+tuning formatting incl allocation / 1024 notes") {
        BenchmarkFixtures::map(notes, labels, [](const Fourier::TunedNote& note) {
            return note.note_string() + " " + note.tuning_string();
        });
    };
    for (size_t i = 0; i < Fourier::ColorMap::names().size(); ++i) {
        const auto function = static_cast<Fourier::ColorMap::Function>(i);
        BENCHMARK(Fourier::ColorMap::names()[i] + " / 1024 colors") {
            BenchmarkFixtures::map(input, colors, [&](float value) {
                return Fourier::ColorMap::color_map(function, value);
            });
        };
        REQUIRE(colors[0].r >= 0.f);
        REQUIRE(colors[0].r <= 1.f);
    }
}
