// Benchmark-only batch/hybrid pair sharing one positive-bin analysis pipeline.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_HYBRID_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_HYBRID_HPP_

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
#include "external.hpp"

namespace Paper {
/// @brief Schedule surrounding scalar work while keeping the native FFT opaque.
/// @details W=N+1+2K dependency-ordered units: N window/input stores, one native
/// call (including provider packing/stores), K magnitude/prefix sums, K band/EMA
/// stores. Balanced quotas count tasks, not equal-cost operations or FFT steps.
/// Both modes retain N+H inputs and run identical arithmetic and cache updates.
/// Disabled smoothing skips prefix/band arithmetic without removing task units;
/// alpha=0 still stores the current magnitude so output history stays current.
/// The extra H slots protect the frame ending at jH until its last preparation
/// read; subsequent input never changes that frame. No boundary snapshot copy.
template<typename T, typename Backend>
struct ScheduledAnalysis {
    Config config;
    Backend fft;
    FrameSchedule schedule;
    std::vector<T> ring, frame, magnitudes, prefix, output;
    std::vector<std::complex<T>> coefficients;
    std::vector<float> window;
    std::vector<size_t> low, high;
    size_t head = 0, origin = 0, cursor = 0, frames = 0;
    bool bands, dirty = false;
    Fourier::Window::Function function = Fourier::Window::Function::Hann;
    float gain = 2.f, alpha;

    explicit ScheduledAnalysis(const Config& c) : config(c), fft(c.n, "analysis"),
        schedule(c, c.n+1+2*(c.n/2+1)), ring(c.n+c.hop), frame(c.n),
        magnitudes(c.n/2+1), prefix(c.n/2+2), output(c.n/2+1),
        coefficients(c.n/2+1), window(c.n), low(c.n/2+1), high(c.n/2+1), bands(octave_width(c) > 0),
        alpha(temporal_alpha(c)) {
        function = window_function(c);
        gain = 1.f/Fourier::Window::coherent_gain(function);
        // Dispatch already consumed the name; avoid retaining unequal label
        // allocations in a pair whose persistent DSP storage is matched.
        std::string().swap(config.backend);
        for (size_t i = 0; i < c.n; ++i) prepare_window(i);
        if (bands) for (size_t k = 0; k < low.size(); ++k) prepare_band(k);
    }
    void prepare_window(size_t i) {
        window[i] = gain*Fourier::Window::window<float>(function, float(i), float(config.n), false);
    }
    void prepare_band(size_t k) {
        const float octave = config.workload_schema == 3 ? octave_width(config) : 1.f/3.f;
        const float half = std::pow(2.f, octave/2.f), ratio = std::pow(2.f, octave);
        const double width = double(config.rate)/config.n, maximum = double(config.rate)/2;
        double a = k*width/half, b = k*width*half;
        if (b > maximum) { b = maximum; a = b/ratio; }
        low[k] = size_t(std::floor(a/width));
        high[k] = std::min(config.n/2, size_t(std::floor(b/width)));
    }
    /// @brief One ordered task; deliberately no timer or per-stage instrumentation.
    void unit() {
        const size_t n = config.n, k_count = output.size();
        if (cursor < n) {
            if (dirty) prepare_window(cursor);
            frame[cursor] = ring[(origin+cursor)%ring.size()]*window[cursor];
        } else if (cursor == n) {
            fft.forward_real_positive(frame.data(), coefficients.data());
        } else if (cursor < n+1+k_count) {
            const size_t k = cursor-n-1;
            magnitudes[k] = std::abs(coefficients[k]);
            if (bands) prefix[k+1] = prefix[k]+magnitudes[k];
        } else {
            const size_t k = cursor-n-1-k_count;
            if (bands && dirty) prepare_band(k);
            const T magnitude = bands ? (prefix[high[k]+1]-prefix[low[k]])/T(high[k]-low[k]+1)
                                      : magnitudes[k];
            output[k] = alpha == 0.f ? magnitude : alpha*output[k]+(1.f-alpha)*magnitude;
        }
        ++cursor;
    }
    void process(float value) {
        ring[head] = T(value); head = (head+1)%ring.size();
        if (schedule.phase == 0) {
            origin = (head+ring.size()-config.n)%ring.size();
            cursor = 0; prefix[0] = 0;
            dirty = config.state == "live";
            if (dirty) {
                function = frames++%2 ? Fourier::Window::Function::Hann
                    : Fourier::Window::Function::BlackmanHarris;
                bands = frames%2 == 0;
                gain = 1.f/Fourier::Window::coherent_gain(function);
            }
        }
        const size_t quota = schedule.quota();
        for (size_t i = 0; i < quota; ++i) unit();
        schedule.complete = quota && cursor == schedule.work;
        schedule.advance();
    }
    size_t delay() const { return schedule.delay(); }
    bool published() const { return schedule.complete; }
    void barrier() const { observe(output.data()); }
    void check() const { for (auto v : output) require(std::isfinite(v), "Invalid hybrid analysis output"); }
    /// @brief Describe the actual adapter, outside measurement and allocation probes.
    std::string info_json() const {
        std::string native = fft.info_json();
        native.pop_back();
        std::ostringstream out;
        out << native << ",\"analysis_schedule\":{\"mode\":\""
            << (schedule.batch ? "batch" : "hybrid") << "\",\"task_units\":" << schedule.work
            << ",\"prepare_units\":" << config.n << ",\"native_calls_per_frame\":1"
            << ",\"magnitude_units\":" << output.size() << ",\"output_units\":" << output.size()
            << ",\"retained_input_samples\":" << ring.size()
            << ",\"fft_sample_offset\":" << (schedule.batch ? 0 : ((config.n+1)*config.hop+schedule.work-1)/schedule.work-1)
            << ",\"publication_delay_samples\":" << delay()
            << ",\"cost_model\":\"unequal tasks; native FFT including conversion is indivisible\"}}";
        return out.str();
    }
};

template<typename T, typename Backend>
struct HybridAudit : ExternalAudit<T, Backend> {
    using ExternalAudit<T, Backend>::ExternalAudit;
    void operator()(const ScheduledAnalysis<T, Backend>& adapter, const std::vector<float>& input, size_t sample) {
        this->analysis(adapter, input, sample);
    }
};

template<typename T, typename Backend>
void hybrid_dispatch(const Config& c, bool provider_info) {
    if (provider_info) { ScheduledAnalysis<T, Backend> adapter(c); std::cout << adapter.info_json() << '\n'; return; }
    SynthesisAccuracy accuracy;
    std::vector<std::string> instances;
    stream<ScheduledAnalysis<T, Backend>>(c, HybridAudit<T, Backend>(accuracy, instances));
    if (!c.resources) {
        require(accuracy.checked && accuracy.publications, "Missing hybrid numerical audit");
        accuracy.print("direct DFT or binary64 FFT of matched frame bytes; direct band sums and EMA", instances);
    }
}

/// @brief Startup, retention wraparound, live caches, idle quotas and H=1.
template<typename Backend>
void verify_hybrid() {
    const auto input = signal();
    for (size_t n : {128u, 2048u, 16384u}) for (size_t hop : {1u, 37u, 65536u}) {
        Config c{};
        c.n = n; c.hop = hop; c.rate = 48000; c.smooth = true; c.state = "live";
        c.backend = "pffft-hybrid-float";
        ScheduledAnalysis<float, Backend> hybrid(c);
        c.backend = "pffft-scheduled-batch-float";
        ScheduledAnalysis<float, Backend> batch(c);
        c.backend = "pffft-analysis-float";
        ExternalAnalysis<float, Backend> immediate(c);
        SynthesisAccuracy accuracy;
        std::vector<std::string> instances;
        HybridAudit<float, Backend> audit(accuracy, instances);
        const size_t samples = hop == 1 ? 4 : 2*n+4*hop;
        std::vector<float> expected(n/2+1);
        for (size_t sample = 0; sample < samples; ++sample) {
            const float value = input[sample%input.size()];
            batch.process(value); immediate.process(value); hybrid.process(value);
            if (batch.published()) {
                expected = batch.output;
                for (size_t k = 0; k < expected.size(); ++k) {
                    require(std::abs(expected[k]-immediate.output[k]) < 1e-4f*std::max(1.f, std::abs(expected[k])),
                        "Hybrid batch differs from immediate baseline");
                }
            }
            require(hybrid.published() == (sample%hop == hop-1), "Hybrid publication cadence");
            if (hybrid.published()) require(hybrid.output == expected, "Hybrid retention or schedule changed arithmetic");
            audit(hybrid, input, sample);
        }
        require(accuracy.checked > 0, "Hybrid verification omitted outputs");
    }
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_HYBRID_HPP_
