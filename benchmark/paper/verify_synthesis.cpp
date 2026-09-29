// Host-independent correctness and negative checks for synthesis benchmarks.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "synthesis.hpp"

template<typename Operation>
void rejects(Operation operation) {
    bool rejected = false;
    try { operation(); } catch (const std::runtime_error&) { rejected = true; }
    Paper::require(rejected, "A deliberately incorrect synthesis result was accepted");
}

template<typename T>
void fixtures() {
    using namespace Paper;
    for (const std::string family : {"ols-identity", "ols-fir"})
        for (const std::string mode : {"batch", "incremental"})
            for (size_t hop : {37u, 64u, 126u})
                for (size_t fixture = 0; fixture < 4; ++fixture) {
                    Config c{};
                    c.n = 128; c.hop = hop; c.backend = family+"-"+mode+"-float";
                    FrequencyChain<T> adapter(c);
                    std::vector<float> input(1024, 0);
                    if (fixture == 1) input[hop-1] = 1;
                    if (fixture == 2) std::fill(input.begin(), input.end(), 0.25f);
                    if (fixture == 3) input = signal();
                    SynthesisAccuracy accuracy;
                    SynthesisAudit audit{accuracy};
                    for (size_t sample = 0; sample < 3*input.size(); ++sample) {
                        adapter.process(input[sample%input.size()]);
                        audit(adapter, input, sample);
                    }
                    require(accuracy.publications > 0, "No fixture outputs checked");
                }
}

int main() {
    using namespace Paper;
    try {
        verify_synthesis<float>();
        verify_synthesis<double>();
        fixtures<float>();
        fixtures<double>();
        Config c{};
        c.n = 128; c.hop = 32; c.backend = "inverse-stream-batch-float";
        InverseStream<float> inverse(c);
        inverse.process(0);
        SynthesisAccuracy accuracy;
        SynthesisAudit audit{accuracy};
        const auto input = signal();
        audit(inverse, input, 0);
        inverse.output[1] *= 128.f;
        rejects([&]() { audit(inverse, input, 0); });
        c.backend = "ols-fir-batch-float";
        FrequencyChain<float> chain(c);
        chain.process(input[0]);
        chain.playing.back() += std::complex<float>(1, 0);
        rejects([&]() { audit(chain, input, 0); });
        c.hop = c.n-1;
        rejects([&]() { FrequencyChain<float> invalid(c); });
        std::cout << "Synthesis baselines passed: both precisions, batch/incremental, "
            << "analytical inverse, direct FIR/identity, startup, wraparound, "
            << "non-dividing hops, output boundaries, and negative checks\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
