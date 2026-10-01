// Deterministic observation clock for correctness fixtures, never publication.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_MEASUREMENT_CLOCK_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_MEASUREMENT_CLOCK_HPP_
#include <chrono>

namespace PaperMeasurement {
#ifdef PAPER_FIXTURE_CLOCK
struct Clock {
    using duration = std::chrono::nanoseconds;
    using time_point = std::chrono::time_point<Clock, duration>;
    static duration& counter() { static duration value{}; return value; }
    static time_point now() {
        counter() += duration(100);
        return time_point(counter());
    }
    static void advance_to(duration value) { if (value > counter()) counter() = value; }
};
#else
using Clock = std::chrono::steady_clock;
#endif
}  // namespace PaperMeasurement
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_MEASUREMENT_CLOCK_HPP_
