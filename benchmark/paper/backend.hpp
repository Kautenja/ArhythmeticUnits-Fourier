// Explicit backend capabilities and resolved measurement contracts.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_BACKEND_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_BACKEND_HPP_
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include "workload.hpp"

namespace Paper {
#include "registry.generated.hpp"

inline const BackendDescriptor& backend_descriptor(const std::string& name) {
    for (const auto& item : backend_registry) if (name == item.id) {
        if (!item.available) throw std::runtime_error("Unavailable backend " + name + ": " + item.reason);
        return item;
    }
    throw std::runtime_error("Unknown backend: " + name);
}

/// @brief Validate capability combinations before allocating or measuring.
template<typename Config>
void validate_backend(const Config& c) {
    const auto& d = backend_descriptor(c.backend);
    validate_workload_controls(c, d);
    auto check = [](bool valid) { if (!valid) throw std::runtime_error("Unsupported backend workload"); };
    check(c.n >= d.size_min && c.n <= d.size_max && c.n%d.size_multiple == 0
        && (!d.power_of_two || !(c.n&(c.n-1))) && (!d.fixed_n || c.n == d.fixed_n)
        && (!d.fixed_hop || c.hop == d.fixed_hop));
    check(c.voices <= d.max_voices && (!c.smooth || d.smoothing) && (c.state != "live" || d.live));
    check(std::string(d.operation) != "shipped-default" || c.workload_schema == 3);
    const std::string kind(d.kind), boundary(d.boundary), model(d.step_model);
    if (!c.transition_suite.empty()) {
        check(c.backend == "core-float" || c.backend == "core-double"
            || c.backend == "pffft-hybrid-float" || c.backend == "pffft-scheduled-batch-float");
        check(boundary == "analysis" && d.channels == 1 && c.pass == "callback"
            && c.state == "startup" && c.count == 1 && c.alignment == "aligned"
            && c.callback_offset == 0 && c.warm_hops == 0);
    }
    check(c.callback_offset < c.hop && (c.state != "startup" || c.callback_offset == 0));
    if (boundary == "transform") check(c.callback_offset == 0);
    if (boundary == "chain") check(c.hop <= c.n-2);
    if (boundary == "transform") {
        check(c.pass == "complete" || ((model == "radix2-real" || model == "radix2-complex" || model == "radix2-inverse") &&
            (c.pass == "incremental" || c.pass == "phases" || c.pass == "steps")));
        check(c.count == 1 && !c.load && !c.cache_mib && !c.smooth
            && c.state == "steady" && c.alignment == "aligned");
    } else {
        check(c.pass == "callback" || c.pass == "throughput");
        check(c.callbacks*c.block >= 2*c.hop);
        check(c.state != "startup" || c.alignment == "aligned");
    }
}

struct BackendContract {
    size_t delay = 0, outputs = 0, steps = 0;
    double center = 0, playback = -1;
    bool has_steps = false;
    std::string origin;
};

template<typename Config>
BackendContract backend_contract(const Config& c) {
    const auto& d = backend_descriptor(c.backend);
    const std::string schedule(d.schedule), boundary(d.boundary), model(d.step_model);
    BackendContract result;
    result.outputs = c.n/2+1;
    result.center = (c.n-1)/2.;
    result.origin = "input frame endpoint";
    if (schedule == "balanced") result.delay = c.hop-1;
    if (schedule == "legacy-budget") {
        size_t work = 0;
        for (size_t n = c.n/2; n > 1; n /= 2) work += c.n/4;
        const size_t quota = (work+c.hop-1)/c.hop;
        result.delay = (work+quota-1)/quota-1;
    }
    if (boundary == "inverse-job") { result.origin = "spectrum release"; result.center = 0; result.outputs = c.n; }
    if (boundary == "chain") { result.center = (c.hop-1)/2.; result.outputs = c.hop; result.playback = c.hop-1+result.delay; }
    if (boundary == "transform") { result.origin = "buffered transform"; result.center = 0; result.outputs = c.n; }
    if (boundary == "control") { result.origin = "none"; result.center = 0; result.outputs = 0; }
    result.has_steps = model == "radix2-complex" || model == "radix2-real" || model == "radix2-inverse";
    if (result.has_steps) {
        for (size_t n = model == "radix2-real" ? c.n/2 : c.n; n > 1; n /= 2)
            result.steps += model == "radix2-real" ? c.n/4 : c.n/2;
        if (model == "radix2-inverse") result.steps += c.n;
    }
    return result;
}

/// @brief Serialize trusted registry fields and resolved counts for cross-language checks.
template<typename Config>
std::string contract_json(const Config& c) {
    const auto& d = backend_descriptor(c.backend);
    const auto r = backend_contract(c);
    std::ostringstream out;
    out.precision(17);
    out << "{\"backend\":\"" << d.id << "\",\"boundary\":\"" << d.boundary
        << "\",\"precision\":\"" << d.precision << "\",\"channels\":" << d.channels
        << ",\"layout\":\"" << d.layout << "\",\"normalization\":\"" << d.normalization
        << "\",\"origin\":\"" << r.origin << "\",\"outputs_per_channel\":" << r.outputs
        << ",\"publication_delay_samples\":" << r.delay << ",\"center_offset_samples\":" << r.center
        << ",\"playback_delay_samples\":" << r.playback << ",\"step_count\":";
    if (r.has_steps) out << r.steps; else out << "null";
    out << ",\"step_model\":\"" << d.step_model << "\",\"operation\":\"" << d.operation
        << "\",\"plan\":\"" << d.plan << '"';
    if (c.workload_schema == 3) out << ",\"workload\":" << workload_controls_json(c)
        << ",\"effective_octave\":" << octave_width(c)
        << ",\"effective_temporal_alpha\":" << temporal_alpha(c)
        << ",\"input_contract\":\"finite-float-v3; abs(input)<=1; silence/decay may flush subnormals\"";
    out << '}';
    return out.str();
}
}  // namespace Paper
#endif
