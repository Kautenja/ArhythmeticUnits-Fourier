// Untimed explicit fixtures and independent all-bin audits; no measurement loop.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../benchmark/paper/scalar_analysis_audit.hpp"
#include "../../benchmark/paper/channels.hpp"
#include "../../benchmark/paper/hybrid.hpp"
#include "../../benchmark/paper/workload_json.hpp"
using namespace Paper;

template<typename T, template<typename> class Adapter>
void scalar(Config c) {
    Adapter<T> adapter(c);
    const auto input = signal(c);
    SynthesisAccuracy accuracy; std::vector<ScalarCoverage> coverage;
    ScalarAnalysisAudit<T> audit(c, accuracy, coverage);
    for (size_t s = 0; s < 8*c.hop; ++s) {
        adapter.process(input_sample(c, input, s)); audit(adapter, input, s);
    }
    adapter.check();
    require(accuracy.analysis.vectors == 8 && accuracy.checked == 8*(c.n/2+1), "Missing explicit fixture bins");
}

template<typename Adapter, typename Audit>
void native(Config c, Audit audit) {
    Adapter adapter(c);
    const auto input = signal(c);
    for (size_t s = 0; s < 8*c.hop; ++s) {
        adapter.process(input_sample(c, input, s)); audit(adapter, input, s);
    }
    adapter.check();
}

int main(int argc, char** argv) {
    Config c{}; c.workload_schema = 3; c.n = 128; c.hop = 37; c.block = 17;
    c.count = c.voices = 1; c.rate = 48000; c.state = "startup"; c.alignment = "aligned";
    c.pass = "callback"; c.callbacks = 20; c.decay_samples = 50;
    if (argc == 2) {
        c.backend = "core-float";
        auto* json = json_loads(argv[1], JSON_REJECT_DUPLICATES, nullptr);
        parse_workload_controls(c, json);
        json_decref(json);
        validate_backend(c);
        std::cout << contract_json(c) << '\n';
        return 0;
    }
    size_t cases = 0;
    for (const std::string fixture : {"mixed", "silence", "decay", "impulse", "dc", "nyquist", "off-bin", "weak", "noise"})
        for (const std::string window : {"hann", "boxcar", "blackman-harris"})
            for (const auto smooth : {std::make_pair(0., 0.), std::make_pair(1./3, 0.),
                                     std::make_pair(0., .8), std::make_pair(1., .8)}) {
                c.fixture = fixture; c.window = window; c.octave = smooth.first; c.temporal_value = smooth.second;
                c.backend = "core-float"; validate_backend(c); scalar<float, Core>(c);
                c.backend = "core-double"; scalar<double, Core>(c);
                c.backend = "legacy-batch-float"; scalar<float, Legacy>(c);
                c.backend = "legacy-incremental-double"; scalar<double, Legacy>(c);
                c.backend = "pffft-analysis-float";
                SynthesisAccuracy accuracy; std::vector<std::string> instances;
                native<ExternalAnalysis<float, PffftBackend>>(c, ExternalAudit<float, PffftBackend>(accuracy, instances));
                c.backend = "pffft-hybrid-float";
                native<ScheduledAnalysis<float, PffftBackend>>(c, HybridAudit<float, PffftBackend>(accuracy, instances));
                ++cases;
            }
    c.fixture = "independent"; c.backend = "core-independent4-simd"; c.active_ports = 4;
    SynthesisAccuracy accuracy; std::vector<std::string> instances;
    native<SimdChannels>(c, ChannelAudit(accuracy, instances, c));
    c.backend = "core-independent4-float";
    native<ScalarChannels<Core<float>>>(c, ChannelAudit(accuracy, instances, c));
    c.backend = "pffft-analysis4-float";
    native<ScalarChannels<ExternalAnalysis<float, PffftBackend>>>(c, ChannelAudit(accuracy, instances, c));
    require(accuracy.analysis.vectors == 3*8*4, "Missing independent-channel spectra");
    c.fixture = "decay";
    const auto decay = signal(c);
    require(input_sample(c, decay, c.decay_samples) == 0 && input_sample(c, decay, 65536) == 0,
        "Decay repeated after the retained fixture period");
    c.fixture = "impulse";
    require(input_sample(c, signal(c), 65536) == 0, "Impulse unexpectedly repeated");
    c.fixture = "noise";
    const auto first = signal(c); ++c.fixture_seed;
    require(first != signal(c), "Seed was ignored");
    std::cout << cases << " explicit window/fixture/smoothing cases and independent-channel audits passed without timing\n";
}
