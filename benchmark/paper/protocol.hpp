// Raw measurement protocol shared by current and future paper backends.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_PROTOCOL_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_PROTOCOL_HPP_

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "../../src/dsp/dc_blocker.hpp"
#include "backend.hpp"
#include "resources.hpp"
#include "runtime.hpp"

namespace Paper {
using Clock = std::chrono::steady_clock;
inline void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
/// @brief Compiler barrier only; no volatile access per arithmetic operation.
template<typename T> void observe(const T& value) {
    asm volatile("" : : "g"(&value) : "memory");
}
inline double elapsed(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::nano>(end-start).count();
}

/// @brief One explicitly labeled workload; all sample counts are engine samples.
struct Config {
    std::string backend, pass, alignment, state;
    size_t n, hop, block, count, load, voices, callbacks, warm_hops, cache_mib;
    float rate;
    bool smooth;
    bool resources = false;
    size_t callback_offset = 0;
    std::string transition_suite;
    bool transition_control = false;
};

/// @brief Common float input bytes across precisions, lanes and backend families.
inline std::vector<float> signal() {
    std::vector<float> values(65536);
    uint32_t seed = 0x12345678u;
    for (size_t i = 0; i < values.size(); ++i) {
        seed = 1664525u * seed + 1013904223u;
        const double angle = 2*std::acos(-1.)*i/2048;
        values[i] = 0.2 + 0.5*std::sin(7*angle) + 0.25*std::cos(31*angle)
            + 0.05*(double(seed >> 8)/16777216. - 0.5);
    }
    return values;
}

/// @brief Retain raw observations; writing CSV happens after all timing.
struct Row {
    std::string kind;
    size_t index, analyzer, sample, samples;
    double ns, endpoint_age = 0, center_age = 0, visible_age = 0;
    double playback_delay = -1;
    Row(std::string k, size_t i, size_t a, size_t s, size_t length, double time) :
        kind(k), index(i), analyzer(a), sample(s), samples(length), ns(time) {}
};
/// @brief Optional single-threaded result sink, invoked only after measurement.
/// Publication CLI output remains unchanged when no development sink is set.
inline std::function<void(const std::vector<Row>&)>& result_sink() {
    static std::function<void(const std::vector<Row>&)> sink;
    return sink;
}
inline void print_csv(const std::vector<Row>& rows, std::ostream& out) {
    out.precision(17);
    out << "kind,index,analyzer,sample,samples,ns,endpoint_age_samples,center_age_samples,callback_visible_age_samples,playback_delay_samples\n";
    for (const auto& r : rows)
        out << r.kind << ',' << r.index << ',' << r.analyzer << ',' << r.sample << ','
            << r.samples << ',' << r.ns << ',' << r.endpoint_age << ',' << r.center_age
            << ',' << r.visible_age << ',' << r.playback_delay << '\n';
}
inline void print(const std::vector<Row>& rows) {
    Runtime::Scope phase(Runtime::Phase::CsvOutput);
    if (result_sink()) result_sink()(rows);
    else {
        print_csv(rows, std::cout);
        if (Runtime::active()) {
            std::cout.flush();
            require(bool(std::cout), "Cannot write paper observations");
        }
    }
}

/// @brief Same-thread fixed DSP load; never calibrate load to a target percentage.
struct Background {
    std::vector<Fourier::DCBlocker<double>> filters;
    double output = 0;
    explicit Background(size_t count) : filters(count) {}
    void process(float value) {
        double result = value;
        for (auto& filter : filters) result = filter.process(result);
        output = result;
    }
};

/// @brief Run identical input traversal for timing and an untimed publication audit.
/// @details Adapter process() includes every output store but no audit/logging.
/// published() is called only during the separate replay. Offsets advance each
/// analyzer's input and schedule together, preserving the represented window.
struct NoAudit {
    template<typename Adapter>
    void operator()(const Adapter&, const std::vector<float>&, size_t) const {}
};

/// @brief Optional provenance collection only after the complete timed pass.
template<typename Audit, typename Adapter>
auto report_timed_instance(Audit& audit, const Adapter& adapter, int)
    -> decltype(audit.timed_instance(adapter), void()) { audit.timed_instance(adapter); }
template<typename Audit, typename Adapter>
void report_timed_instance(Audit&, const Adapter&, long) {}

/// @brief Optional variable-frame timestamps, evaluated only during replay.
template<typename Adapter>
auto publication_timestamps(const Adapter& adapter, size_t sample, Row& row, int)
    -> decltype(adapter.publication_endpoint(sample), adapter.publication_center_offset(), void()) {
    const size_t endpoint = adapter.publication_endpoint(sample);
    require(endpoint <= sample, "Publication represents future input");
    row.endpoint_age = sample-endpoint;
    row.center_age = row.endpoint_age+adapter.publication_center_offset();
}
template<typename Adapter>
void publication_timestamps(const Adapter&, size_t, Row&, long) {}

template<typename Adapter, typename Audit = NoAudit>
void stream(const Config& c, Audit audit = Audit(), double center_offset = -1,
        double playback_delay = -1) {
    Runtime::set(Runtime::Phase::Setup);
    const auto contract = backend_contract(c);
    if (center_offset < 0) center_offset = contract.center;
    const auto input = signal();
    if (c.resources) {
        const size_t samples = c.transition_suite.empty() ? 2*c.n+2*c.hop : c.callbacks*c.block;
        PaperResources::inspect<Adapter>([&]() { return new Adapter(c); }, [&](Adapter& adapter) {
            for (size_t i = 0; i < samples; ++i) adapter.process(input[i%input.size()]);
            adapter.barrier();
            adapter.check();
        }, samples);
        return;
    }
    const size_t total = c.callbacks*c.block;
    std::vector<Row> rows;
    rows.reserve(c.callbacks + total/c.hop*c.count + 2048);
    std::vector<unsigned char> cache(c.cache_mib*1024*1024, 1);
    auto prepare = [&](std::vector<std::unique_ptr<Adapter>>& bank, std::vector<size_t>& cursors, bool replay) {
        for (size_t a = 0; a < c.count; ++a) {
            Runtime::set(replay ? Runtime::Phase::ReplaySetup : Runtime::Phase::TimedSetup);
            bank.emplace_back(new Adapter(c));
            const size_t offset = c.callback_offset + (c.alignment == "staggered" ? a*c.hop/c.count : 0);
            const size_t warm = c.state == "startup" ? 0 :
                ((c.n+c.hop-1)/c.hop + c.warm_hops)*c.hop;
            cursors.push_back(warm+offset);
            Runtime::set(replay ? Runtime::Phase::ReplayWarmup : Runtime::Phase::TimedWarmup);
            for (size_t s = 0; s < warm+offset; ++s)
                bank.back()->process(input[s%input.size()]);
            bank.back()->published(); // Establish the initial consumer snapshot.
        }
    };
    // Empty clock readings are retained, never subtracted from short calls.
    Runtime::set(Runtime::Phase::TimerCalibration);
    for (size_t i = 0; i < 1024; ++i) {
        const auto start = Clock::now();
        observe(input);
        const auto end = Clock::now();
        rows.emplace_back("timer", i, 0, 0, 0, elapsed(start, end));
    }
    Runtime::set(Runtime::Phase::TimedSetup);
    {
        std::vector<std::unique_ptr<Adapter>> bank;
        std::vector<size_t> cursors;
        prepare(bank, cursors, false);
        Runtime::set(Runtime::Phase::TimedSetup);
        Background background(c.load);
        Runtime::set(Runtime::Phase::TimedWarmup);
        if (c.state != "startup")
            for (size_t i = 0; i < c.warm_hops*c.hop; ++i) background.process(input[i%input.size()]);
        auto run = [&](size_t samples) {
            for (size_t s = 0; s < samples; ++s) {
                background.process(input[cursors[0]%input.size()]);
                for (size_t a = 0; a < bank.size(); ++a) {
                    bank[a]->process(input[cursors[a]%input.size()]);
                    ++cursors[a];
                }
            }
            for (const auto& adapter : bank) adapter->barrier();
            observe(background);
        };
        const size_t chunks = c.pass == "throughput" ? 1 : c.callbacks;
        const size_t samples = c.pass == "throughput" ? total : c.block;
        Runtime::set(Runtime::Phase::Measurement);
        for (size_t i = 0; i < chunks; ++i) {
            // Cache pressure is outside timing, not charged to simulated deadlines.
            for (size_t k = 0; k < cache.size(); k += 64) ++cache[k];
            observe(cache);
            const auto start = Clock::now();
            run(samples);
            const auto end = Clock::now();
            rows.emplace_back(c.pass, i, 0, i*samples, samples, elapsed(start, end));
        }
        Runtime::set(Runtime::Phase::PostMeasurementChecks);
        for (const auto& adapter : bank) {
            adapter->check();
            report_timed_instance(audit, *adapter, 0);
        }
        Runtime::set(Runtime::Phase::TimedTeardown);
    }
    Runtime::set(Runtime::Phase::Other);
    if (std::string(backend_descriptor(c.backend).boundary) == "control") { print(rows); return; }
    // Replay separately so per-sample timing contains no instrumentation for
    // publication statistics or consumer mailbox traffic.
    Runtime::set(Runtime::Phase::ReplaySetup);
    {
        std::vector<std::unique_ptr<Adapter>> bank;
        std::vector<size_t> cursors;
        prepare(bank, cursors, true);
        Runtime::set(Runtime::Phase::ReplaySetup);
        std::vector<size_t> publications(c.count, 0), previous(c.count, 0);
        Runtime::set(Runtime::Phase::CorrectnessReplay);
        for (size_t s = 0; s < total; ++s) {
            for (size_t a = 0; a < bank.size(); ++a) {
                bank[a]->process(input[cursors[a]++%input.size()]);
                audit(*bank[a], input, cursors[a]-1);
                if (!bank[a]->published()) continue;
                const size_t offset = c.callback_offset + (c.alignment == "staggered" ? a*c.hop/c.count : 0);
                const size_t delay = bank[a]->delay();
                if (c.transition_suite.empty()) {
                    require(delay == contract.delay, "Adapter delay differs from registered contract");
                    require((s+offset)%c.hop == delay, "Publication phase differs from contract");
                    if (publications[a]) require(s-previous[a] == c.hop, "Publication cadence changed");
                }
                previous[a] = s;
                Row row("publication", publications[a]++, a, s, 0, 0);
                row.endpoint_age = delay;
                row.center_age = row.endpoint_age + center_offset;
                publication_timestamps(*bank[a], cursors[a]-1, row, 0);
                row.visible_age = row.endpoint_age + (c.block-1-s%c.block);
                row.playback_delay = playback_delay;
                rows.push_back(row);
            }
        }
        for (size_t a = 0; a < bank.size(); ++a) {
            const size_t offset = c.callback_offset + (c.alignment == "staggered" ? a*c.hop/c.count : 0);
            if (c.transition_suite.empty()) {
                const size_t bias = c.hop-1-bank[a]->delay();
                require(publications[a] == (total+offset+bias)/c.hop-(offset+bias)/c.hop, "Missing publications");
            }
            bank[a]->check();
        }
        Runtime::set(Runtime::Phase::ReplayTeardown);
    }
    Runtime::set(Runtime::Phase::Other);
    print(rows);
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_PROTOCOL_HPP_
