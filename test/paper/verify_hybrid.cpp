// Schedule and retained-input invariants without a Rack/native FFT dependency.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../benchmark/paper/hybrid.hpp"

struct InspectBackend {
    size_t n, calls = 0, endpoint = 0;
    bool live = false;
    InspectBackend(size_t length, const std::string&) : n(length) {}
    void forward_real_positive(const float* input, std::complex<float>* output) {
        const auto function = live && calls%2 == 0 ? Fourier::Window::Function::BlackmanHarris
                                                  : Fourier::Window::Function::Hann;
        const float gain = 1.f/Fourier::Window::coherent_gain(function);
        for (size_t i = 0; i < n; ++i) {
            const int64_t source = int64_t(endpoint)-int64_t(n)+1+int64_t(i);
            const float value = source < 0 ? 0.f : float(source%23-11);
            const float expected = value*(gain*Fourier::Window::window<float>(function, float(i), float(n), false));
            Paper::require(input[i] == expected, "Native call read an incomplete or overwritten frame");
        }
        for (size_t k = 0; k <= n/2; ++k) output[k] = {float(k+1), 0};
        ++calls;
    }
    std::string info_json() const { return "{}"; }
};

/// @brief Direct band sums and EMA history across optional/live smoothing.
/// @details The native fixture produces exact positive bins k+1. Compare the
/// ordinary batch, scheduled batch and hybrid against independent scalar sums,
/// including frames that disable bands between frames that require them.
void verify_optional_postprocessing() {
    using namespace Paper;
    for (bool smooth : {false, true}) for (bool live : {false, true}) {
        Config c{};
        c.n = 128; c.hop = 37; c.rate = 48000; c.smooth = smooth;
        c.state = live ? "live" : "steady";
        c.backend = "pffft-hybrid-float";
        ScheduledAnalysis<float, InspectBackend> hybrid(c);
        c.backend = "pffft-scheduled-batch-float";
        ScheduledAnalysis<float, InspectBackend> scheduled(c);
        c.backend = "pffft-analysis-float";
        ExternalAnalysis<float, InspectBackend> batch(c);
        hybrid.fft.live = scheduled.fft.live = batch.fft.live = live;
        std::vector<float> expected(c.n/2+1, 0.f);
        for (size_t s = 0; s < 8*c.hop; ++s) {
            hybrid.fft.endpoint = scheduled.fft.endpoint = batch.fft.endpoint = s/c.hop*c.hop;
            const float input = float(int64_t(s%23)-11);
            hybrid.process(input); scheduled.process(input); batch.process(input);
            if (batch.published()) {
                const bool bands = live ? (s/c.hop)%2 == 1 : smooth;
                const float alpha = smooth ? 0.8f : 0.f;
                for (size_t k = 0; k < expected.size(); ++k) {
                    long double magnitude = k+1;
                    if (bands) {
                        const double width = double(c.rate)/c.n;
                        const float half = std::pow(2.f, (1.f/3.f)/2.f);
                        double a = k*width/half, b = k*width*half;
                        if (b > c.rate/2) { b = c.rate/2; a = b/std::pow(2.f, 1.f/3.f); }
                        const size_t first = size_t(std::floor(a/width));
                        const size_t last = std::min(c.n/2, size_t(std::floor(b/width)));
                        magnitude = 0;
                        for (size_t j = first; j <= last; ++j) magnitude += j+1;
                        magnitude /= last-first+1;
                    }
                    expected[k] = float(alpha*expected[k]+(1.f-alpha)*magnitude);
                }
            }
            auto compare = [&](const std::vector<float>& output) {
                for (size_t k = 0; k < output.size(); ++k)
                    require(std::abs(output[k]-expected[k]) <= 1e-5f*std::max(1.f, expected[k]),
                        "Optional smoothing changed band output or EMA history");
            };
            require(batch.published() == scheduled.published(), "Matched batch cadence changed");
            if (batch.published()) { compare(batch.output); compare(scheduled.output); }
            if (hybrid.published()) compare(hybrid.output);
        }
    }
}

int main() {
    using namespace Paper;
    verify_optional_postprocessing();
    for (size_t n = 128; n <= 16384; n *= 2) for (size_t hop : {1u, 37u, 257u, 65536u}) {
        for (bool live : {false, true}) {
            Config c{};
            c.backend = "pffft-hybrid-float"; c.n = n; c.hop = hop;
            c.rate = 48000; c.state = live ? "live" : "steady";
            ScheduledAnalysis<float, InspectBackend> adapter(c);
            adapter.fft.live = live;
            // H=1 must also wrap retention; long hops include idle quota samples.
            const size_t total = hop == 1 && n > 128 ? 4 : 2*n+3*hop;
            PaperResources::reset_phase(); PaperResources::active = true;
            for (size_t s = 0; s < total; ++s) {
                adapter.fft.endpoint = s/hop*hop;
                const size_t phase = s%hop;
                const size_t before = adapter.fft.calls;
                adapter.process(float(int64_t(s%23)-11));
                const size_t expected_cursor = (phase+1)*adapter.schedule.work/hop;
                require(adapter.cursor == expected_cursor, "Balanced task quota differs");
                const size_t fft_phase = ((n+1)*hop+adapter.schedule.work-1)/adapter.schedule.work-1;
                require(adapter.fft.calls-before == size_t(phase == fft_phase), "Opaque call schedule differs");
                require(adapter.published() == (phase == hop-1), "Early, late or missing publication");
                if (adapter.published() && !live) for (size_t k = 0; k < adapter.output.size(); ++k)
                    require(adapter.output[k] == float(k+1), "Postprocessing dependency or output store omitted");
            }
            PaperResources::active = false;
            require(PaperResources::counts.allocations == 0, "Hybrid execution allocated C++ storage");
        }
    }
    // Delay a read until after an undersized retained ring has been overwritten.
    Config c{}; c.n = 128; c.hop = 65536; c.rate = 48000;
    c.backend = "pffft-hybrid-float"; c.state = "steady";
    ScheduledAnalysis<float, InspectBackend> broken(c);
    broken.ring.resize(c.n);
    bool rejected = false;
    try { for (size_t s = 0; s < c.hop; ++s) broken.process(float(int64_t(s%23)-11)); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "Retained-input fixture failed to detect overwritten history");
}
