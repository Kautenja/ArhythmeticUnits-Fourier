// Streaming filter, delay-buffer and trigger block costs.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "catch_amalgamated.hpp"
#include "../fixtures.hpp"
#include "dsp/circular_buffer.hpp"
#include "dsp/dc_blocker.hpp"
#include "dsp/threshold_trigger.hpp"
#include "dsp/trigger_divider.hpp"

TEMPLATE_TEST_CASE("DC blocker: 1024 samples, 10 Hz at 48 kHz", "[filter]", float, double) {
    using T = TestType;
    const auto input = BenchmarkFixtures::signal<T>(1024);
    std::vector<T> output(input.size());
    Fourier::DCBlocker<T> filter;
    filter.setTransitionWidth(T(10), T(48000));
    for (size_t i = 0; i < 65536; ++i) filter.process(input[i%input.size()]);
    BENCHMARK("DC blocker process / 1024 samples") {
        BenchmarkFixtures::map(input, output, [&](T value) { return filter.process(value); });
    };
    REQUIRE(std::isfinite(filter.getValue()));
    REQUIRE(std::abs(filter.getValue()) > T(0.001));
}

TEST_CASE("Circular buffers: 1024 insert+read operations", "[buffer]") {
    const auto input = BenchmarkFixtures::signal<float>(1024);
    std::vector<float> output(input.size());
    for (const size_t capacity : {128u, 2048u, 16384u}) {
        Fourier::CircularBuffer<float> circular(capacity);
        Fourier::ContiguousCircularBuffer<float> contiguous(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            circular.insert(input[i%input.size()]);
            contiguous.insert(input[i%input.size()]);
        }
        BENCHMARK("circular insert+delay=63 / 1024 capacity=" + std::to_string(capacity)) {
            BenchmarkFixtures::map(input, output, [&](float value) {
                circular.insert(value);
                return circular.at(-63);
            });
        };
        BENCHMARK("contiguous insert+oldest / 1024 capacity=" + std::to_string(capacity)) {
            BenchmarkFixtures::map(input, output, [&](float value) {
                contiguous.insert(value);
                return contiguous.contiguous()[0];
            });
        };
        REQUIRE(circular.at(0) == input.back());
        REQUIRE(contiguous.contiguous()[capacity-1] == input.back());
        std::vector<float> frame(capacity);
        BENCHMARK("contiguous frame copy / capacity=" + std::to_string(capacity)) {
            Catch::Benchmark::keep_memory(contiguous.contiguous());
            std::copy(contiguous.contiguous(), contiguous.contiguous()+capacity, frame.begin());
            Catch::Benchmark::keep_memory(frame.data());
        };
    }
}

TEST_CASE("Trigger streams: 1024 samples or ticks", "[trigger]") {
    auto input = BenchmarkFixtures::signal<float>(1024);
    for (auto& value : input) value = 2.f * value + 0.25f;
    std::vector<unsigned char> output(input.size());
    Fourier::ThresholdTrigger<float> threshold;
    BENCHMARK("threshold crossings / 1024 samples") {
        BenchmarkFixtures::map(input, output, [&](float value) { return threshold.process(value); });
    };
    REQUIRE(std::count(output.begin(), output.end(), 1) > 0);
    for (const unsigned division : {3u, 512u}) {
        Fourier::TriggerDivider divider;
        divider.setDivision(division);
        BENCHMARK("divider process+gate / 1024 ticks division=" + std::to_string(division)) {
            for (size_t i = 0; i < output.size(); ++i) {
                const bool trigger = divider.process();
                output[i] = static_cast<unsigned char>(trigger + 2 * divider.getGate());
            }
            Catch::Benchmark::keep_memory(output.data());
        };
        REQUIRE(divider.getClock() < division);
    }
}
