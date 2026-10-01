// Independent all-output module replay, outside every measured interval.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULE_AUDIT_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULE_AUDIT_HPP_
#include <map>
#include <memory>
#include <vector>
#include "modules.hpp"
#include "analysis_reference.hpp"
#include "synthesis.hpp"

namespace Paper {
/// @brief Preserve effective panel values, including the hop's float seconds.
template<typename Module>
std::string module_settings_json(Module& module, const Config& c) {
    std::ostringstream out; out.precision(17);
    out << "{\"policy\":\"module-controls-v1\",\"backend\":\"" << c.backend
        << "\",\"n\":" << c.n << ",\"hop\":" << module.get_hop_length()
        << ",\"rate\":" << module.get_sample_rate() << ",\"window_id\":" << int(module.get_window_function())
        << ",\"time_seconds\":" << module.get_time_smoothing()
        << ",\"alpha\":" << module.get_time_smoothing_alpha()
        << ",\"octave_id\":" << int(module.get_frequency_smoothing())
        << ",\"ac_coupled\":" << (module.is_ac_coupled ? "true" : "false")
        << ",\"active_ports\":" << (c.workload_schema == 3 ? c.active_ports : module.inputs.size())
        << ",\"voices_per_active_port\":" << c.voices << ",\"snapshot_metadata_instrumentation\":true,\"params\":[";
    for (size_t i = 0; i < module.params.size(); ++i) { if (i) out << ','; out << module.params[i].getValue(); }
    out << "]}"; return out.str();
}

/// @brief Copy geometry at frame start, independently of the coordinate cache.
struct ModuleGeometry {
    double rate, low, high, slope;
    FrequencyScale frequency;
    MagnitudeScale magnitude;
};
inline ModuleGeometry module_geometry(SpectrumAnalyzer& m) {
    return {m.get_sample_rate(), m.get_low_frequency(), m.get_high_frequency(), m.get_slope(),
        m.get_frequency_scale(), m.get_magnitude_scale()};
}
inline ModuleGeometry module_geometry(Spectrogram&) { return {}; }

/// @brief Invert display scaling independently, checking every x coordinate.
/// @details Error norms stay in unnormalized FFT-magnitude units. Comparing
/// logarithms directly would over-weight tiny float leakage near spectral zeros.
inline std::vector<float> module_magnitudes(const SpectrumAnalyzer::DisplaySpectrum& snapshot,
        const ModuleGeometry& geometry, size_t channel, size_t n) {
    const size_t bins = n/2+1;
    require(snapshot.count == bins, "Missing Fourier display bins");
    std::vector<float> values(bins);
    for (size_t k = 0; k < bins; ++k) {
        const double u = double(k)/bins, nyquist = geometry.rate/2.;
        const double low = geometry.low/nyquist;
        const double extent = (geometry.high-geometry.low)/nyquist;
        double x = (u-low)/(extent == 0 ? 1 : extent);
        if (geometry.frequency == FrequencyScale::Logarithmic) x = std::copysign(std::sqrt(std::abs(x)), x);
        const auto point = snapshot.points[channel][k];
        require(std::isfinite(point.x) && std::abs(point.x-x) <= 2e-6*std::max(1., std::abs(x)), "Fourier frequency mapping differs");
        const double slope = geometry.slope;
        const double gain = std::pow(10., slope*std::log2(u*nyquist/1000.+std::numeric_limits<float>::epsilon())/20.);
        double magnitude = point.y;
        if (geometry.magnitude != MagnitudeScale::Linear) {
            const double range = geometry.magnitude == MagnitudeScale::Logarithmic60dB ? 72 : 132;
            magnitude = std::pow(10., (magnitude-1)*range/20.);
        }
        values[k] = magnitude*std::pow(10., 12./20.)*bins/gain;
    }
    return values;
}
inline std::vector<float> module_magnitudes(SpectrumAnalyzer& module, size_t channel, size_t n) {
    return module_magnitudes(module.consume_display_spectrum(), module_geometry(module), channel, n);
}
inline std::vector<float> module_magnitudes(Spectrogram& module, size_t, size_t) {
    const size_t index = (module.get_hop_index()+Spectrogram::N_STFT-1)%Spectrogram::N_STFT;
    const auto* column = module.consume_display_column(index, true);
    require(column && column->revision, "Missing Spectre history column");
    return {column->values.begin(), column->values.end()};
}

/// @brief Independent recurrence for normalization, double DC filter and gain.
/// No production filter, FFT, smoother or coordinate helper serves as oracle.
template<typename Module>
struct ModuleReference {
    Config config;
    struct Lane {
        long double previous = 0, filtered = 0;
        std::vector<float> conditioned;
        std::unique_ptr<AnalysisReference<float>> spectrum;
    };
    std::vector<Lane> lanes;
    size_t prepared = 0;
    ModuleReference(Host<Module>& host) : config(host.config), lanes(host.module.inputs.size()) {
        Config oracle = config; oracle.backend = "core-float";
        oracle.workload_schema = 3; oracle.fixture = "mixed";
        oracle.temporal_mode = "alpha"; oracle.temporal_value = host.module.get_time_smoothing_alpha();
        // V2's smooth flag belongs to the old wrapper; record actual module controls.
        oracle.octave = octave_width(config);
        for (auto& lane : lanes) lane.spectrum.reset(new AnalysisReference<float>(oracle));
    }
    void advance(Host<Module>& host, const std::vector<float>& input, size_t endpoint) {
        const long double pole = 1.L-20.L/config.rate, gain = (1+pole)/2;
        for (; prepared <= endpoint; ++prepared) for (size_t c = 0; c < lanes.size(); ++c) {
            auto& lane = lanes[c];
            const float raw = host.input_signals.empty() ? input_sample(config, input, prepared)
                : input_sample(host.input_signals[c], prepared, host.limit);
            const float voltage = 5.f*raw*(1.f-.15f*c)/config.voices;
            float sum = 0;
            for (size_t voice = 0; voice < size_t(host.module.inputs[c].channels); ++voice) sum += voltage;
            const float normalized = sum/5.f;
            lane.filtered = gain*(normalized-lane.previous)+pole*lane.filtered;
            lane.previous = normalized;
            const float value = host.module.is_ac_coupled ? float(lane.filtered) : normalized;
            lane.conditioned.push_back(value*host.module.params[Module::PARAM_INPUT_GAIN+c].getValue());
        }
        for (auto& lane : lanes) lane.spectrum->advance(lane.conditioned, endpoint);
    }
};

/// @brief Poll only the replay's consumer, once per processed sample.
template<typename Module>
struct ModuleAudit {
    SynthesisAccuracy& accuracy;
    std::vector<std::string>& controls;
    std::map<const void*, std::unique_ptr<ModuleReference<Module>>> references;
    ModuleAudit(SynthesisAccuracy& a, std::vector<std::string>& c) : accuracy(a), controls(c) {}
    ModuleAudit(ModuleAudit&&) = default;
    void timed_instance(const Host<Module>& host) {
        // Rack's control getters are non-const but only read module state.
        controls.push_back(module_settings_json(const_cast<Module&>(host.module), host.config));
    }
    void operator()(Host<Module>& host, const std::vector<float>& input, size_t sample) {
        if (!references.count(&host)) references[&host].reset(new ModuleReference<Module>(host));
        const bool ready = sample%host.config.hop == host.delay();
        require(host.published() == ready, "Module publication cadence differs");
        if (!ready) return;
        const size_t endpoint = sample-host.delay();
        auto& reference = *references[&host]; reference.advance(host, input, endpoint);
        for (size_t channel = 0; channel < reference.lanes.size(); ++channel) {
            const auto actual = module_magnitudes(host.module, channel, host.config.n);
            accuracy.analysis.compare(reference.lanes[channel].spectrum->expected,
                [&](size_t k) { return actual[k]; }, endpoint, channel);
            accuracy.checked += actual.size();
        }
        ++accuracy.publications;
        accuracy.maximum_error = accuracy.analysis.maximum_error;
        accuracy.maximum_reference = accuracy.analysis.maximum_reference;
    }
};

template<typename Module>
void module_stream(const Config& c) {
    SynthesisAccuracy accuracy; std::vector<std::string> controls;
    stream<Host<Module>>(c, ModuleAudit<Module>(accuracy, controls));
    if (!c.resources) accuracy.print("independent input/DC recurrence, binary64 FFT, direct bands/EMA and inverse display mapping", {},
        ",\"module_policy\":\"all-module-outputs-v1\",\"module_controls\":["+[&]() {
            std::string values; for (const auto& control : controls) { if (!values.empty()) values += ','; values += control; } return values;
        }()+"]");
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_MODULE_AUDIT_HPP_
