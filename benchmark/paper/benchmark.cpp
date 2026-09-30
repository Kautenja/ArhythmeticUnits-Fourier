// Publication experiments using current production code and the common protocol.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef PAPER_FIXTURE_CLOCK
#error The benchmark executable cannot be built with synthetic measurement clocks.
#endif
#include <complex>
#include <cstdlib>
#include <limits>
#include "fourier.hpp"
#include "modules.hpp"
#include "protocol.hpp"
#include "synthesis.hpp"
#include "scalar_analysis_audit.hpp"
#include "external.hpp"
#include "hybrid.hpp"
#include "pffft.hpp"
#include "channels.hpp"
#ifdef PAPER_HAVE_VDSP
#include "vdsp.hpp"
#endif
#ifdef PAPER_HAVE_FFTW
#include "fftw.hpp"
#endif

#include "transitions.hpp"
#include "native_analysis.hpp"
#include "native_diagnostics.hpp"
#include "module_audit.hpp"
#include "engine_host.hpp"

namespace Paper {
/// @brief Harness/load control, labeled separately from spectral computation.
struct Driver {
    float output = 0;
    explicit Driver(const Config&) {}
    void process(float value) { output = 0.999f*output + value; }
    bool published() { return false; }
    size_t delay() const { return 0; }
    void barrier() const { observe(output); }
    void check() const { require(std::isfinite(output), "Invalid driver output"); }
};

size_t integer(const char* value) {
    char* end = nullptr;
    const auto result = std::strtoull(value, &end, 10);
    require(*value && *value != '-' && end && !*end && result <= (1u << 28), "Invalid integer argument");
    return result;
}
/// @brief Independent correctness preflight, shared by both runners.
void verify_all() {
    verify_transform<float, Fourier::OnTheFlyFFT<float>>(false, false);
    verify_transform<double, Fourier::OnTheFlyFFT<double>>(false, false);
    verify_transform<float, Fourier::OnTheFlyRFFT<float>>(true, false);
    verify_transform<double, Fourier::OnTheFlyRFFT<double>>(true, false);
    verify_transform<float, Fourier::OnTheFlyIFFT<float>>(false, true);
    verify_transform<double, Fourier::OnTheFlyIFFT<double>>(false, true);
    verify_scalar_analysis();
    Transition::verify<float, Transition::CoreEngine<float>>();
    Transition::verify<double, Transition::CoreEngine<double>>();
    Transition::verify<float, Transition::NativeEngine<float, PffftBackend, false>>();
    Transition::verify<float, Transition::NativeEngine<float, PffftBackend, true>>();
    verify_analyzer<float>();
    verify_analyzer<double>();
    verify_controls<float>();
    verify_controls<double>();
    verify_synthesis<float>();
    verify_synthesis<double>();
    verify_external<float, PffftBackend>("pffft", "float");
    verify_hybrid<PffftBackend>();
    verify_channels();
    verify_native_preflight<PffftNative<false>>("pffft-native-hybrid-float");
    verify_native_preflight<PffftNative<true>>("pffft-native-unordered-batch-float");
#ifdef PAPER_HAVE_VDSP
    verify_external<float, VdspBackend<float>>("vdsp", "float");
    verify_external<double, VdspBackend<double>>("vdsp", "double");
    verify_native_preflight<VdspNative<float>>("vdsp-native-hybrid-float");
    verify_native_preflight<VdspNative<double>>("vdsp-native-batch-double");
    verify_native_preflight<VdspNative<float, 4>>("vdsp-native4-batch-float");
    verify_native_preflight<VdspNative<double, 4>>("vdsp-native4-hybrid-double");
#endif
#ifdef PAPER_HAVE_FFTW
    verify_external<float, FftwBackend<float>>("fftw", "float");
    verify_external<double, FftwBackend<double>>("fftw", "double");
    verify_native_preflight<FftwNative<float>>("fftw-native-hybrid-float");
    verify_native_preflight<FftwNative<double>>("fftw-native-batch-double");
    verify_native_preflight<FftwNative<float, 4>>("fftw-native4-batch-float");
    verify_native_preflight<FftwNative<double, 4>>("fftw-native4-hybrid-double");
#endif
    std::cout << "Independent transform/analyzer fixtures and matched analysis frames verified for 48 configurations and two controls; "
        << "inverse jobs and overlap-save identity/FIR verified in both precisions; PFFFT hybrid, scalar replay, transition lifecycle and all compiled native kernels verified\n";
}

/// @brief One workload with unchanged timing and numerical-audit boundaries.
void execute(const Config& c, bool provider_info = false, const std::string& diagnostic = "") {
    const auto& descriptor = backend_descriptor(c.backend);
    const std::string kind(descriptor.kind), precision(descriptor.precision);
    require(!provider_info || kind == "external" || kind == "scheduled-analysis" || kind == "analysis4" || kind == "native-analysis", "Provider metadata is only available for external adapters");
    Paper::Context context(c.rate);
    std::unique_ptr<Execution::Session> execution;
    require(diagnostic.empty() || kind == "native-analysis", "Stage diagnostics require a native-segments backend");
    if (!c.resources && !provider_info && diagnostic.empty()) execution.reset(new Execution::Session(c.pass));
    if (execution && c.workload_schema == 3)
        require(execution->policy.regime == c.execution_regime, "Workload/execution regime mismatch");
    if (!c.transition_suite.empty()) {
        require(!provider_info, "Use transition resource and trace provenance");
        if (c.backend == "core-float") Transition::run<float, Transition::CoreEngine<float>>(c);
        else if (c.backend == "core-double") Transition::run<double, Transition::CoreEngine<double>>(c);
        else if (c.backend == "pffft-hybrid-float") Transition::run<float, Transition::NativeEngine<float, PffftBackend, false>>(c);
        else if (c.backend == "pffft-scheduled-batch-float") Transition::run<float, Transition::NativeEngine<float, PffftBackend, true>>(c);
        else require(false, "Backend has no transition adapter");
    }
    else if (kind == "native-analysis") {
        const std::string provider(descriptor.provider);
        if (provider == "pffft") {
            if (c.backend.find("unordered") != std::string::npos) native_dispatch<PffftNative<true>>(c, provider_info, diagnostic);
            else native_dispatch<PffftNative<false>>(c, provider_info, diagnostic);
        }
#ifdef PAPER_HAVE_VDSP
        else if (provider == "vdsp") {
            if (descriptor.channels == 4) {
                if (precision == "float") native_dispatch<VdspNative<float, 4>>(c, provider_info, diagnostic);
                else native_dispatch<VdspNative<double, 4>>(c, provider_info, diagnostic);
            } else if (precision == "float") native_dispatch<VdspNative<float>>(c, provider_info, diagnostic);
            else native_dispatch<VdspNative<double>>(c, provider_info, diagnostic);
        }
#endif
#ifdef PAPER_HAVE_FFTW
        else if (provider == "fftw") {
            if (descriptor.channels == 4) {
                if (precision == "float") native_dispatch<FftwNative<float, 4>>(c, provider_info, diagnostic);
                else native_dispatch<FftwNative<double, 4>>(c, provider_info, diagnostic);
            } else if (precision == "float") native_dispatch<FftwNative<float>>(c, provider_info, diagnostic);
            else native_dispatch<FftwNative<double>>(c, provider_info, diagnostic);
        }
#endif
        else require(false, "Unknown native analysis provider");
    }
    else if (c.backend == "core-independent4-simd") channel_stream<SimdChannels>(c, provider_info);
    else if (c.backend == "core-independent4-float") channel_stream<ScalarChannels<Core<float>>>(c, provider_info);
    else if (c.backend == "pffft-analysis4-float") channel_stream<ScalarChannels<ExternalAnalysis<float, PffftBackend>>>(c, provider_info);
#ifdef PAPER_HAVE_FFTW
    else if (c.backend == "fftw-analysis4-float") channel_stream<ScalarChannels<ExternalAnalysis<float, FftwBackend<float>>>>(c, provider_info);
#endif
#ifdef PAPER_HAVE_VDSP
    else if (c.backend == "vdsp-analysis4-float") channel_stream<ScalarChannels<ExternalAnalysis<float, VdspBackend<float>>>>(c, provider_info);
#endif
    else if (kind == "scheduled-analysis") hybrid_dispatch<float, PffftBackend>(c, provider_info);
    else if (std::string(descriptor.provider) == "pffft") external_dispatch<float, PffftBackend>(c, provider_info);
#ifdef PAPER_HAVE_FFTW
    else if (std::string(descriptor.provider) == "fftw") {
        if (precision == "float") external_dispatch<float, FftwBackend<float>>(c, provider_info);
        else external_dispatch<double, FftwBackend<double>>(c, provider_info);
    }
#endif
#ifdef PAPER_HAVE_VDSP
    else if (std::string(descriptor.provider) == "vdsp") {
        if (precision == "float") external_dispatch<float, VdspBackend<float>>(c, provider_info);
        else external_dispatch<double, VdspBackend<double>>(c, provider_info);
    }
#endif
    else if (kind == "inverse-job" || kind == "chain") {
        if (precision == "float") synthesis_stream<float>(c);
        else synthesis_stream<double>(c);
    } else if (kind == "driver") stream<Driver>(c);
    else if (kind == "legacy") {
        if (precision == "float") scalar_analysis_stream<float, Legacy<float>>(c);
        else scalar_analysis_stream<double, Legacy<double>>(c);
    } else if (kind == "core") {
        if (c.backend.find("core-matched-batch-") == 0) {
            if (precision == "float") scalar_analysis_stream<float, MatchedCore<float, true>>(c);
            else scalar_analysis_stream<double, MatchedCore<double, true>>(c);
        }
        else if (descriptor.channels == 4) stream<Core<simd::float_4>>(c);
        else if (precision == "float") scalar_analysis_stream<float, Core<float>>(c);
        else scalar_analysis_stream<double, Core<double>>(c);
    } else if (kind == "fourier") module_stream<SpectrumAnalyzer>(c);
    else if (kind == "spectre") module_stream<Spectrogram>(c);
    else if (kind == "fft") {
        if (precision == "float") transform<float, Fourier::OnTheFlyFFT<float>>(c, false, false);
        else transform<double, Fourier::OnTheFlyFFT<double>>(c, false, false);
    } else if (kind == "rfft") {
        if (precision == "float") transform<float, Fourier::OnTheFlyRFFT<float>>(c, true, false);
        else transform<double, Fourier::OnTheFlyRFFT<double>>(c, true, false);
    } else if (kind == "ifft") {
        if (precision == "float") transform<float, Fourier::OnTheFlyIFFT<float>>(c, false, true);
        else transform<double, Fourier::OnTheFlyIFFT<double>>(c, false, true);
    } else require(false, "Adapter implementation missing");
    if (execution) execution->finish();
}
}  // namespace Paper

#include "development.hpp"

int main(int argc, char** argv) {
    using namespace Paper;
    try {
        if (argc == 3 && std::string(argv[1]) == "--engine-resources") {
            const auto json = Development::read_json(argv[2]);
            std::cout << EngineHost::resources(EngineHost::parse(json.get())) << '\n'; return 0;
        }
        if (argc == 3 && (std::string(argv[1]) == "--engine" || std::string(argv[1]) == "--engine-verify"))
            return EngineHost::run(argv[2], std::string(argv[1]) == "--engine-verify");
        if (argc > 1 && std::string(argv[1]) == "--development")
            return Development::run(argc-2, argv+2, argv[0], [](const Config& c, bool info) { execute(c, info); }, verify_all, []() {
                verify_analyzer<float>();
                verify_analyzer<double>();
                verify_controls<float>({128, 2048}, {257, 1024});
                verify_controls<double>({128, 2048}, {257, 1024});
            });
        if (argc == 2 && std::string(argv[1]) == "--verify") { verify_all(); return 0; }
        if (argc == 2 && std::string(argv[1]) == "--inventory") {
            std::cout << registry_json << '\n'; return 0;
        }
        std::string diagnostic;
        if (argc > 3 && std::string(argv[1]) == "--diagnostic") {
            diagnostic = argv[2]; argc -= 2; argv += 2;
        }
        bool describe = false, resources = false, provider_info = false;
        if (argc > 2 && std::string(argv[1]).substr(0, 2) == "--" && std::string(argv[1]) != "--transition") {
            describe = std::string(argv[1]) == "--describe";
            resources = std::string(argv[1]) == "--resources";
            provider_info = std::string(argv[1]) == "--provider-info";
            require(describe || resources || provider_info, "Unknown command");
            --argc; ++argv;
        }
        Config c;
        if (argc > 4 && std::string(argv[1]) == "--transition") {
            c.transition_suite = argv[2];
            require(std::string(argv[3]) == "control" || std::string(argv[3]) == "change", "Invalid transition mode");
            c.transition_control = std::string(argv[3]) == "control";
            argc -= 3; argv += 3;
        }
        require(argc == 17 || argc == 18 || argc == 19, "Use run.py; expected v1, v2 or v3 protocol arguments");
        c.backend = argv[1]; c.pass = argv[2]; c.n = integer(argv[3]); c.hop = integer(argv[4]);
        c.block = integer(argv[5]); c.count = integer(argv[6]); c.alignment = argv[7];
        c.load = integer(argv[8]);
        const size_t smooth = integer(argv[9]);
        require(smooth <= 1, "Smoothing must be 0 or 1");
        c.smooth = smooth; c.voices = integer(argv[10]);
        c.callbacks = integer(argv[11]); c.warm_hops = integer(argv[12]); c.rate = integer(argv[13]);
        c.state = argv[14]; c.cache_mib = integer(argv[15]);
        require((argc == 17 && std::string(argv[16]) == "v1")
            || (argc == 18 && std::string(argv[17]) == "v2")
            || (argc == 19 && std::string(argv[17]) == "v3"), "Unknown protocol version");
        c.callback_offset = argc >= 18 ? integer(argv[16]) : 0;
        if (argc == 19) {
            auto controls = Development::own(json_loads(argv[18], JSON_REJECT_DUPLICATES, nullptr));
            parse_workload_controls(c, controls.get());
        }
        require(c.n >= 128 && c.n <= 16384 && !(c.n & (c.n-1)) && c.hop && c.hop <= 65536,
            "Invalid FFT length or hop");
        require(c.block && c.block <= 65536 && c.count && c.count <= 64 && c.callbacks
            && c.callbacks <= 1000000 && c.load <= 4096 && c.voices >= 1 && c.voices <= 16
            && c.rate >= 8000 && c.rate <= 192000 && c.cache_mib <= 256 && c.warm_hops <= 4096,
            "Workload outside protocol bounds");
        require(c.alignment == "aligned" || c.alignment == "staggered", "Invalid alignment");
        require(c.state == "steady" || c.state == "startup" || c.state == "live", "Invalid state");
        validate_backend(c);
        if (!c.transition_suite.empty()) Transition::suite(c);
        require(diagnostic.empty() || (!describe && !resources && !provider_info && c.transition_suite.empty()),
            "Stage diagnostics cannot be combined with another command mode");
        if (describe) { std::cout << contract_json(c) << '\n'; return 0; }
        c.resources = resources;
        Runtime::Profile runtime(!resources && !provider_info && diagnostic.empty());
        Runtime::set(Runtime::Phase::Setup);
        execute(c, provider_info, diagnostic);
        runtime.finish();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
