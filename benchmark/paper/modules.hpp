// Headless Fourier and Spectre module workloads.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULES_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULES_HPP_

#include "../../src/SpectrumAnalyzer.cpp"
#include "../../src/Spectrogram.cpp"
#include "protocol.hpp"

Plugin* plugin_instance = nullptr;
namespace Paper {
/// @brief Headless module context; no engine worker or display thread is started.
struct Context {
    rack::Context context;
    explicit Context(float rate) {
        rack::contextSet(&context);
        context.engine = new rack::engine::Engine;
        context.engine->setSampleRate(rate);
    }
    ~Context() { rack::contextSet(nullptr); }
};

void configure(SpectrumAnalyzer& module, const Config& c) {
    module.set_window_length(c.n);
    module.set_hop_length(c.hop);
    require(module.get_window_length() == c.n && module.get_hop_length() == c.hop,
        "Fourier panel quantized N/H; choose exactly representable settings");
}
void configure(Spectrogram& module, const Config& c) {
    require(c.n == Spectrogram::N_FFT && c.hop == Spectrogram::N_FFT/2,
        "Spectre has fixed N=2048 H=1024");
}

/// @brief Publication detection is consumer-side and only used by untimed audit.
struct Publication {
    const void* last = nullptr;
    uint32_t column = 0;
    bool poll(SpectrumAnalyzer& module) {
        const auto& snapshot = module.consume_display_spectrum();
        const bool changed = last != &snapshot && snapshot.count != 0;
        last = &snapshot;
        return changed;
    }
    bool poll(Spectrogram& module) {
        const auto next = module.get_hop_index();
        const bool changed = next != column;
        column = next;
        return changed;
    }
};
void check(SpectrumAnalyzer& module, size_t n) {
    const auto& snapshot = module.consume_display_spectrum();
    require(snapshot.count == n/2+1, "Wrong module spectrum size");
    // The existing logarithmic display maps zero magnitude to -infinity.
    // This is a coordinate audit, not the module's pending all-bin oracle.
    const bool logarithmic = module.get_magnitude_scale() != MagnitudeScale::Linear;
    for (const auto& lane : snapshot.points)
        for (size_t bin = 0; bin < snapshot.count; ++bin)
            require(std::isfinite(lane[bin].x) && (std::isfinite(lane[bin].y)
                || (logarithmic && lane[bin].y == -std::numeric_limits<float>::infinity())),
                "Invalid module spectrum coordinate");
}
void check(Spectrogram& module, size_t) {
    const auto index = (module.get_hop_index()+Spectrogram::N_STFT-1)%Spectrogram::N_STFT;
    const auto* column = module.consume_display_column(index, true);
    require(column && column->revision > 0, "Empty module history");
    for (auto value : column->values) require(std::isfinite(value), "Non-finite module history");
}

template<typename Module>
struct Host {
    Module module;
    Publication publication;
    rack::engine::Module::ProcessArgs args = {};
    Config config;
    std::vector<std::vector<float>> input_signals;
    size_t limit;
    size_t cursor = 0;
    size_t phase = 0, frames = 0;
    explicit Host(const Config& c) : config(c), limit(input_limit(c)) {
        configure(module, c);
        module.set_window_function(window_function(c));
        const float octave = octave_width(c);
        module.set_frequency_smoothing(octave == 0 ? FrequencySmoothing::None :
            octave == 1 ? FrequencySmoothing::_1_1 : octave == 2 ? FrequencySmoothing::_2_1 : FrequencySmoothing::_1_3);
        module.set_time_smoothing(c.workload_schema == 3 ? float(c.temporal_value) : c.smooth ? 0.1f : 0.f);
        for (size_t port = 0; port < module.inputs.size(); ++port) {
            module.inputs[port].channels = c.workload_schema < 3 || port < c.active_ports ? c.voices : 0;
            if (c.workload_schema == 3 && c.fixture == "independent") input_signals.push_back(signal(c, port));
        }
        args.sampleRate = c.rate;
        args.sampleTime = 1.f/c.rate;
        publication.poll(module);
    }
    void process(float value) {
        if (config.state == "live" && phase == 0) {
            module.set_window_function(frames++%2 ? Fourier::Window::Function::Hann
                                                 : Fourier::Window::Function::BlackmanHarris);
            module.set_frequency_smoothing(frames%2 ? FrequencySmoothing::None : FrequencySmoothing::_1_3);
        }
        for (size_t port = 0; port < module.inputs.size(); ++port)
            for (size_t voice = 0; voice < size_t(module.inputs[port].channels); ++voice) {
                const float input = input_signals.empty() ? value : input_sample(input_signals[port], cursor, limit);
                module.inputs[port].setVoltage(5.f*input*(1.f-0.15f*port)/config.voices, voice);
            }
        module.process(args);
        ++cursor;
        ++args.frame;
        phase = (phase+1)%config.hop;
    }
    size_t delay() const { return config.hop-1; }
    bool published() { return publication.poll(module); }
    void barrier() const { observe(module); }
    void check() { Paper::check(module, config.n); }
};

}  // namespace Paper

#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULES_HPP_
