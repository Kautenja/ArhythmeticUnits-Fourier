// Opt-in stage diagnostics, separate from primary integrated measurements.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_DIAGNOSTICS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_DIAGNOSTICS_HPP_
#include "native_analysis.hpp"
namespace Paper {
struct StageEvent { size_t sample, stage, first, count; double ns; };
/// @brief Trace has no clock; overhead times empty brackets then does real work.
struct StageObserver {
    std::string mode;
    size_t sample = 0;
    std::vector<StageEvent> events;
    explicit StageObserver(const std::string& value, size_t samples) : mode(value) { events.reserve(4*samples); }
    template<typename Work> void operator()(size_t stage, size_t first, size_t count, Work work) {
        double ns = -1;
        if (mode == "trace") work();
        else {
            const auto start = Clock::now();
            if (mode == "stages") work();
            else asm volatile("" : : : "memory");
            const auto end = Clock::now(); ns = elapsed(start, end);
            if (mode == "overhead") work();
        }
        events.push_back({sample, stage, first, count, ns});
    }
};

template<typename Kernel>
void diagnose(const Config& c, const std::string& mode) {
    require(mode == "trace" || mode == "stages" || mode == "overhead", "Unknown diagnostic mode");
    require(c.count == 1 && !c.load && !c.cache_mib && !c.callback_offset && c.alignment == "aligned"
        && c.pass == "callback" && c.execution_regime == "continuous", "Diagnostics require an isolated continuous callback case");
    const size_t total = c.callbacks*c.block;
    require(total <= 65536, "Diagnostics are bounded to 65536 samples; use primary passes for long runs");
    const auto execution_path = Execution::environment("PAPER_DIAGNOSTIC_EXECUTION_PATH");
    require(mode == "trace" || !execution_path.empty(), "Timed diagnostics require PAPER_DIAGNOSTIC_EXECUTION_PATH");
    Execution::OutputPath execution_output(execution_path);
    const auto input = signal(c);
    NativeAnalysis<Kernel> adapter(c);
    StageObserver observer(mode, total);
    const size_t warm = c.state == "startup" ? 0 : ((c.n+c.hop-1)/c.hop+c.warm_hops)*c.hop;
    std::unique_ptr<Execution::Session> session;
    if (mode != "trace") { session.reset(new Execution::Session("complete")); session->settle(); }
    for (size_t s = 0; s < warm; ++s) adapter.process(input_sample(c, input, s));
    for (size_t s = 0; s < total; ++s) {
        observer.sample = s;
        adapter.process_observed(input_sample(c, input, warm+s), observer);
    }
    adapter.check();
    if (session) { session->measured(); session->finish(); }
    // Independent replay does not perturb the stage diagnostic's measured run.
    NativeAnalysis<Kernel> replay(c);
    SynthesisAccuracy accuracy; std::vector<std::string> providers;
    NativeAudit<Kernel> audit(c, accuracy, providers);
    for (size_t s = 0; s < warm; ++s) replay.process(input_sample(c, input, s));
    for (size_t s = warm; s < warm+total; ++s) { replay.process(input_sample(c, input, s)); audit(replay, input, s); }
    std::cout.precision(17);
    std::cout << "{\"schema\":1,\"kind\":\"native-stage-diagnostic-v1\",\"mode\":\"" << mode
        << "\",\"fixture_clock\":"
#ifdef PAPER_FIXTURE_CLOCK
        << "true"
#else
        << "false"
#endif
        << ",\"config\":" << workload_config_json(c)
        << ",\"contract\":" << contract_json(c) << ",\"provider\":" << adapter.info_json()
        << ",\"warm_samples\":" << warm << ",\"samples\":" << total
        << ",\"limitations\":\"Perturbed diagnostic, not integrated callback evidence; do not sum stage times. Overhead includes empty timer brackets only; trace/event storage and instrumentation alter execution.\""
        << ",\"max_abs_error\":" << accuracy.maximum_error << ",\"max_reference\":" << accuracy.maximum_reference
        << ",\"checked_samples\":" << accuracy.checked
        << ",\"accuracy\":" << accuracy.analysis.json() << ",\"coverage\":" << audit.coverage()
        << ",\"stages\":[\"window-pack\",\"native-transform-reorder\",\"magnitude-prefix\",\"smooth-store\"],\"events\":[";
    for (size_t i = 0; i < observer.events.size(); ++i) {
        const auto& event = observer.events[i]; if (i) std::cout << ',';
        std::cout << "{\"sample\":" << event.sample << ",\"stage\":" << event.stage << ",\"first\":" << event.first
            << ",\"count\":" << event.count << ",\"ns\":";
        if (mode == "trace") std::cout << "null"; else std::cout << event.ns;
        std::cout << '}';
    }
    std::cout << "]}\n";
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_DIAGNOSTICS_HPP_
