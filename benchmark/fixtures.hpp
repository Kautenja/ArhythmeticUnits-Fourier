// Deterministic inputs and observable block outputs for DSP benchmarks.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ARHYTHMETIC_UNITS_FOURIER_BENCHMARK_FIXTURES_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_BENCHMARK_FIXTURES_HPP_

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "catch.hpp"

namespace BenchmarkFixtures {

/// @brief A periodic two-tone signal plus DC and seeded broadband noise.
/// @details Generation/allocation happens before timing. Use a private LCG for
/// identical inputs across standard libraries; do not benchmark random engines.
template<typename T>
std::vector<T> signal(size_t length) {
    std::vector<T> result(length);
    uint32_t state = 0x12345678u;
    for (size_t i = 0; i < length; ++i) {
        state = 1664525u * state + 1013904223u;
        const double phase = 2.0 * std::acos(-1.0) * i / length;
        const double noise = double(state >> 8) / 16777216.0 - 0.5;
        result[i] = T(0.2 + 0.5 * std::sin(7.0 * phase)
            + 0.25 * std::cos(31.0 * phase) + 0.05 * noise);
    }
    return result;
}

/// @brief Time a full block, including traversal and output stores.
/// @details Barriers make every input/output observable under -O3, without a
/// volatile store or barrier per element. Call only inside a benchmark body.
template<typename Input, typename Output, typename Operation>
void map(const std::vector<Input>& input, std::vector<Output>& output,
        Operation operation) {
    Catch::Benchmark::keep_memory(input.data());
    for (size_t i = 0; i < input.size(); ++i) output[i] = operation(input[i]);
    Catch::Benchmark::keep_memory(output.data());
}

}  // namespace BenchmarkFixtures
#endif  // ARHYTHMETIC_UNITS_FOURIER_BENCHMARK_FIXTURES_HPP_
