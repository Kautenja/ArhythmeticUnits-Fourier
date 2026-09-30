// Fake-clock pacing, chunking and policy regressions; no benchmark is run.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iostream>
#include "../../benchmark/paper/execution.hpp"

namespace E = Paper::Execution;
struct FakeClock {
    int64_t value = 1000000;
    int64_t now() const { return value; }
    void sleep_until(int64_t target) { value = std::max(value, target); }
};

template<typename Action> void rejects(Action action) {
    bool failed = false;
    try { action(); } catch (const std::runtime_error&) { failed = true; }
    E::check(failed, "Invalid execution was accepted");
}

int main() {
    E::Policy policy;
    policy.regime = "paced";
    FakeClock clock;
    std::vector<E::Observation> rows;
    rows.reserve(5);
    size_t samples = 0;
    E::measure(policy, "callback", 5, 64, 48000, clock,
        [&](size_t count) { samples += count; clock.value += samples == 64 ? 5000000 : 100; },
        [&]() { clock.value += 25; }, rows);
    E::check(samples == 320 && rows.size() == 5, "Overrun dropped logical samples");
    E::check(rows[1].release == 1333333 && rows[2].release == 2666667,
        "Absolute releases were rebased or accumulated rounding drift");
    E::check(rows[1].wake == rows[0].finish && rows[1].wake > rows[1].release,
        "Overdue callback did not catch up");
    E::check(rows[4].wake == rows[4].release && rows[4].start-rows[4].wake == 25,
        "Recovered pacing or separate conditioning duration is wrong");
    E::check(rows[0].finish > rows[0].deadline && rows[4].finish <= rows[4].deadline,
        "Release-to-finish deadline semantics changed");
    for (const auto& row : rows)
        E::check(row.deadline == E::offset_ns(row.sample+row.samples, 48000), "Wrong deadline");

    policy.regime = "continuous"; policy.chunks = 3;
    rows.clear(); samples = 0;
    E::measure(policy, "throughput", 10, 17, 44100, clock,
        [&](size_t count) { samples += count; clock.value += count; }, []() {}, rows);
    E::check(rows.size() == 3 && rows[0].samples == 68 && rows[1].samples == 51 &&
        rows[2].samples == 51 && rows[2].sample == 119 && samples == 170,
        "Non-divisible throughput chunks lost or duplicated input");
    for (const auto& row : rows)
        E::check(row.release == -1 && row.deadline == -1, "Unpaced samples claim release times");
    rows.clear(); policy.chunks = 8;
    E::measure(policy, "throughput", 2, 1, 48000, clock, [](size_t) {}, []() {}, rows);
    E::check(rows.size() == 2, "Small workload created empty chunks");
    rejects([&]() { policy.regime = "rack-engine"; policy.validate("callback"); });
    rejects([&]() { policy.regime = "paced"; policy.validate("throughput"); });
    rejects([&]() { policy.regime = "continuous"; policy.chunks = 0; policy.validate("callback"); });
    rejects([&]() { E::offset_ns(1, 0); });
    rejects([&]() { E::offset_ns(std::numeric_limits<size_t>::max(), 1); });
    std::cout << "Fake-clock execution fixtures passed; no measured performance data\n";
}
