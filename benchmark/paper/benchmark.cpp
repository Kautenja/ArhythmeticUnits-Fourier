// Publication experiments using current production code and the common protocol.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <complex>
#include <cstdlib>
#include <limits>
#include "fourier.hpp"
#include "modules.hpp"
#include "protocol.hpp"
#include "synthesis.hpp"
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
    verify_analyzer<float>();
    verify_analyzer<double>();
    verify_controls<float>();
    verify_controls<double>();
    verify_synthesis<float>();
    verify_synthesis<double>();
    verify_external<float, PffftBackend>("pffft", "float");
    verify_hybrid<PffftBackend>();
    verify_channels();
#ifdef PAPER_HAVE_VDSP
    verify_external<float, VdspBackend<float>>("vdsp", "float");
    verify_external<double, VdspBackend<double>>("vdsp", "double");
#endif
#ifdef PAPER_HAVE_FFTW
    verify_external<float, FftwBackend<float>>("fftw", "float");
    verify_external<double, FftwBackend<double>>("fftw", "double");
#endif
    std::cout << "Independent transform/analyzer fixtures and matched analysis frames verified for 48 configurations and two controls; "
        << "inverse jobs and overlap-save identity/FIR verified in both precisions; PFFFT hybrid verified\n";
}

/// @brief One workload with unchanged timing and numerical-audit boundaries.
void execute(const Config& c, bool provider_info = false) {
    const auto& descriptor = backend_descriptor(c.backend);
    const std::string kind(descriptor.kind), precision(descriptor.precision);
    require(!provider_info || kind == "external" || kind == "scheduled-analysis" || kind == "analysis4", "Provider metadata is only available for external adapters");
    Paper::Context context(c.rate);
    if (c.backend == "core-independent4-simd") channel_stream<SimdChannels>(c, provider_info);
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
        if (precision == "float") stream<Legacy<float>>(c);
        else stream<Legacy<double>>(c);
    } else if (kind == "core") {
        if (descriptor.channels == 4) stream<Core<simd::float_4>>(c);
        else if (precision == "float") stream<Core<float>>(c);
        else stream<Core<double>>(c);
    } else if (kind == "fourier") stream<Host<SpectrumAnalyzer>>(c);
    else if (kind == "spectre") stream<Host<Spectrogram>>(c);
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
}
}  // namespace Paper

#include "development.hpp"

int main(int argc, char** argv) {
    using namespace Paper;
    try {
        if (argc > 1 && std::string(argv[1]) == "--development")
            return Development::run(argc-2, argv+2, argv[0], execute, verify_all, []() {
                verify_analyzer<float>();
                verify_analyzer<double>();
                verify_controls<float>({128, 2048}, {257, 1024});
                verify_controls<double>({128, 2048}, {257, 1024});
            });
        if (argc == 2 && std::string(argv[1]) == "--verify") { verify_all(); return 0; }
        if (argc == 2 && std::string(argv[1]) == "--inventory") {
            std::cout << registry_json << '\n'; return 0;
        }
        bool describe = false, resources = false, provider_info = false;
        if (argc > 2 && std::string(argv[1]).substr(0, 2) == "--") {
            describe = std::string(argv[1]) == "--describe";
            resources = std::string(argv[1]) == "--resources";
            provider_info = std::string(argv[1]) == "--provider-info";
            require(describe || resources || provider_info, "Unknown command");
            --argc; ++argv;
        }
        require(argc == 17 || argc == 18, "Use docs/whitepaper/benchmarks/run.py; expected v1 or v2 protocol arguments");
        Config c;
        c.backend = argv[1]; c.pass = argv[2]; c.n = integer(argv[3]); c.hop = integer(argv[4]);
        c.block = integer(argv[5]); c.count = integer(argv[6]); c.alignment = argv[7];
        c.load = integer(argv[8]);
        const size_t smooth = integer(argv[9]);
        require(smooth <= 1, "Smoothing must be 0 or 1");
        c.smooth = smooth; c.voices = integer(argv[10]);
        c.callbacks = integer(argv[11]); c.warm_hops = integer(argv[12]); c.rate = integer(argv[13]);
        c.state = argv[14]; c.cache_mib = integer(argv[15]);
        require((argc == 17 && std::string(argv[16]) == "v1")
            || (argc == 18 && std::string(argv[17]) == "v2"), "Unknown protocol version");
        c.callback_offset = argc == 18 ? integer(argv[16]) : 0;
        require(c.n >= 128 && c.n <= 16384 && !(c.n & (c.n-1)) && c.hop && c.hop <= 65536,
            "Invalid FFT length or hop");
        require(c.block && c.block <= 65536 && c.count && c.count <= 64 && c.callbacks
            && c.callbacks <= 1000000 && c.load <= 4096 && c.voices >= 1 && c.voices <= 16
            && c.rate >= 8000 && c.rate <= 192000 && c.cache_mib <= 256 && c.warm_hops <= 4096,
            "Workload outside protocol bounds");
        require(c.alignment == "aligned" || c.alignment == "staggered", "Invalid alignment");
        require(c.state == "steady" || c.state == "startup" || c.state == "live", "Invalid state");
        validate_backend(c);
        if (describe) { std::cout << contract_json(c) << '\n'; return 0; }
        c.resources = resources;
        Runtime::Profile runtime(!resources && !provider_info);
        Runtime::set(Runtime::Phase::Setup);
        execute(c, provider_info);
        runtime.finish();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
