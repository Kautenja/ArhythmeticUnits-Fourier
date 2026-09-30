// Equal, independent four-channel analysis workloads.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_CHANNELS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_CHANNELS_HPP_
#include <array>
#include "external.hpp"
#include "fourier.hpp"
#include "pffft.hpp"

namespace Paper {
/// @brief Shared immutable input fixtures, prepared before resource/timing probes.
inline const std::array<std::vector<float>, 4>& channel_signals() {
    static const auto values = []() {
        std::array<std::vector<float>, 4> data;
        for (size_t channel = 0; channel < 4; ++channel) {
            data[channel].resize(65536);
            uint32_t seed = 0x12345678u + uint32_t(channel)*0x9e3779b9u;
            for (size_t s = 0; s < data[channel].size(); ++s) {
                seed = 1664525u*seed+1013904223u;
                const double angle = 2*std::acos(-1.)*s/2048;
                data[channel][s] = float(0.05*channel + 0.5*std::sin((7+3*channel)*angle)
                    +0.25*std::cos((31+7*channel)*angle)+0.05*(double(seed>>8)/16777216.-0.5));
            }
        }
        return data;
    }();
    return values;
}

inline std::array<std::vector<float>, 4> channel_signals(const Config& c) {
    std::array<std::vector<float>, 4> values;
    if (c.workload_schema == 3)
        for (size_t j = 0; j < 4; ++j) values[j] = signal(c, c.fixture == "independent" ? j : 0);
    return values;
}

/// @brief Four scalar analyzers receive exactly the same channel bytes as SIMD.
template<typename Adapter>
struct ScalarChannels {
    std::array<std::unique_ptr<Adapter>, 4> channels;
    Config config;
    std::array<std::vector<float>, 4> inputs;
    size_t limit;
    size_t cursor = 0;
    explicit ScalarChannels(const Config& c) : config(c), inputs(channel_signals(c)), limit(input_limit(c)) {
        for (auto& channel : channels) channel.reset(new Adapter(c));
    }
    void process(float) {
        const auto& input = config.workload_schema == 3 ? inputs : channel_signals();
        for (size_t j = 0; j < 4; ++j) channels[j]->process(input_sample(input[j], cursor, limit));
        ++cursor;
    }
    size_t delay() const { return channels[0]->delay(); }
    bool published() const { return channels[0]->published(); }
    size_t bins() const { return channels[0]->output.size(); }
    float value(size_t j, size_t k) const { return channels[j]->output[k]; }
    void check() const { for (auto& c : channels) c->check(); }
    void barrier() const { for (auto& c : channels) c->barrier(); }
    std::string info_json() const {
        std::ostringstream out;
        out << "{\"input_contract\":\"" << (config.workload_schema == 3 ? "explicit-four-v3" : "independent-four-v1")
            << "\",\"scalar_instances\":4,\"native_instances\":[";
        for (size_t j = 0; j < 4; ++j) {
            if (j) out << ',';
            out << PaperResources::provider_info(*channels[j], 0);
        }
        out << "]}"; return out.str();
    }
};

struct SimdChannels : Core<rack::simd::float_4> {
    Config config;
    std::array<std::vector<float>, 4> inputs;
    size_t limit;
    size_t cursor = 0;
    explicit SimdChannels(const Config& c) : Core<rack::simd::float_4>(c), config(c), inputs(channel_signals(c)), limit(input_limit(c)) {}
    void process(float) {
        const auto& input = config.workload_schema == 3 ? inputs : channel_signals();
        const size_t i = cursor++;
        process_value(rack::simd::float_4(input_sample(input[0], i, limit), input_sample(input[1], i, limit),
            input_sample(input[2], i, limit), input_sample(input[3], i, limit)));
    }
    size_t bins() const { return output.size(); }
    float value(size_t j, size_t k) const { return output[k][j]; }
    std::string info_json() const {
        return std::string("{\"input_contract\":\"") + (config.workload_schema == 3 ? "explicit-four-v3" : "independent-four-v1")
            + "\",\"scalar_instances\":0,\"native_instances\":[]}";
    }
};

/// @brief Check every channel/bin against independently recomputed spectra/EMA.
struct ChannelAudit {
    SynthesisAccuracy& accuracy;
    std::vector<std::string>& instances;
    Config config;
    std::array<std::vector<float>, 4> inputs;
    std::map<const void*, std::array<std::unique_ptr<AnalysisReference<float>>, 4>> references;
    ChannelAudit(SynthesisAccuracy& a, std::vector<std::string>& p, const Config& c)
        : accuracy(a), instances(p), config(c), inputs(channel_signals(c)) {}
    ChannelAudit(ChannelAudit&&) = default;
    template<typename Adapter> void timed_instance(const Adapter& a) { instances.push_back(a.info_json()); }
    template<typename Adapter> void operator()(const Adapter& a, const std::vector<float>&, size_t sample) {
        if (!a.published()) return;
        ++accuracy.publications;
        auto& bank = references[&a];
        for (size_t j = 0; j < 4; ++j) {
            if (!bank[j]) bank[j].reset(new AnalysisReference<float>(config));
            bank[j]->advance(config.workload_schema == 3 ? inputs[j] : channel_signals()[j], sample-a.delay());
            require(a.bins() == bank[j]->expected.size(), "Missing four-channel bins");
            accuracy.analysis.compare(bank[j]->expected, [&](size_t k) { return a.value(j, k); }, sample-a.delay(), j);
            accuracy.maximum_error = accuracy.analysis.maximum_error;
            accuracy.maximum_reference = accuracy.analysis.maximum_reference;
            accuracy.checked += a.bins();
        }
    }
};

template<typename Adapter>
void channel_stream(const Config& c, bool provider_info = false) {
    channel_signals();
    if (provider_info) { Adapter adapter(c); std::cout << adapter.info_json() << '\n'; return; }
    SynthesisAccuracy accuracy;
    std::vector<std::string> instances;
    stream<Adapter>(c, ChannelAudit(accuracy, instances, c));
    if (!c.resources) {
        require(accuracy.checked && accuracy.publications, "Missing four-channel numerical audit");
        accuracy.print("independent-four-v1; per-channel DFT/binary64 FFT, direct bands and EMA", instances);
    }
}

inline void verify_channels() {
    channel_signals();
    const auto dummy = signal();
    for (const std::string state : {"steady", "live"}) {
        Config c{}; c.n = 128; c.hop = 37; c.rate = 48000; c.state = state; c.smooth = true;
        SimdChannels simd(c);
        ScalarChannels<Core<float>> scalar(c);
        SynthesisAccuracy accuracy; std::vector<std::string> instances;
        ChannelAudit audit(accuracy, instances, c);
        for (size_t sample = 0; sample < 1024; ++sample) {
            simd.process(0); scalar.process(0);
            audit(simd, dummy, sample); audit(scalar, dummy, sample);
            if (simd.published()) {
                require(scalar.published(), "SIMD/scalar channel cadence differs");
                for (size_t j = 0; j < 4; ++j) for (size_t k = 0; k < simd.bins(); ++k)
                    require(std::abs(simd.value(j,k)-scalar.value(j,k)) < 1e-4, "SIMD/scalar channel output differs");
            }
        }
    }
}
}  // namespace Paper
#endif
