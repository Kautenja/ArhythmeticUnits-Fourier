// Complete benchmark-only native module paths with production conditioning/sinks.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_MODULES_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_MODULES_HPP_
#include "module_audit.hpp"
#include "native_analysis.hpp"

namespace Paper {
/// @brief Preserve the actual module's input/control code, plus the native pipeline.
/// @details The unused analyzer/display storage in the control shell is retained
/// and disclosed. Timing includes conditioning, complete output and publication;
/// this is not an exact storage ablation against the shipped module.
template<typename Kernel>
struct NativeFourier {
    using Module = SpectrumAnalyzer;
    Host<SpectrumAnalyzer> host;
    std::unique_ptr<NativeAnalysis<Kernel>> analysis;
    std::string backend;
    Fourier::CachedSpectrumCoordinates<SpectrumAnalyzer::MAX_FFT/2+1> coordinates;
    Fourier::DisplayMailbox<SpectrumAnalyzer::DisplaySpectrum> mailbox;
    uint64_t sequence = 0;
    int64_t endpoint = -1, last_capture = -1;
    bool running = true;
    static Config native_config(Config c, const std::string& backend) { c.backend = backend; return c; }
    NativeFourier(const Config& c, const std::string& backend) : host(c), analysis(new NativeAnalysis<Kernel>(native_config(c, backend))), backend(backend) {
        static_assert(Kernel::channels == 4, "Fourier requires four independent native channels");
    }
    void process(const rack::engine::Module::ProcessArgs& args, float value) {
        auto& module = host.module;
        for (size_t port = 0; port < module.inputs.size(); ++port)
            for (size_t voice = 0; voice < size_t(module.inputs[port].channels); ++voice) {
                const float input = host.input_signals.empty() ? value : input_sample(host.input_signals[port], host.cursor, host.limit);
                module.inputs[port].setVoltage(5.f*input*(1.f-.15f*port)/host.config.voices, voice);
            }
        module.process_run_button();
        const auto input = module.process_input_signal();
        if (running) last_capture = args.frame;
        if (analysis->schedule.phase == 0) {
            endpoint = last_capture;
            const auto band = module.get_frequency_smoothing();
            analysis->set_controls(module.get_window_function(), band == FrequencySmoothing::None ? 0.f : to_float(band),
                                  module.get_time_smoothing_alpha(), args.sampleRate);
            Fourier::SpectrumCoordinates settings;
            settings.bins = analysis->bins_count; settings.sample_rate = args.sampleRate;
            settings.low_frequency = module.get_low_frequency(); settings.high_frequency = module.get_high_frequency();
            settings.slope = module.get_slope(); settings.frequency_scale = module.get_frequency_scale();
            settings.magnitude_scale = module.get_magnitude_scale(); coordinates.configure(settings);
        }
        NoStageObserver observer;
        auto sink = [&](size_t channel, size_t bin, float) {
            if (channel != 3) return;
            rack::simd::float_4 magnitudes;
            for (size_t lane = 0; lane < 4; ++lane) magnitudes[lane] = analysis->output[lane*analysis->bins_count+bin];
            const auto points = coordinates.map(bin, magnitudes);
            for (size_t lane = 0; lane < 4; ++lane) mailbox.writable().points[lane][bin] = points[lane];
        };
        analysis->process_inputs([&](size_t lane) { return input[lane]; }, observer, sink, running);
        if (analysis->published()) {
            auto& snapshot = mailbox.writable(); snapshot.count = analysis->bins_count;
            snapshot.sequence = ++sequence; snapshot.endpoint = endpoint; snapshot.published_at = args.frame;
            mailbox.publish();
        }
        module.process_lights(args); ++host.cursor;
    }
    void reset_analysis(bool clear_display = true) {
        host.config.rate = host.module.get_sample_rate(); host.config.hop = host.module.get_hop_length();
        analysis.reset(new NativeAnalysis<Kernel>(native_config(host.config, backend)));
        running = true; last_capture = endpoint = -1;
        if (clear_display) {
            auto& snapshot = mailbox.writable(); snapshot.count = 0; snapshot.sequence = ++sequence;
            snapshot.endpoint = snapshot.published_at = -1; mailbox.publish();
        }
    }
    const SpectrumAnalyzer::DisplaySpectrum& consume() { mailbox.consume(); return mailbox.current(); }
};

template<typename Kernel>
struct NativeSpectre {
    using Module = Spectrogram;
    Host<Spectrogram> host;
    std::unique_ptr<NativeAnalysis<Kernel>> analysis;
    std::string backend;
    std::unique_ptr<Fourier::DisplayMailbox<Spectrogram::DisplayColumn>[]> mailboxes{
        new Fourier::DisplayMailbox<Spectrogram::DisplayColumn>[Spectrogram::N_STFT]};
    size_t column = 0;
    uint64_t sequence = 0;
    int64_t endpoint = -1;
    bool running = true;
    Fourier::ThresholdTrigger<float> trigger;
    Fourier::TriggerDivider lights;
    static Config native_config(Config c, const std::string& backend) { c.backend = backend; return c; }
    NativeSpectre(const Config& c, const std::string& backend) : host(c), analysis(new NativeAnalysis<Kernel>(native_config(c, backend))), backend(backend) {
        static_assert(Kernel::channels == 1, "Spectre requires one native channel");
        lights.setDivision(512);
        for (size_t i = 0; i < Spectrogram::N_STFT; ++i) {
            mailboxes[i].writable().revision = ++sequence;
            mailboxes[i].publish();
        }
    }
    void process(const rack::engine::Module::ProcessArgs& args, float value) {
        auto& module = host.module;
        for (size_t voice = 0; voice < size_t(module.inputs[0].channels); ++voice)
            module.inputs[0].setVoltage(5.f*value/host.config.voices, voice);
        if (trigger.process(module.params[Spectrogram::PARAM_RUN].getValue())) running = !running;
        if (running) {
            const float input = module.process_input_signal();
            if (analysis->schedule.phase == 0) {
                endpoint = args.frame;
                const auto band = module.get_frequency_smoothing();
                analysis->set_controls(module.get_window_function(), band == FrequencySmoothing::None ? 0.f : to_float(band),
                                      module.get_time_smoothing_alpha(), args.sampleRate);
            }
            NoStageObserver observer;
            auto sink = [&](size_t, size_t bin, float magnitude) { mailboxes[column].writable().values[bin] = magnitude; };
            analysis->process_inputs([&](size_t) { return input; }, observer, sink);
            if (analysis->published()) {
                auto& snapshot = mailboxes[column].writable();
                snapshot.revision = ++sequence; snapshot.endpoint = endpoint; snapshot.published_at = args.frame;
                mailboxes[column].publish(); column = (column+1)%Spectrogram::N_STFT;
            }
        }
        if (lights.process()) module.lights[Spectrogram::LIGHT_RUN].setSmoothBrightness(running, args.sampleTime*lights.getDivision());
        ++host.cursor;
    }
    void reset_analysis(bool clear_display = true) {
        host.config.rate = host.module.get_sample_rate(); host.config.hop = host.module.get_hop_length();
        analysis.reset(new NativeAnalysis<Kernel>(native_config(host.config, backend)));
        running = true; endpoint = -1; trigger.reset(); lights.reset();
        if (!clear_display) return;
        column = 0;
        for (size_t i = 0; i < Spectrogram::N_STFT; ++i) {
            auto& snapshot = mailboxes[i].writable(); snapshot.values.fill(0);
            snapshot.revision = ++sequence; snapshot.endpoint = snapshot.published_at = -1; mailboxes[i].publish();
        }
    }
    const Spectrogram::DisplayColumn* consume(size_t index) { return mailboxes[index].consume(); }
};
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_MODULES_HPP_
