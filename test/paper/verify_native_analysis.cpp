// Untimed numerical, retention, allocation and diagnostic-clock fixtures.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#define PAPER_FIXTURE_CLOCK
#define PAPER_ALLOCATION_AUDIT
#include "../../benchmark/paper/native_diagnostics.hpp"
#include "../../benchmark/paper/scalar_analysis_audit.hpp"
using namespace Paper;

Config configuration(const std::string& backend) {
    Config c{}; c.backend = backend; c.n = 128; c.hop = 37; c.block = 16; c.count = 1;
    c.voices = 1; c.rate = 48000; c.state = "startup"; c.pass = "callback"; c.alignment = "aligned";
    c.callbacks = 20; return c;
}

template<typename Kernel>
void verify_native(const std::string& name) {
    using T = typename Kernel::Scalar;
    for (size_t hop : {1u, 37u, 257u, 65536u}) {
        Config c = configuration(name+"batch-"+(sizeof(T) == 4 ? "float" : "double"));
        c.hop = hop; c.callbacks = (4*hop+c.block-1)/c.block;
        for (const std::string state : {"startup", "live"}) {
            c.state = state; c.smooth = true;
            NativeAnalysis<Kernel> batch(c);
            Config h = c; h.backend = name+"hybrid-"+(sizeof(T) == 4 ? "float" : "double");
            NativeAnalysis<Kernel> hybrid(h);
            const auto input = signal(c);
            SynthesisAccuracy ba, ha; std::vector<std::string> providers;
            NativeAudit<Kernel> batch_audit(c, ba, providers), hybrid_audit(h, ha, providers);
            const size_t total = hop == 1 ? 2*c.n+4 : 4*hop;
            for (size_t s = 0; s < total; ++s) {
                // Allocation observation excludes oracle and trace bookkeeping.
                PaperResources::reset_phase(); PaperResources::active = true;
                batch.process(input_sample(c, input, s)); hybrid.process(input_sample(c, input, s));
                PaperResources::active = false;
                require(PaperResources::counts.allocations == 0, "Native sample work allocated C++ storage");
                batch_audit(batch, input, s); hybrid_audit(hybrid, input, s);
            }
            require(ba.analysis.vectors && ha.analysis.vectors, "No native audit vectors");
            require(ba.analysis.vectors == ha.analysis.vectors, "Lost native publication");
            batch.check(); hybrid.check();
        }
    }
    // Explicit fixtures exercise separate octave/temporal controls and windows.
    for (const std::string fixture : {"silence", "decay", "impulse", "dc", "nyquist", "off-bin", "weak", "noise", "mixed"}) {
        Config c = configuration(name+"hybrid-"+(sizeof(T) == 4 ? "float" : "double"));
        c.workload_schema = 3; c.active_ports = Kernel::channels; c.fixture = fixture;
        c.window = fixture == "dc" || fixture == "nyquist" ? "boxcar" : "blackman-harris";
        c.octave = fixture == "weak" || fixture == "noise" ? 1. : 0.;
        c.temporal_value = fixture == "decay" || fixture == "weak" ? .8 : 0.; c.decay_samples = 47;
        NativeAnalysis<Kernel> adapter(c);
        SynthesisAccuracy accuracy; std::vector<std::string> providers;
        NativeAudit<Kernel> audit(c, accuracy, providers); const auto input = signal(c);
        for (size_t s = 0; s < 8*c.hop; ++s) { adapter.process(input_sample(c, input, s)); audit(adapter, input, s); }
        require(accuracy.analysis.vectors == 8*Kernel::channels, "Missing fixture channel/bin audit");
    }
    // Large frames validate native layout/scaling, all bins and wraparound.
    for (size_t n : {2048u, 16384u}) {
        Config c = configuration(name+"batch-"+(sizeof(T) == 4 ? "float" : "double")); c.n = n; c.hop = n/2;
        NativeAnalysis<Kernel> adapter(c); const auto input = signal(c);
        SynthesisAccuracy accuracy; std::vector<std::string> providers; NativeAudit<Kernel> audit(c, accuracy, providers);
        for (size_t s = 0; s < 3*n; ++s) { adapter.process(input_sample(c, input, s)); audit(adapter, input, s); }
    }
}

template<typename T>
void verify_core() {
    for (size_t n : {128u, 2048u, 16384u}) for (size_t hop : {1u, 37u, 257u, 65536u}) {
        Config c = configuration(std::string("core-matched-batch-")+(sizeof(T) == 4 ? "float" : "double"));
        c.n = n; c.hop = hop; c.state = "live"; c.smooth = true;
        MatchedCore<T, true> batch(c); MatchedCore<T, false> distributed(c);
        require(sizeof(batch) == sizeof(distributed), "Core pair storage differs");
        const auto input = signal(c); std::vector<T> expected(n/2+1);
        const size_t total = hop == 1 && n > 128 ? 4 : 2*n+4*hop;
        for (size_t s = 0; s < total; ++s) {
            PaperResources::reset_phase(); PaperResources::active = true;
            batch.process(input[s%input.size()]); distributed.process(input[s%input.size()]);
            PaperResources::active = false;
            require(PaperResources::counts.allocations == 0, "Core sample work allocated");
            require(batch.published() == (s%hop == 0) && distributed.published() == (s%hop == hop-1), "Core matched cadence");
            if (batch.published()) expected = batch.output;
            if (distributed.published()) require(distributed.output == expected, "Core matched arithmetic/cache/retention differs");
        }
        // Independent all-bin checks supplement exact matched-pair equality.
        SynthesisAccuracy accuracy; std::vector<ScalarCoverage> coverage;
        Config small = c; small.n = 128; small.hop = 37;
        MatchedCore<T, true> tested(small); ScalarAnalysisAudit<T> audit(small, accuracy, coverage);
        for (size_t s = 0; s < 4*small.hop; ++s) { tested.process(input[s]); audit(tested, input, s); }
    }
}

template<typename Kernel> void emit(const Config& c, const std::string& mode) {
    if (mode == "stream") native_dispatch<Kernel>(c, false);
    else if (mode == "info") { NativeAnalysis<Kernel> adapter(c); std::cout << adapter.info_json(); }
    else diagnose<Kernel>(c, mode);
}

int main(int argc, char** argv) {
    try {
        if (argc == 3) {
            Config c = configuration(argv[2]); c.workload_schema = 3;
            require(std::string(backend_descriptor(c.backend).precision) == "float", "Fixture CLI supports float; direct audits cover both precisions");
            c.active_ports = backend_descriptor(c.backend).channels;
            c.fixture = c.active_ports == 4 ? "independent" : "noise";
            const std::string mode(argv[1]);
            if (mode == "stream") { c.state = "steady"; c.count = 2; c.alignment = "staggered"; c.callback_offset = 3; c.warm_hops = 2; }
            if (c.backend.find("pffft-native-unordered") == 0) emit<PffftNative<true>>(c, mode);
            else if (c.backend.find("pffft") == 0) emit<PffftNative<false>>(c, mode);
#ifdef PAPER_HAVE_VDSP
            else if (c.backend.find("vdsp-native4") == 0) emit<VdspNative<float, 4>>(c, mode);
            else if (c.backend.find("vdsp") == 0) emit<VdspNative<float>>(c, mode);
#endif
#ifdef PAPER_HAVE_FFTW
            else if (c.backend.find("fftw-native4") == 0) emit<FftwNative<float, 4>>(c, mode);
            else if (c.backend.find("fftw") == 0) emit<FftwNative<float>>(c, mode);
#endif
            else throw std::runtime_error("Unknown fixture provider");
            return 0;
        }
        verify_core<float>(); verify_core<double>();
        verify_native<PffftNative<false>>("pffft-native-");
        verify_native<PffftNative<true>>("pffft-native-unordered-");
#ifdef PAPER_HAVE_VDSP
        verify_native<VdspNative<float>>("vdsp-native-");
        verify_native<VdspNative<double>>("vdsp-native-");
        verify_native<VdspNative<float, 4>>("vdsp-native4-");
        verify_native<VdspNative<double, 4>>("vdsp-native4-");
#endif
#ifdef PAPER_HAVE_FFTW
        verify_native<FftwNative<float>>("fftw-native-");
        verify_native<FftwNative<double>>("fftw-native-");
        verify_native<FftwNative<float, 4>>("fftw-native4-");
        verify_native<FftwNative<double, 4>>("fftw-native4-");
#endif
        std::cout << "Native and matched core audits passed without benchmark timing\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
