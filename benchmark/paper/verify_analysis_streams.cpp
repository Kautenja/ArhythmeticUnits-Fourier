// Untimed long-stream and dynamic-range validation for native analysis backends.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "external.hpp"
#include "providers/pffft.hpp"
#ifdef PAPER_HAVE_FFTW
#include "providers/fftw.hpp"
#endif
#ifdef PAPER_HAVE_VDSP
#include "providers/vdsp.hpp"
#endif

template<typename T, typename Backend>
void verify(const char* provider, size_t n, bool smooth, double amplitude, bool pilot) {
    using namespace Paper;
    Config c{}; c.n = n; c.hop = n == 128 ? 37 : 1024; c.rate = 48000;
    c.state = "steady"; c.smooth = smooth; c.backend = "pffft-analysis-float";
    auto input = signal();
    size_t samples = 220000;
    if (!pilot) {
        samples = 4*n+3*c.hop;
        input.assign(samples, 0);
        uint32_t seed = 12345;
        for (size_t s = 0; s < 2*n; ++s) {
            seed = 1664525u*seed+1013904223u;
            const double angle = 2*std::acos(-1.)*s/n;
            input[s] = float(amplitude*(std::sin(7*angle)+1e-3*std::cos(31*angle)
                +1e-5*(double(seed>>8)/16777216.-.5)));
        }
    }
    ExternalAnalysis<T, Backend> adapter(c);
    AnalysisReference<T> reference(c);
    AnalysisAccuracy accuracy;
    for (size_t s = 0; s < samples; ++s) {
        adapter.process(input[s%input.size()]);
        if (!adapter.published()) continue;
        reference.advance(input, s);
        accuracy.compare(reference.expected, [&](size_t k) { return adapter.output[k]; }, s);
        // Independently check the large binary64 oracle at strong, weak, and
        // near-Nyquist bins; long double may be binary64 on this machine.
        if (!pilot && n > 256 && s == n) {
            const std::vector<Reference::Complex> frame(reference.frame.begin(), reference.frame.end());
            for (size_t k : {size_t(0), size_t(7), size_t(31), n/2-1, n/2}) {
                const auto direct = Reference::coefficient(frame, k, false);
                require(std::abs(Reference::Complex(reference.forward.coefficients[k])-direct)
                    <= 1e-8*amplitude, "Binary64 oracle disagrees with selected direct sums");
            }
        }
    }
    require(accuracy.vectors && (pilot || smooth || accuracy.zero_vectors), "Missing stream/silence coverage");
    std::cout << "{\"provider\":\"" << provider << "\",\"precision\":\"" << (sizeof(T) == 4 ? "float" : "double")
        << "\",\"n\":" << n << ",\"smooth\":" << smooth << ",\"pilot_signal\":" << pilot
        << ",\"amplitude\":" << amplitude << ",\"accuracy\":" << accuracy.json() << "}\n";
}

template<typename T, typename Backend>
void suite(const char* name) {
    for (bool smooth : {false, true}) {
        verify<T, Backend>(name, 16384, smooth, 1, true);
        for (size_t n : {128u, 2048u, 4096u, 16384u})
            for (double amplitude : {1., 1e-6}) verify<T, Backend>(name, n, smooth, amplitude, false);
    }
}
int main() {
    suite<float, Paper::PffftBackend>("pffft");
#ifdef PAPER_HAVE_FFTW
    suite<float, Paper::FftwBackend<float>>("fftw");
    suite<double, Paper::FftwBackend<double>>("fftw");
#endif
#ifdef PAPER_HAVE_VDSP
    suite<float, Paper::VdspBackend<float>>("vdsp");
    suite<double, Paper::VdspBackend<double>>("vdsp");
#endif
}
