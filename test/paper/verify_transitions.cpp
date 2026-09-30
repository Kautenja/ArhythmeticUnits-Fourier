// Deterministic transition identities, retained history and corrupted outputs.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#define PAPER_FIXTURE_CLOCK
#include <limits>
#include "../../benchmark/paper/transitions.hpp"

using Paper::Config;
namespace Transition = Paper::Transition;

Config configuration(bool control = false) {
    Config c{};
    c.backend = "core-float"; c.pass = "callback"; c.state = "startup"; c.alignment = "aligned";
    c.n = 128; c.hop = 16; c.block = 16; c.callbacks = 26; c.count = c.voices = 1; c.rate = 48000;
    c.transition_suite = "interactive-v1"; c.transition_control = control;
    return c;
}

/// @brief Native scheduling fixture; actual PFFFT is covered by the short smoke.
struct FixtureBackend {
    size_t n;
    FixtureBackend(size_t length, const char*) : n(length) {}
    FixtureBackend(size_t length, const std::string&) : n(length) {}
    void forward_real_positive(const float* input, std::complex<float>* output) {
        std::vector<std::complex<double>> frame(n);
        for (size_t i = 0; i < n; ++i) frame[i] = input[i];
        const auto spectrum = Paper::Reference::independent_fft(frame);
        for (size_t k = 0; k <= n/2; ++k) output[k] = std::complex<float>(spectrum[k]);
    }
    std::string info_json() const { return "{}"; }
};

template<typename T, typename Engine>
void correct(const Config& c) {
    Transition::Adapter<T, Engine> adapter(c);
    Transition::Audit<T, Engine> audit(c);
    const auto input = Paper::signal();
    for (size_t sample = 0; sample < c.callbacks*c.block; ++sample) {
        adapter.process(input[sample%input.size()]);
        audit(adapter, input, sample);
    }
    Paper::require(audit.requests[6].replaced == 8 && audit.requests[6].application == -1,
        "Pending request replacement was not exercised");
    Paper::require(audit.requests.back().application == -1 && audit.requests.back().first == -1,
        "Missing explicit horizon no-response outcome");
    Paper::require(audit.requests[0].application == 2*c.hop, "Boundary request was not applied at boundary");
    Paper::require(audit.requests[0].first == int64_t(2*c.hop+(Engine::immediate() ? 0 : c.hop-1)),
        "First publication identity/age differs");
}

void corruption(size_t fixture) {
    const auto c = configuration();
    Transition::Adapter<float, Transition::CoreEngine<float>> adapter(c);
    Transition::Audit<float, Transition::CoreEngine<float>> audit(c);
    const auto input = Paper::signal();
    bool rejected = false, changed = false;
    try {
        for (size_t sample = 0; sample < c.callbacks*c.block; ++sample) {
            adapter.process(input[sample]);
            if (!changed && sample >= 2*c.hop && adapter.published()) {
                if (fixture == 0) ++adapter.generation;
                if (fixture == 1) ++adapter.endpoint;
                if (fixture == 2) adapter.engine.output[3] = std::numeric_limits<float>::quiet_NaN();
                if (fixture == 3) adapter.engine.output[3] += 100.f; // Mixed/corrupt bin, even if all bins finite.
                if (fixture == 4) adapter.complete = false;
                if (fixture == 5) ++adapter.history_start;
                changed = true;
            }
            if (fixture == 6 && adapter.requested) { adapter.requested = 0; changed = true; }
            if (fixture == 7 && adapter.replaced) { adapter.replaced = 0; changed = true; }
            audit(adapter, input, sample);
        }
    } catch (const std::runtime_error&) { rejected = true; }
    Paper::require(changed && rejected, "Transition corruption escaped independent replay");
}

int main() {
    for (bool control : {false, true}) {
        const auto c = configuration(control);
        correct<float, Transition::CoreEngine<float>>(c);
        correct<double, Transition::CoreEngine<double>>(c);
        correct<float, Transition::NativeEngine<float, FixtureBackend, false>>(c);
        correct<float, Transition::NativeEngine<float, FixtureBackend, true>>(c);
    }
    for (size_t fixture = 0; fixture < 8; ++fixture) corruption(fixture);
    Transition::run<float, Transition::CoreEngine<float>>(configuration());
}
