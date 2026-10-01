// Direct response and lifecycle contracts for the DC blocker.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <limits>
#include "dsp/dc_blocker.hpp"
#include "catch_amalgamated.hpp"

TEMPLATE_TEST_CASE("DC blocker starts silent with the default transition width",
                  "[dc-blocker]", float, double) {
    Fourier::DCBlocker<TestType> filter;
    REQUIRE(filter.getValue() == TestType(0));
    for (int n = 0; n < 32; ++n) REQUIRE(filter.process(0) == TestType(0));
    for (const TestType rate : {TestType(44100), TestType(48000), TestType(96000)}) {
        CAPTURE(rate);
        const double expected = double(rate) * (1.0 - double(TestType(0.999))) / 2;
        REQUIRE(filter.getTransitionWidth(rate) == Catch::Approx(expected).epsilon(
            4 * std::numeric_limits<TestType>::epsilon()));
    }
}

TEMPLATE_TEST_CASE("DC blocker impulse and step responses follow geometric decay",
                  "[dc-blocker]", float, double) {
    // These configured poles are exactly representable; -1 selects the default.
    const int width = GENERATE(-1, 750, 3000, 12000);
    Fourier::DCBlocker<TestType> filter;
    if (width >= 0) filter.setTransitionWidth(TestType(width), TestType(48000));
    const double p = width < 0 ? double(TestType(0.999)) : 1.0 - width / 24000.0;
    const double gain = (1 + p) / 2;
    const double tolerance = 64 * std::numeric_limits<TestType>::epsilon();
    SECTION("unit impulse") {
        // H(z) = g(1-z^-1)/(1-p*z^-1): h[0]=g, h[n]=g(p-1)p^(n-1).
        for (int n = 0; n < 256; ++n) {
            CAPTURE(width, n);
            const double expected = n == 0 ? gain : gain * (p - 1) * std::pow(p, n - 1);
            const TestType actual = filter.process(n == 0 ? 1 : 0);
            REQUIRE(actual == Catch::Approx(expected).epsilon(tolerance).margin(tolerance));
            REQUIRE(filter.getValue() == actual);
        }
    }
    SECTION("unit step") {
        // Summing the impulse response gives s[n] = g*p^n.
        for (int n = 0; n < 256; ++n) {
            CAPTURE(width, n);
            REQUIRE(filter.process(1) == Catch::Approx(gain * std::pow(p, n))
                .epsilon(tolerance).margin(tolerance));
        }
    }
}

TEMPLATE_TEST_CASE("DC blocker rejects positive and negative constant inputs",
                  "[dc-blocker]", float, double) {
    for (const int width : {-1, 750, 12000}) {
        for (const TestType dc : {TestType(-3), TestType(0.25), TestType(2)}) {
            Fourier::DCBlocker<TestType> filter;
            if (width >= 0) filter.setTransitionWidth(TestType(width), TestType(48000));
            // Even the slowest pole, 0.999, decays below 6e-15 in 32768 samples.
            for (int n = 0; n < 32768; ++n) filter.process(dc);
            for (int n = 0; n < 32; ++n) {
                CAPTURE(width, dc, n);
                REQUIRE(std::abs(filter.process(dc)) < TestType(1e-10));
            }
        }
    }
}

TEMPLATE_TEST_CASE("DC blocker has unity steady-state Nyquist gain",
                  "[dc-blocker]", float, double) {
    for (const int width : {-1, 750, 3000, 12000}) {
        Fourier::DCBlocker<TestType> filter;
        if (width >= 0) filter.setTransitionWidth(TestType(width), TestType(48000));
        const double p = width < 0 ? double(TestType(0.999)) : 1.0 - width / 24000.0;
        // Bound accumulated rounding by the geometric sum 1/(1-p).
        // This remains below the error from omitting Nyquist normalization.
        const double tolerance = 2 * std::numeric_limits<TestType>::epsilon() / (1 - p);
        for (int n = 0; n < 32800; ++n) {
            const TestType input = n % 2 ? -1 : 1;
            const TestType actual = filter.process(input);
            if (n >= 32768) {
                CAPTURE(width, n);
                REQUIRE(actual == Catch::Approx(input).epsilon(0).margin(tolerance));
            }
        }
    }
}

TEMPLATE_TEST_CASE("DC blocker transition width controls the response at each sample rate",
                  "[dc-blocker]", float, double) {
    for (const TestType rate : {TestType(44100), TestType(48000), TestType(96000)}) {
        for (const TestType width : {TestType(10), TestType(100), TestType(1000)}) {
            CAPTURE(rate, width);
            Fourier::DCBlocker<TestType> filter;
            filter.setTransitionWidth(width, rate);
            const double epsilon = std::numeric_limits<TestType>::epsilon();
            // Width recovery subtracts a near-unity pole, so use an Hz bound.
            REQUIRE(filter.getTransitionWidth(rate) == Catch::Approx(width)
                .epsilon(0).margin(double(rate) * epsilon));
            REQUIRE(filter.getTransitionWidth(2 * rate) == Catch::Approx(2 * width)
                .epsilon(0).margin(2 * double(rate) * epsilon));
            const double p = 1 - 2 * double(width) / double(rate);
            const double gain = (1 + p) / 2;
            for (int n = 0; n < 64; ++n) {
                CAPTURE(n);
                REQUIRE(filter.process(1) == Catch::Approx(gain * std::pow(p, n))
                    .epsilon(0).margin(64 * epsilon));
            }
        }
    }
}

TEMPLATE_TEST_CASE("DC blocker reset clears both delays and retains configuration",
                  "[dc-blocker]", float, double) {
    Fourier::DCBlocker<TestType> reused;
    Fourier::DCBlocker<TestType> fresh;
    reused.setTransitionWidth(3000, 48000);
    fresh.setTransitionWidth(3000, 48000);
    reused.process(2);
    reused.process(-3);
    REQUIRE(reused.getValue() != TestType(0));
    reused.reset();
    REQUIRE(reused.getValue() == TestType(0));
    REQUIRE(reused.getTransitionWidth(48000) == TestType(3000));
    REQUIRE(reused.process(0) == TestType(0));
    reused.reset();
    REQUIRE(reused.process(0) == TestType(0));
    for (const TestType input : {TestType(1), TestType(-2), TestType(0), TestType(3)}) {
        REQUIRE(reused.process(input) == fresh.process(input));
    }
}

TEMPLATE_TEST_CASE("DC blocker reconfiguration preserves the current filter history",
                  "[dc-blocker]", float, double) {
    Fourier::DCBlocker<TestType> filter;
    filter.setTransitionWidth(12000, 48000);  // p = 1/2, g = 3/4
    REQUIRE(filter.process(2) == TestType(1.5));
    REQUIRE(filter.process(-1) == TestType(-1.5));
    filter.setTransitionWidth(3000, 48000);   // p = 7/8, g = 15/16
    REQUIRE(filter.getValue() == TestType(-1.5));
    // g*(3-(-1)) + p*(-1.5) = 39/16, using both pre-change delays.
    REQUIRE(filter.process(3) == TestType(2.4375));
}
