// Direct scalar/SIMD agreement checks using Rack's actual vector type.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <cmath>
#include <limits>
#include <simd/Vector.hpp>
#include "../../src/dsp/dc_blocker.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("DC blocker SIMD lanes agree with independent scalar filters") {
    using rack::simd::float_4;
    Fourier::DCBlocker<float_4> vector;
    std::array<Fourier::DCBlocker<float>, 4> scalar;
    for (int phase = 0; phase < 3; ++phase) {
        CAPTURE(phase);
        if (phase > 0) {
            // Different normalized widths catch accidental lane broadcasting.
            const float_4 widths(10.f, 750.f, 3000.f, 12000.f);
            const float_4 rates(44100.f, 48000.f, 96000.f, 48000.f);
            vector.setTransitionWidth(widths, rates);
            for (int lane = 0; lane < 4; ++lane) {
                CAPTURE(lane);
                scalar[lane].setTransitionWidth(widths[lane], rates[lane]);
                // Recovering Hz subtracts a near-unity pole. Optimized scalar
                // algebra can cancel that subtraction; vector rounding remains.
                REQUIRE(vector.getTransitionWidth(rates)[lane]
                    == Approx(scalar[lane].getTransitionWidth(rates[lane]))
                        .epsilon(0).margin(rates[lane] * std::numeric_limits<float>::epsilon()));
            }
        }
        if (phase == 2) {
            vector.reset();
            for (int lane = 0; lane < 4; ++lane) {
                scalar[lane].reset();
                REQUIRE(vector.getValue()[lane] == 0.f);
            }
        }
        for (int n = 0; n < 4096; ++n) {
            // Impulse, step, Nyquist, and a changing signal with DC offset.
            // Lane zero stays silent after the impulse to expose cross-talk.
            const float_4 input(n == 0 ? 1.f : 0.f, -2.f,
                                n % 2 ? -0.5f : 0.5f,
                                0.3f + 0.7f * std::sin(0.17f * n));
            const float_4 actual = vector.process(input);
            for (int lane = 0; lane < 4; ++lane) {
                CAPTURE(n, lane);
                const float expected = scalar[lane].process(input[lane]);
                // Allow a few float ULPs for optimized scalar/SIMD arithmetic.
                REQUIRE(actual[lane] == Approx(expected).epsilon(2e-6f).margin(2e-6f));
                REQUIRE(vector.getValue()[lane] == actual[lane]);
            }
        }
    }
}
