// Shared contiguous-stage native batch/hybrid analysis and independent audits.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_ANALYSIS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_ANALYSIS_HPP_

#include <array>
#include <map>
#include "native_kernels.hpp"
#include "channels.hpp"

namespace Paper {
/// @brief Compiles to the ordinary work call: no clocks, trace stores or branch.
struct NoStageObserver {
    template<typename Work> void operator()(size_t, size_t, size_t, Work work) { work(); }
};
struct NoNativeSink { template<typename T> void operator()(size_t, size_t, T) {} };

/// @brief One retained ring and one provider plan, shared by batch/hybrid modes.
/// @details W=C*N+1+2*C*K units. Native transform/reorder is one indivisible
/// call. Contiguous stages split only at channel/ring boundaries. Both modes
/// perform the same arithmetic and stores; segment sizes can affect provider
/// vector kernels' rounding. Four-channel plans publish simultaneous spectra.
template<typename Kernel>
struct NativeAnalysis {
    using T = typename Kernel::Scalar;
    static constexpr size_t channels = Kernel::channels;
    Config config;
    Kernel kernel;
    FrameSchedule schedule;
    const size_t n, bins_count, retained, limit;
    std::vector<T> ring, window, magnitude, prefix, output;
    std::vector<size_t> low, high;
    std::array<std::vector<float>, channels> fixtures;
    size_t head = 0, origin = 0, cursor = 0, sample = 0, frames = 0;
    bool dirty = true, bands;
    float alpha, gain, half_band, band_ratio;
    Fourier::Window::Function function;

    explicit NativeAnalysis(const Config& c) : config(c), kernel(c.n),
        schedule(c, channels*c.n+1+2*channels*(c.n/2+1)), n(c.n), bins_count(n/2+1),
        retained(n+c.hop), limit(input_limit(c)), ring(channels*retained), window(n),
        magnitude(channels*bins_count), prefix(channels*(bins_count+1)), output(channels*bins_count),
        low(bins_count), high(bins_count), bands(octave_width(c) > 0), alpha(temporal_alpha(c)),
        gain(1.f/Fourier::Window::coherent_gain(window_function(c))),
        half_band(std::pow(2.f, (c.workload_schema == 3 ? octave_width(c) : 1.f/3.f)/2.f)),
        band_ratio(std::pow(2.f, c.workload_schema == 3 ? octave_width(c) : 1.f/3.f)), function(window_function(c)) {
        if (channels > 1) for (size_t channel = 0; channel < channels; ++channel)
            fixtures[channel] = c.workload_schema == 3 ? signal(c, c.fixture == "independent" ? channel : 0)
                                                       : channel_signals()[channel];
        // Avoid unequal backend-name capacity in the matched storage pair.
        std::string().swap(config.backend);
    }
    /// @brief Frame-boundary controls for benchmark-only complete module sinks.
    void set_controls(Fourier::Window::Function next_window, float octave, float next_alpha, size_t rate) {
        if (function != next_window || float(config.octave) != octave || config.rate != rate) {
            dirty = true; function = next_window;
            gain = 1.f/Fourier::Window::coherent_gain(function);
            half_band = std::pow(2.f, octave/2.f); band_ratio = std::pow(2.f, octave);
            config.octave = octave; config.rate = rate;
        }
        bands = octave > 0; alpha = next_alpha;
    }
    void prepare(size_t first, size_t count) {
        while (count) {
            const size_t channel = first/n, at = first%n, source = (origin+at)%retained;
            const size_t length = std::min(count, std::min(n-at, retained-source));
            if (dirty && channel == 0) for (size_t i = at; i < at+length; ++i)
                window[i] = T(gain*Fourier::Window::window<float>(function, float(i), float(n), false));
            kernel.prepare(channel, at, ring.data()+channel*retained+source, window.data()+at, length);
            first += length; count -= length;
        }
    }
    void magnitudes(size_t first, size_t count) {
        while (count) {
            const size_t channel = first/bins_count, at = first%bins_count;
            const size_t length = std::min(count, bins_count-at);
            kernel.magnitudes(channel, at, length, magnitude.data()+first);
            if (bands) for (size_t k = at; k < at+length; ++k)
                prefix[channel*(bins_count+1)+k+1] = prefix[channel*(bins_count+1)+k]+magnitude[channel*bins_count+k];
            first += length; count -= length;
        }
    }
    template<typename Sink>
    void outputs(size_t first, size_t count, Sink& sink) {
        while (count) {
            const size_t channel = first/bins_count, at = first%bins_count;
            const size_t length = std::min(count, bins_count-at);
            const T* sums = prefix.data()+channel*(bins_count+1);
            if (bands && dirty && channel == 0) {
                for (size_t k = at; k < at+length; ++k) {
                    const double width = double(config.rate)/n, maximum = double(config.rate)/2;
                    double a = k*width/half_band, b = k*width*half_band;
                    if (b > maximum) { b = maximum; a = b/band_ratio; }
                    low[k] = size_t(std::floor(a/width)); high[k] = std::min(n/2, size_t(std::floor(b/width)));
                }
            }
            for (size_t i = 0; i < length; ++i) {
                const size_t k = at+i;
                const T value = bands ? (sums[high[k]+1]-sums[low[k]])/T(high[k]-low[k]+1) : magnitude[first+i];
                output[first+i] = alpha == 0.f ? value : alpha*output[first+i]+(1.f-alpha)*value;
                sink(channel, k, output[first+i]);
            }
            first += length; count -= length;
        }
    }
    template<typename Input, typename Observer, typename Sink>
    void process_inputs(Input input, Observer& observer, Sink& sink, bool capture = true) {
        if (capture) {
            for (size_t channel = 0; channel < channels; ++channel) ring[channel*retained+head] = T(input(channel));
            head = (head+1 == retained ? 0 : head+1);
        }
        ++sample;
        if (schedule.phase == 0) {
            origin = (head+retained-n)%retained; cursor = 0;
            for (size_t channel = 0; channel < channels; ++channel) prefix[channel*(bins_count+1)] = 0;
            if (config.state == "live") {
                function = frames++%2 ? Fourier::Window::Function::Hann : Fourier::Window::Function::BlackmanHarris;
                bands = frames%2 == 0; dirty = true;
                gain = 1.f/Fourier::Window::coherent_gain(function);
            }
        }
        size_t quota = schedule.quota();
        const size_t ends[] = {channels*n, channels*n+1, channels*n+1+channels*bins_count, schedule.work};
        while (quota) {
            size_t stage = 0; while (cursor >= ends[stage]) ++stage;
            const size_t length = std::min(quota, ends[stage]-cursor);
            const size_t first = cursor-(stage ? ends[stage-1] : 0);
            observer(stage, first, length, [&]() {
                switch (stage) {
                case 0: prepare(first, length); break;
                case 1: kernel.transform(); break;
                case 2: magnitudes(first, length); break;
                case 3: outputs(first, length, sink); break;
                }
            });
            cursor += length; quota -= length;
        }
        schedule.complete = cursor == schedule.work && schedule.phase == delay();
        if (schedule.complete) dirty = false;
        schedule.advance();
    }
    template<typename Observer>
    void process_observed(float value, Observer& observer) {
        NoNativeSink sink;
        process_inputs([&](size_t channel) { return channels == 1 ? value : input_sample(fixtures[channel], sample, limit); }, observer, sink);
    }
    void process(float value) { NoStageObserver observer; process_observed(value, observer); }
    size_t delay() const { return schedule.delay(); }
    bool published() const { return schedule.complete; }
    void barrier() const { observe(output.data()); }
    void check() const { for (T value : output) require(std::isfinite(value), "Invalid native analysis output"); }
    std::string info_json() const {
        auto native = kernel.info_json(); native.pop_back();
        std::ostringstream out;
        out << native << ",\"analysis_pipeline\":\"native-segments-v1\",\"channels\":" << channels
            << ",\"channel_endpoints\":\"simultaneous per instance; jH\",\"task_units\":" << schedule.work
            << ",\"retained_input_samples_per_channel\":" << retained
            << ",\"publication_delay_samples\":" << delay() << ",\"initial_cache\":\"dirty; rebuild in first frame\"}";
        return out.str();
    }
};

/// @brief Every sample's cadence and every channel/bin, outside timing only.
template<typename Kernel>
struct NativeAudit {
    using Adapter = NativeAnalysis<Kernel>;
    using T = typename Kernel::Scalar;
    Config config;
    SynthesisAccuracy& accuracy;
    std::vector<std::string>& providers;
    struct Instance {
        size_t first_sample = 0, samples = 0, publications = 0, first_endpoint = 0, last_endpoint = 0;
        std::array<std::unique_ptr<AnalysisReference<T>>, Kernel::channels> references;
    };
    std::map<const void*, size_t> ids;
    std::vector<Instance> instances;
    NativeAudit(const Config& c, SynthesisAccuracy& a, std::vector<std::string>& p) : config(c), accuracy(a), providers(p) {}
    NativeAudit(NativeAudit&&) = default;
    void timed_instance(const Adapter& adapter) { providers.push_back(adapter.info_json()); }
    void operator()(const Adapter& adapter, const std::vector<float>& input, size_t sample) {
        if (!ids.count(&adapter)) {
            ids[&adapter] = instances.size(); instances.emplace_back();
            auto& instance = instances.back(); instance.first_sample = sample;
            for (auto& reference : instance.references) reference.reset(new AnalysisReference<T>(config));
        }
        auto& instance = instances[ids.at(&adapter)];
        require(sample == instance.first_sample+instance.samples++, "Missing native replay sample");
        const bool expected = sample%config.hop == adapter.delay();
        require(adapter.published() == expected, "Native publication cadence mismatch");
        if (!expected) return;
        const size_t endpoint = sample-adapter.delay();
        if (!instance.publications++) instance.first_endpoint = endpoint;
        instance.last_endpoint = endpoint;
        require(adapter.output.size() == Kernel::channels*(config.n/2+1), "Missing native bins");
        for (size_t channel = 0; channel < Kernel::channels; ++channel) {
            auto& reference = *instance.references[channel];
            reference.advance(Kernel::channels == 1 ? input : adapter.fixtures[channel], endpoint);
            accuracy.analysis.compare(reference.expected, [&](size_t k) { return adapter.output[channel*(config.n/2+1)+k]; }, endpoint, channel);
            accuracy.checked += reference.expected.size();
        }
        ++accuracy.publications;
        accuracy.maximum_error = accuracy.analysis.maximum_error;
        accuracy.maximum_reference = accuracy.analysis.maximum_reference;
    }
    std::string coverage() const {
        std::ostringstream out;
        out << "{\"policy\":\"native-all-channels-v1\",\"instances\":[";
        for (size_t i = 0; i < instances.size(); ++i) {
            if (i) out << ',';
            const auto& item = instances[i];
            out << "{\"instance\":" << i << ",\"first_sample\":" << item.first_sample << ",\"samples\":" << item.samples
                << ",\"publications\":" << item.publications << ",\"channels\":[";
            for (size_t c = 0; c < Kernel::channels; ++c) {
                if (c) out << ',';
                out << "{\"channel\":" << c << ",\"first_endpoint\":" << item.first_endpoint
                    << ",\"last_endpoint\":" << item.last_endpoint << ",\"spectra\":" << item.publications
                    << ",\"bins\":" << item.publications*(config.n/2+1) << '}';
            }
            out << "]}";
        }
        out << "]}"; return out.str();
    }
};

template<typename Kernel> void diagnose(const Config& c, const std::string& mode);

/// @brief Small untimed all-channel preflight before any user campaign.
template<typename Kernel>
void verify_native_preflight(const std::string& backend) {
    Config c{}; c.backend = backend; c.n = 128; c.hop = 37; c.rate = 48000;
    c.state = "live"; c.smooth = true;
    NativeAnalysis<Kernel> adapter(c);
    SynthesisAccuracy accuracy; std::vector<std::string> providers;
    NativeAudit<Kernel> audit(c, accuracy, providers); const auto input = signal(c);
    for (size_t s = 0; s < 8*c.hop; ++s) { adapter.process(input[s]); audit(adapter, input, s); }
    require(accuracy.analysis.vectors == 8*Kernel::channels, "Incomplete native preflight");
}

template<typename Kernel>
void native_dispatch(const Config& c, bool provider_info, const std::string& diagnostic = "") {
    if (!diagnostic.empty()) { diagnose<Kernel>(c, diagnostic); return; }
    if (provider_info) { NativeAnalysis<Kernel> adapter(c); std::cout << adapter.info_json() << '\n'; return; }
    SynthesisAccuracy accuracy; std::vector<std::string> providers;
    // stream owns/moves its audit. Retain coverage through an untimed shared sink.
    auto audit = std::make_shared<NativeAudit<Kernel>>(c, accuracy, providers);
    struct Forward {
        std::shared_ptr<NativeAudit<Kernel>> audit;
        void timed_instance(const NativeAnalysis<Kernel>& a) { audit->timed_instance(a); }
        void operator()(const NativeAnalysis<Kernel>& a, const std::vector<float>& input, size_t sample) { (*audit)(a, input, sample); }
    };
    stream<NativeAnalysis<Kernel>>(c, Forward{audit});
    if (!c.resources) accuracy.print("independent all-channel DFT/binary64 FFT, direct bands and EMA", providers,
        ",\"native_audit\":"+audit->coverage());
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_ANALYSIS_HPP_
