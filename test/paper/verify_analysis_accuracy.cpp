// Acceptance must be scale-invariant, per-channel and sensitive to corruption.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../benchmark/paper/analysis_accuracy.hpp"
#include <iostream>
#include <limits>

void expect(bool value) { if (!value) throw std::runtime_error("Analysis policy regression"); }
void rejects(const std::vector<float>& expected, const std::vector<float>& actual) {
    bool rejected = false;
    try { Paper::AnalysisAccuracy a; a.compare(expected, [&](size_t k) { return actual[k]; }, 0); }
    catch (const std::runtime_error&) { rejected = true; }
    expect(rejected);
}
int main() {
    // Real pilot failure: a weak bin fails the old pointwise criterion even
    // though the spectrum-level error is below the existing transform budget.
    std::vector<float> expected{4096.f, 0.75898933410644531f, 8.f};
    std::vector<float> actual{4096.f, 0.75855857133865356f, 8.f};
    Paper::AnalysisAccuracy baseline;
    baseline.compare(expected, [&](size_t k) { return actual[k]; }, 39936);
    expect(baseline.pointwise_failures == 1 && baseline.maximum_pointwise > 3e-4);
    for (float scale : {1e-6f, 1.f, 1e6f}) {
        std::vector<float> e(expected), a(actual);
        for (auto& v : e) v *= scale;
        for (auto& v : a) v *= scale;
        Paper::AnalysisAccuracy measured;
        measured.compare(e, [&](size_t k) { return a[k]; }, 0);
        expect(std::abs(measured.maximum_l2/baseline.maximum_l2-1) < .001);
        for (auto& v : a) v *= .5f;
        rejects(e, a);  // Scaling error, even when every absolute value is tiny.
    }
    auto bad = expected; bad[2] = 0; rejects(expected, bad);  // Missing weak tone.
    bad = expected; std::swap(bad[0], bad[1]); rejects(expected, bad);  // Layout.
    bad = expected; bad[1] = std::numeric_limits<float>::quiet_NaN(); rejects(expected, bad);
    rejects({0,0,0}, {0,1e-20f,0});
    rejects({1e-20f,2e-20f}, {0,0});  // The old absolute floor could hide this.
    // A prior strong frame or channel must not supply the scale for a weak one.
    bool rejected = false;
    try {
        Paper::AnalysisAccuracy a;
        a.compare(expected, [&](size_t k) { return expected[k]; }, 0, 0);
        std::vector<float> weak{1e-6f,2e-6f};
        a.compare(weak, [](size_t) { return 0.f; }, 1, 1);
    } catch (const std::runtime_error&) { rejected = true; }
    expect(rejected);
    std::vector<float> zeros(65, 0);
    Paper::AnalysisAccuracy silence;
    silence.compare(zeros, [](size_t) { return 0.f; }, 0);
    expect(silence.zero_vectors == 1 && silence.maximum_l2 == 0);
    std::cout << baseline.json() << '\n';
}
