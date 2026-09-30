// Explicit version-3 workload controls; historical defaults remain version 2.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_WORKLOAD_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_WORKLOAD_HPP_
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include "../../src/dsp/window.hpp"

namespace Paper {
struct WorkloadControls {
    size_t workload_schema = 2;
    std::string window = "hann", fixture = "mixed", temporal_mode = "alpha";
    std::string execution_regime = "continuous", experimental_policy = "existing";
    double octave = 0, temporal_value = 0;
    size_t fixture_seed = 305419896, decay_samples = 4096, active_ports = 1;
};

template<typename Config> Fourier::Window::Function window_function(const Config& c) {
    if (c.workload_schema < 3 || c.window == "hann") return Fourier::Window::Function::Hann;
    if (c.window == "blackman-harris") return Fourier::Window::Function::BlackmanHarris;
    if (c.window == "boxcar") return Fourier::Window::Function::Boxcar;
    if (c.window == "flattop") return Fourier::Window::Function::Flattop;
    throw std::runtime_error("Unsupported workload window");
}
template<typename Config> float octave_width(const Config& c) {
    return c.workload_schema == 3 ? float(c.octave) : c.smooth ? 1.f/3.f : 0.f;
}
/// @brief Module time knobs use exp(-10 * hop_seconds / knob_seconds), not a tau.
template<typename Config> float temporal_alpha(const Config& c) {
    if (c.workload_schema < 3) return c.smooth ? 0.8f : 0.f;
    if (c.temporal_mode == "alpha") return float(c.temporal_value);
    return c.temporal_value == 0 ? 0.f : std::exp(-10.f*(float(c.hop)/c.rate)/float(c.temporal_value));
}

template<typename Config, typename Descriptor>
void validate_workload_controls(const Config& c, const Descriptor& d) {
    if (c.workload_schema == 2) return;
    auto check = [](bool valid) { if (!valid) throw std::runtime_error("Unsupported version-3 workload controls"); };
    check(c.workload_schema == 3 && !c.smooth && c.state != "live" && c.transition_suite.empty());
    const std::string boundary(d.boundary), kind(d.kind);
    check((boundary == "analysis" || boundary == "module" || boundary == "control") && kind != "core4");
    window_function(c);
    check(std::isfinite(c.octave) && c.octave >= 0 && c.octave <= 2.5 &&
        std::isfinite(c.temporal_value) && c.temporal_value >= 0 &&
        ((c.temporal_mode == "alpha" && c.temporal_value < 1) ||
         (c.temporal_mode == "module-seconds" && c.temporal_value <= 10)));
    check(temporal_alpha(c) >= 0 && temporal_alpha(c) < 1);
    check(c.fixture_seed <= UINT32_MAX && c.decay_samples > 0 && c.decay_samples <= 1000000000);
    check(c.fixture == "mixed" || c.fixture == "silence" || c.fixture == "decay" ||
        c.fixture == "impulse" || c.fixture == "dc" || c.fixture == "nyquist" ||
        c.fixture == "off-bin" || c.fixture == "weak" || c.fixture == "noise" || c.fixture == "independent");
    check(c.experimental_policy == "existing" && (c.execution_regime == "continuous" ||
        (c.execution_regime == "paced" && c.pass == "callback")));
    check(c.active_ports >= 1 && c.active_ports <= size_t(d.channels));
    check(boundary == "module" || c.active_ports == size_t(d.channels));
    check(c.fixture != "independent" || d.channels > 1);
    if (boundary == "module") {
        check(c.temporal_mode == "module-seconds" && c.temporal_value <= 2.5);
        // These are exact public panel settings; other widths fail explicitly.
        check(c.octave == 0 || c.octave == 1./3 || c.octave == 1 || c.octave == 2);
        if (std::string(d.operation) == "shipped-default")
            check(c.window == "flattop" && c.octave == 0 && c.temporal_value == 0 && c.rate == 48000);
    }
}

/// @brief Requested controls plus effective binary32 arithmetic in the contract.
template<typename Config> std::string workload_controls_json(const Config& c) {
    std::ostringstream out; out.precision(17);
    out << "{\"workload_schema\":3,\"window\":\"" << c.window << "\",\"octave\":" << c.octave
        << ",\"temporal_mode\":\"" << c.temporal_mode << "\",\"temporal_value\":" << c.temporal_value
        << ",\"fixture\":\"" << c.fixture << "\",\"fixture_seed\":" << c.fixture_seed
        << ",\"decay_samples\":" << c.decay_samples << ",\"active_ports\":" << c.active_ports
        << ",\"execution_regime\":\"" << c.execution_regime
        << "\",\"experimental_policy\":\"" << c.experimental_policy << "\"}";
    return out.str();
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_WORKLOAD_HPP_
