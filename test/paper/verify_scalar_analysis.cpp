// Factorized scalar coverage and corruption fixtures; no performance claims.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../benchmark/paper/scalar_analysis_audit.hpp"
#include <limits>

using namespace Paper;
size_t cases = 0;

template<typename T, template<typename> class Adapter>
void run_case(Config c, const std::vector<float>& input, size_t samples, size_t instances = 1) {
    SynthesisAccuracy accuracy; std::vector<ScalarCoverage> coverage;
    ScalarAnalysisAudit<T> audit(c, accuracy, coverage);
    std::vector<std::unique_ptr<Adapter<T>>> bank;
    for (size_t i = 0; i < instances; ++i) bank.emplace_back(new Adapter<T>(c));
    for (size_t s = 0; s < samples; ++s) for (size_t i = 0; i < instances; ++i) {
        bank[i]->process(input[s%input.size()]);
        audit(*bank[i], input, s);
    }
    require(coverage.size() == instances && accuracy.publications > 0, "Missing fixture coverage");
    for (const auto& item : coverage)
        require(item.expected_spectra == item.checked_spectra, "Truncated fixture coverage");
    ++cases;
}

template<typename T>
void suite() {
    const std::string precision = sizeof(T) == 4 ? "float" : "double";
    Config c{}; c.rate = 48000; c.state = "steady";
    auto input = signal();
    // All registered lengths, both precisions, all scalar schedules and both
    // smoothing modes. Three frames cross startup into a full represented frame.
    for (size_t n = 128; n <= 16384; n *= 2) {
        c.n = n; c.hop = n/2;
        for (bool smooth : {false, true}) {
            c.smooth = smooth;
            c.backend = "core-"+precision; run_case<T, Core>(c, input, 3*c.hop);
            c.backend = "legacy-batch-"+precision; run_case<T, Legacy>(c, input, 3*c.hop);
            c.backend = "legacy-incremental-"+precision; run_case<T, Legacy>(c, input, 3*c.hop);
        }
    }
    // Non-dividing hops, ring wraparound, live settings, and three instances.
    c.n = 128; c.hop = 37; c.state = "live";
    for (bool smooth : {false, true}) {
        c.smooth = smooth;
        c.backend = "core-"+precision; run_case<T, Core>(c, input, 20*c.hop, 3);
        c.backend = "legacy-batch-"+precision; run_case<T, Legacy>(c, input, 20*c.hop, 3);
        c.backend = "legacy-incremental-"+precision; run_case<T, Legacy>(c, input, 20*c.hop, 3);
    }
    // Independent signal shapes, with and without retained temporal smoothing.
    c.state = "startup";
    for (size_t fixture = 0; fixture < 7; ++fixture) {
        input.assign(1024, 0);
        uint32_t seed = 0x12345678u;
        for (size_t i = 0; i < input.size(); ++i) {
            if (fixture == 1 && i == 3) input[i] = 1;
            if (fixture == 2) input[i] = .25f;
            if (fixture == 3) input[i] = i%2 ? -.5f : .5f;
            if (fixture == 4) input[i] = std::sin(2*std::acos(-1.)*7.25*i/c.n);
            seed = 1664525u*seed+1013904223u;
            if (fixture == 5) input[i] = float(double(seed>>8)/16777216.-.5);
            if (fixture == 6) input[i] = 1e-9f*std::sin(.37*i);
        }
        for (bool smooth : {false, true}) {
            c.smooth = smooth;
            c.backend = "core-"+precision; run_case<T, Core>(c, input, 12*c.hop);
            c.backend = "legacy-batch-"+precision; run_case<T, Legacy>(c, input, 12*c.hop);
            c.backend = "legacy-incremental-"+precision; run_case<T, Legacy>(c, input, 12*c.hop);
        }
    }
}

template<typename T, template<typename> class Adapter>
void warm_replay(const std::string& backend) {
    Config c{}; c.backend = backend; c.n = 2048; c.hop = 257; c.rate = 44100;
    c.state = "live"; c.smooth = true; c.count = 3; c.callbacks = 12;
    c.block = 64; c.warm_hops = 2; c.callback_offset = 5; c.alignment = "staggered";
    SynthesisAccuracy accuracy; std::vector<ScalarCoverage> coverage;
    ScalarAnalysisAudit<T> audit(c, accuracy, coverage);
    const auto input = signal();
    std::vector<std::unique_ptr<Adapter<T>>> bank;
    std::vector<size_t> cursors;
    for (size_t i = 0; i < c.count; ++i) {
        bank.emplace_back(new Adapter<T>(c));
        const size_t start = ((c.n+c.hop-1)/c.hop+c.warm_hops)*c.hop
            +c.callback_offset+i*c.hop/c.count;
        cursors.push_back(start);
        for (size_t sample = 0; sample < start; ++sample)
            bank.back()->process(input[sample%input.size()]);
    }
    for (size_t s = 0; s < c.callbacks*c.block; ++s) for (size_t i = 0; i < c.count; ++i) {
        const size_t sample = cursors[i]++;
        bank[i]->process(input[sample%input.size()]);
        audit(*bank[i], input, sample);
    }
    require(!scalar_coverage_json(c, accuracy, coverage).empty(), "Missing warm replay coverage");
    ++cases;
}

void negative_cases() {
    // Corrupt data, suppress/add a publication, alter settings or history,
    // non-finite output, duplicate replay sample, and truncate a spectrum.
    for (size_t mode = 0; mode < 8; ++mode) {
        bool rejected = false;
        try {
            Config c{}; c.backend = "core-float"; c.n = 128; c.hop = 37;
            c.rate = 48000; c.state = "steady"; c.smooth = true;
            Core<float> adapter(c);
            if (mode == 3) {
                adapter.settings.window = Fourier::Window::Function::Boxcar;
                adapter.analysis.configure(adapter.settings);
            }
            SynthesisAccuracy accuracy; std::vector<ScalarCoverage> coverage;
            ScalarAnalysisAudit<float> audit(c, accuracy, coverage);
            const auto input = signal();
            for (size_t s = 0; s < 8*c.hop; ++s) {
                if (mode == 4 && s == 3*c.hop) adapter.analysis.reset();
                adapter.process(input[s%input.size()]);
                if (adapter.published() && s > c.hop) {
                    if (mode == 0) adapter.output[5] += 10;
                    if (mode == 1) adapter.complete = false;
                    if (mode == 5) adapter.output[3] = std::numeric_limits<float>::quiet_NaN();
                    if (mode == 7) adapter.output.pop_back();
                }
                if (mode == 2 && s == 3) adapter.complete = true;
                audit(adapter, input, s);
                if (mode == 6 && s == 3) audit(adapter, input, s);
            }
        } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Scalar negative fixture accepted");
    }
}
int main() {
    verify_scalar_analysis();
    suite<float>(); suite<double>();
    warm_replay<float, Core>("core-float"); warm_replay<double, Core>("core-double");
    warm_replay<float, Legacy>("legacy-batch-float"); warm_replay<double, Legacy>("legacy-batch-double");
    warm_replay<float, Legacy>("legacy-incremental-float"); warm_replay<double, Legacy>("legacy-incremental-double");
    negative_cases();
    std::cout << cases << " scalar factor cases and eight negative fixtures passed\n";
}
