// Untimed independent module reference through serialized lifecycle events.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULE_LIFECYCLE_REFERENCE_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULE_LIFECYCLE_REFERENCE_HPP_
#include "module_audit.hpp"
namespace Paper {
inline size_t module_length(SpectrumAnalyzer& m) { return m.get_window_length(); }
inline size_t module_length(Spectrogram&) { return Spectrogram::N_FFT; }
inline bool freeze_keeps_processing(SpectrumAnalyzer&) { return true; }
inline bool freeze_keeps_processing(Spectrogram&) { return false; }
inline double decay_floor(Spectrogram&, size_t n) {
    return 64. * n * std::numeric_limits<float>::min();
}
inline double decay_floor(SpectrumAnalyzer& m, size_t n) {
    const double slope = m.get_slope(), bins = n/2+1;
    const double low = std::pow(10., slope*std::log2(std::numeric_limits<float>::epsilon())/20.);
    const double high = std::pow(10., slope*std::log2((n/2)/bins*m.get_sample_rate()/2000.
        + std::numeric_limits<float>::epsilon())/20.);
    // Inverting a flushed display ordinate magnifies its absolute floor.
    const double display = 2*std::pow(10., 12./20.)*bins/std::min(low, high);
    return std::max(64.*n, display)*std::numeric_limits<float>::min();
}
inline std::string module_window_name(Fourier::Window::Function f) {
    if (f == Fourier::Window::Function::Hann) return "hann";
    if (f == Fourier::Window::Function::Flattop) return "flattop";
    if (f == Fourier::Window::Function::BlackmanHarris) return "blackman-harris";
    if (f == Fourier::Window::Function::Boxcar) return "boxcar";
    throw std::runtime_error("Module oracle window unsupported");
}
/// @brief Reference capture and frame clocks are independent of the tested scheduler.
/// @details History contains captured inputs, not processing calls. Fourier
/// continues analysis while capture is frozen; Spectre pauses both clocks.
template<typename Module>
struct ModuleLifecycleReference {
    Host<Module>& host;
    struct Lane {
        long double previous = 0, filtered = 0;
        std::vector<float> history;
        std::unique_ptr<AnalysisReference<float>> spectrum;
    };
    std::vector<Lane> lanes;
    size_t phase = 0, hop = 1, endpoint = 0;
    int64_t endpoint_frame = -1, last_capture = -1;
    bool running = true, immediate, native;
    SynthesisAccuracy accuracy;
    ModuleGeometry geometry{};
    explicit ModuleLifecycleReference(Host<Module>& h, bool batch = false, bool native_ = false)
        : host(h), lanes(h.module.inputs.size()), immediate(batch), native(native_) {
        if (h.config.fixture == "decay")
            accuracy.analysis.decay_floor = decay_floor(h.module, h.config.n);
        reset(false);
    }
    void reset(bool resume = true) {
        if (resume) running = true;
        phase = 0; endpoint_frame = last_capture = -1;
        for (auto& lane : lanes) {
            lane.previous = lane.filtered = 0; lane.history.clear();
            Config c = host.config; c.backend = "core-float"; c.hop = 1; c.fixture = "mixed";
            c.state = "steady"; c.window = module_window_name(host.module.get_window_function());
            c.temporal_mode = "alpha"; c.temporal_value = host.module.get_time_smoothing_alpha();
            lane.spectrum.reset(new AnalysisReference<float>(c)); lane.spectrum->float_intervals = !native;
        }
    }
    void change(const std::string& kind) {
        if (kind == "reset") reset();
        else if (kind == "sample-rate") reset(false);
        else if (kind == "freeze" || kind == "resume") running = kind == "resume";
    }
    bool step(const std::vector<float>& source, size_t source_sample, int64_t frame) {
        auto& module = host.module;
        if (!running && !freeze_keeps_processing(module)) return false;
        const long double pole = 1.L-20.L/module.get_sample_rate(), gain = (1+pole)/2;
        for (size_t c = 0; running && c < lanes.size(); ++c) {
            auto& lane = lanes[c];
            const float raw = host.input_signals.empty() ? input_sample(host.config, source, source_sample)
                : input_sample(host.input_signals[c], source_sample, host.limit);
            const float voltage = 5.f*raw*(1.f-.15f*c)/host.config.voices;
            float sum = 0; for (size_t v = 0; v < size_t(module.inputs[c].channels); ++v) sum += voltage;
            const float normalized = sum/5.f;
            lane.filtered = gain*(normalized-lane.previous)+pole*lane.filtered; lane.previous = normalized;
            if (running) lane.history.push_back((module.is_ac_coupled ? float(lane.filtered) : normalized)
                *module.params[Module::PARAM_INPUT_GAIN+c].getValue());
        }
        if (running) last_capture = frame;
        if (phase == 0) {
            hop = module.get_hop_length(); endpoint_frame = last_capture;
            geometry = module_geometry(module);
            endpoint = lanes[0].history.empty() ? 0 : lanes[0].history.size()-1;
            for (auto& lane : lanes) {
                auto& c = lane.spectrum->config;
                c.rate = module.get_sample_rate(); c.window = module_window_name(module.get_window_function());
                c.temporal_value = module.get_time_smoothing_alpha();
                c.octave = module.get_frequency_smoothing() == FrequencySmoothing::None ? 0.f : to_float(module.get_frequency_smoothing());
            }
        }
        const bool complete = phase == (immediate ? 0 : hop-1);
        phase = (phase+1)%hop;
        if (complete) for (auto& lane : lanes) {
            auto& reference = *lane.spectrum; reference.next_endpoint = endpoint;
            const std::vector<float> zero(1, 0);
            reference.advance(lane.history.empty() ? zero : lane.history, endpoint);
        }
        return complete;
    }
    void compare(size_t channel, const std::vector<float>& values) {
        const auto& expected = lanes[channel].spectrum->expected;
        require(values.size() == expected.size(), "Missing lifecycle spectrum bins");
        try { accuracy.analysis.compare(expected, [&](size_t k) { return values[k]; }, endpoint, channel); }
        catch (const std::exception& e) { throw std::runtime_error(std::string(e.what())+" "+accuracy.analysis.json()); }
        accuracy.checked += values.size();
    }
};
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULE_LIFECYCLE_REFERENCE_HPP_
