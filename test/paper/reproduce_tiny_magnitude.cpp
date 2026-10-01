// Untimed reproducer for the spec-014 long-decay numerical gate failure.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <complex>
#include <iostream>
#include <rack.hpp>
#include "../../src/dsp/math.hpp"

int main() {
    // Volatile inputs prevent compile-time folding around Rack's FPU policy.
    volatile float input = 1e-25f;
    const auto previous = rack::system::getFpuFlags();
    rack::system::resetFpuFlags();
    const rack::simd::float_4 value(input);
    const auto magnitude = rack::simd::abs(std::complex<rack::simd::float_4>(value, value));
    const auto repaired = Fourier::complex_magnitude(std::complex<rack::simd::float_4>(value, value));
    const double reference = std::hypot(double(input), double(input));
    std::cout.precision(17);
    std::cout << "{\"kind\":\"untimed-magnitude-reproducer\",\"input\":" << input
        << ",\"fpu_control\":" << rack::system::getFpuFlags()
        << ",\"legacy_actual\":" << magnitude[0] << ",\"repaired_actual\":" << repaired[0] << ",\"reference\":" << reference << "}\n";
    rack::system::setFpuFlags(previous);
    // Keep the failing Rack calculation visible while gating the first-party repair.
    return std::abs(double(repaired[0])-reference) <= 3e-4*reference ? 0 : 1;
}
