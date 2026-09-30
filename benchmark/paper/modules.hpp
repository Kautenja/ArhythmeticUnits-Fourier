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
    for (const auto& lane : snapshot.points)
        require(std::isfinite(lane[7].y), "Empty module spectrum");
}
void check(Spectrogram& module, size_t) {
    const auto index = (module.get_hop_index()+Spectrogram::N_STFT-1)%Spectrogram::N_STFT;
    const auto* column = module.consume_display_column(index, true);
    require(column && column->revision > 0 && column->values[7] > 0, "Empty module history");
}

template<typename Module>
struct Host {
    Module module;
    Publication publication;
    rack::engine::Module::ProcessArgs args = {};
    Config config;
    size_t phase = 0, frames = 0;
    explicit Host(const Config& c) : config(c) {
        configure(module, c);
        module.set_window_function(Fourier::Window::Function::Hann);
        module.set_frequency_smoothing(c.smooth ? FrequencySmoothing::_1_3 : FrequencySmoothing::None);
        module.set_time_smoothing(c.smooth ? 0.1f : 0.f);
        for (auto& input : module.inputs) input.channels = c.voices;
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
            for (size_t voice = 0; voice < config.voices; ++voice)
                module.inputs[port].setVoltage(5.f*value*(1.f-0.15f*port)/config.voices, voice);
        module.process(args);
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
