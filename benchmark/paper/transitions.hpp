// Benchmark-only interactive control replay and timed analyzer adapters.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_TRANSITIONS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_TRANSITIONS_HPP_
#include <fstream>
#include <limits>
#include <memory>
#include "protocol.hpp"
#include "analysis_accuracy.hpp"
#include "../../src/dsp/spectrum_analysis.hpp"

namespace Paper {
namespace Transition {
using Settings = Fourier::SpectrumSettings;
using Window = Fourier::Window::Function;

/// @brief One full desired configuration, requested before the indicated input.
struct Event { size_t sample, generation; Settings settings; const char* reason; };
struct Suite {
    Settings initial;
    size_t maximum_length, maximum_hop, horizon;
    std::vector<Event> events;
};

/// @brief Frozen interactive sequence; no response is expected to the final request.
/// @details Request coordinates are absolute input samples from zero-padded startup.
/// The final request lands one sample before the declared observation horizon.
inline Suite suite(const Config& c) {
    require(c.transition_suite == "interactive-v1", "Unknown transition suite");
    require(c.n >= 128 && c.n <= 2048 && c.hop >= 8,
        "Transition suite requires N in [128,2048], H >= 8");
    require(c.count == 1 && c.voices == 1 && !c.load && !c.cache_mib && c.state == "startup"
        && c.alignment == "aligned" && c.callback_offset == 0 && c.pass == "callback"
        && !c.smooth && c.warm_hops == 0,
        "Transition suite requires one startup callback analyzer with no added load");
    require(c.callbacks*c.block >= 26*c.hop, "Transition observation horizon must cover at least 26H");
    Suite result;
    result.initial.length = c.n; result.initial.hop = c.hop;
    result.initial.window = Window::Hann; result.initial.sample_rate = c.rate;
    result.maximum_length = std::min(size_t(16384), 8*c.n);
    result.maximum_hop = 2*c.hop; result.horizon = c.callbacks*c.block;
    auto settings = result.initial;
    auto add = [&](size_t sample, const char* reason) {
        result.events.push_back({sample, result.events.size()+1,
            c.transition_control ? result.initial : settings, reason});
    };
    const size_t h = c.hop;
    settings.length = result.maximum_length; add(2*h, "length-increase-boundary");
    settings.length = c.n; add(4*h+1, "length-decrease-early");
    settings.hop = 2*h; add(6*h+h/2, "hop-increase-middle");
    settings.window = Window::BlackmanHarris; add(11*h-2, "window-late");
    settings.octave = 1.f/3.f; settings.alpha = 0.8f; add(13*h-1, "smoothing-before-publication");
    add(15*h, "no-op-boundary");
    settings.window = Window::Hann; add(17*h+1, "replace-pending-first");
    settings.alpha = 0.f; add(17*h+2, "replace-pending-last");
    settings.hop = h/2; add(21*h, "hop-decrease-boundary");
    settings.octave = 0.f; add(23*h+1, "smoothing-disable");
    settings.window = Window::Boxcar; add(result.horizon-1, "horizon-no-response");
    return result;
}

inline std::string settings_json(const Settings& s) {
    std::ostringstream out; out.precision(17);
    out << "{\"n\":" << s.length << ",\"hop\":" << s.hop << ",\"window\":" << int(s.window)
        << ",\"rate\":" << s.sample_rate << ",\"octave\":" << s.octave << ",\"alpha\":" << s.alpha << '}';
    return out.str();
}

/// @brief Actual production scalar API; maximum capacities prepared before timing.
template<typename T>
struct CoreEngine {
    Fourier::SpectrumAnalysis<T> analysis;
    std::vector<T> output;
    explicit CoreEngine(const Suite& s) : analysis(s.maximum_length, s.maximum_hop),
        output(s.maximum_length/2+1) { require(analysis.configure(s.initial), "Initial transition settings rejected"); }
    bool boundary() const { return analysis.is_frame_start(); }
    bool configure(const Settings& s) { return analysis.configure(s); }
    bool process(float value) {
        return analysis.process(T(value), [&](size_t k, T v) { output[k] = v; });
    }
    static bool immediate() { return false; }
    static bool float_bands() { return true; }
    std::string engine_policy() const { return "production-scalar"; }
    std::string provider_json() const { return "[]"; }
    std::string policy() const { return "production SpectrumAnalysis.configure; preallocated maximum capacities"; }
};

/// @brief Prepared native FFT with matched boundary-latched surrounding work.
/// @details This is a benchmark control, not a configurable provider API. Exact
/// size plans for both lengths and maximum ring/working storage are prepared at
/// construction. Changing length clears history logically; all other settings
/// preserve it. Every dirty window/band operation executes inside process().
/// Batch mode executes all work at the endpoint; scheduled mode balances
/// N+1+2K units, retaining the opaque native call as one indivisible unit.
template<typename T, typename Backend, bool Immediate>
struct NativeEngine {
    std::unique_ptr<Backend> plans[2];
    size_t sizes[2];
    Settings settings;
    std::vector<T> ring, frame, magnitude, prefix, output;
    std::vector<std::complex<T>> coefficients;
    std::vector<float> window;
    std::vector<size_t> low, high;
    size_t head = 0, available = 0, origin = 0, frame_available = 0, phase = 0, cursor = 0;
    size_t quota_base = 0, quota_remainder = 0, quota_error = 0, selected = 0;
    bool window_dirty = true, bands_dirty = true, clear_average = true;
    float gain = 1;
    explicit NativeEngine(const Suite& s) : settings(s.initial),
        ring(s.maximum_length+s.maximum_hop), frame(s.maximum_length),
        magnitude(s.maximum_length/2+1), prefix(s.maximum_length/2+2), output(s.maximum_length/2+1),
        coefficients(s.maximum_length/2+1), window(s.maximum_length),
        low(s.maximum_length/2+1), high(s.maximum_length/2+1) {
        sizes[0] = s.initial.length; sizes[1] = s.maximum_length;
        for (size_t i = 0; i < 2; ++i) plans[i].reset(new Backend(sizes[i], "analysis"));
        configure(s.initial);
    }
    bool boundary() const { return phase == 0; }
    bool configure(const Settings& s) {
        if (!boundary() || (s.length != sizes[0] && s.length != sizes[1])) return false;
        const bool resized = settings.length != s.length;
        if (resized) { available = 0; clear_average = true; }
        window_dirty = window_dirty || resized || settings.window != s.window;
        bands_dirty = bands_dirty || resized || settings.octave != s.octave;
        settings = s; selected = s.length == sizes[0] ? 0 : 1;
        gain = 1.f/Fourier::Window::coherent_gain(s.window);
        const size_t work = s.length+1+2*(s.length/2+1);
        quota_base = work/s.hop; quota_remainder = work%s.hop;
        return true;
    }
    void unit() {
        const size_t n = settings.length, k_count = n/2+1;
        if (cursor < n) {
            if (window_dirty) window[cursor] = gain*Fourier::Window::window<float>(settings.window,
                float(cursor), float(n), false);
            const T value = cursor+frame_available < n ? T(0) : ring[(origin+cursor)%ring.size()];
            frame[cursor] = value*window[cursor];
        } else if (cursor == n) plans[selected]->forward_real_positive(frame.data(), coefficients.data());
        else if (cursor < n+1+k_count) {
            const size_t k = cursor-n-1;
            magnitude[k] = std::abs(coefficients[k]);
            if (settings.octave) prefix[k+1] = prefix[k]+magnitude[k];
        } else {
            const size_t k = cursor-n-1-k_count;
            if (settings.octave && bands_dirty) {
                const double width = double(settings.sample_rate)/n;
                const double maximum = double(settings.sample_rate)/2;
                const float half = std::pow(2.f, settings.octave/2.f), ratio = std::pow(2.f, settings.octave);
                double a = k*width/half, b = k*width*half;
                if (b > maximum) { b = maximum; a = b/ratio; }
                low[k] = size_t(std::floor(a/width)); high[k] = std::min(n/2, size_t(std::floor(b/width)));
            }
            const T value = settings.octave ? (prefix[high[k]+1]-prefix[low[k]])/T(high[k]-low[k]+1) : magnitude[k];
            const T previous = clear_average ? T(0) : std::abs(output[k]);
            output[k] = settings.alpha == 0.f ? value : settings.alpha*previous+(1.f-settings.alpha)*value;
        }
        ++cursor;
    }
    bool process(float value) {
        ring[head] = T(value); head = (head+1)%ring.size(); available = std::min(sizes[1], available+1);
        if (phase == 0) {
            origin = (head+ring.size()-settings.length)%ring.size(); frame_available = available;
            cursor = quota_error = 0; prefix[0] = 0;
        }
        size_t quota = 0;
        if (Immediate) { if (phase == 0) quota = settings.length+1+2*(settings.length/2+1); }
        else {
            quota = quota_base; quota_error += quota_remainder;
            if (quota_error >= settings.hop) { quota_error -= settings.hop; ++quota; }
        }
        for (size_t i = 0; i < quota; ++i) unit();
        const bool complete = Immediate ? phase == 0 : phase == settings.hop-1;
        if (complete) window_dirty = bands_dirty = clear_average = false;
        phase = (phase+1)%settings.hop;
        return complete;
    }
    static bool immediate() { return Immediate; }
    static bool float_bands() { return false; }
    std::string engine_policy() const { return Immediate ? "prepared-native-immediate" : "prepared-native-balanced"; }
    std::string provider_json() const { return "["+plans[0]->info_json()+","+plans[1]->info_json()+"]"; }
    std::string policy() const { return "benchmark native control; two exact-size plans and maximum buffers prepared before timing; no transition allocation"; }
};

/// @brief Latest desired settings latch only at the real analyzer's boundary.
/// @details A pending generation may be replaced, but active work is never
/// cancelled. Public output changes only at completion. Logging is replay-only.
template<typename T, typename Engine>
struct Adapter {
    using Value = T;
    Suite plan;
    Engine engine;
    Settings settings;
    size_t sample = 0, event_index = 0, pending = 0, generation = 0, endpoint = 0, history_start = 0;
    size_t requested = 0, replaced = 0, applied = 0;
    bool complete = false;
    explicit Adapter(const Config& c) : plan(suite(c)), engine(plan), settings(plan.initial) {}
    void process(float value) {
        requested = replaced = applied = 0;
        if (event_index < plan.events.size() && plan.events[event_index].sample == sample) {
            replaced = pending; pending = plan.events[event_index++].generation; requested = pending;
        }
        if (engine.boundary()) {
            if (pending) {
                const auto& next = plan.events[pending-1].settings;
                require(engine.configure(next), "Boundary transition rejected");
                if (settings.length != next.length) history_start = sample;
                settings = next; generation = applied = pending; pending = 0;
            }
            endpoint = sample;
        }
        complete = engine.process(value); ++sample;
    }
    bool published() const { return complete; }
    size_t delay() const { return Engine::immediate() ? 0 : settings.hop-1; }
    size_t publication_endpoint(size_t) const { return endpoint; }
    double publication_center_offset() const { return (settings.length-1)/2.; }
    void barrier() const { observe(engine.output.data()); }
    std::string info_json() const {
        std::ostringstream out;
        out << "{\"transition_policy\":\"interactive-v1\",\"engine_policy\":\"" << engine.engine_policy()
            << "\",\"memory_policy\":\"" << engine.policy() << "\",\"provider_instances\":" << engine.provider_json()
            << ",\"prepared_lengths\":[" << plan.initial.length << ',' << plan.maximum_length
            << "],\"maximum_hop\":" << plan.maximum_hop << '}';
        return out.str();
    }
    void check() const {
        for (size_t k = 0; k <= settings.length/2; ++k)
            require(std::isfinite(engine.output[k]), "Non-finite transition output");
    }
};

/// @brief Independent arithmetic plus independently predicted lifecycle on replay.
/// @details The reference never uses emitted bins to update its EMA. Direct DFT
/// covers small frames; the independent recursive binary64 FFT covers large ones.
template<typename T>
struct Oracle {
    std::vector<T> average;
    size_t length = 0;
    std::vector<T> spectrum(const Settings& s, const std::vector<float>& input,
            size_t endpoint, size_t history_start, bool float_bands) {
        if (length != s.length) { average.assign(s.length/2+1, T(0)); length = s.length; }
        std::vector<std::complex<double>> frame(s.length);
        const float gain = 1.f/Fourier::Window::coherent_gain(s.window);
        for (size_t i = 0; i < s.length; ++i) {
            const int64_t source = int64_t(endpoint)+1-int64_t(s.length)+int64_t(i);
            const float window = gain*Fourier::Window::window<float>(s.window, float(i), float(s.length), false);
            if (source >= int64_t(history_start)) frame[i] = T(input[size_t(source)%input.size()])*window;
        }
        std::vector<double> magnitude(average.size());
        if (s.length <= 256) {
            const std::vector<Reference::Complex> precise(frame.begin(), frame.end());
            for (size_t k = 0; k < magnitude.size(); ++k) magnitude[k] = std::abs(Reference::coefficient(precise, k, false));
        } else {
            const auto bins = Reference::independent_fft(frame);
            for (size_t k = 0; k < magnitude.size(); ++k) magnitude[k] = std::abs(bins[k]);
        }
        for (size_t k = 0; k < average.size(); ++k) {
            double value = magnitude[k];
            if (s.octave) {
                const float half = std::pow(2.f, s.octave/2.f), ratio = std::pow(2.f, s.octave);
                size_t low, high;
                if (float_bands) {
                    const float width = s.sample_rate/s.length, maximum = s.sample_rate/2.f;
                    float a = k*width/half, b = k*width*half;
                    if (b > maximum) { b = maximum; a = b/ratio; }
                    low = size_t(std::floor(a/width)); high = std::min(s.length/2, size_t(std::floor(b/width)));
                } else {
                    const double width = double(s.sample_rate)/s.length, maximum = double(s.sample_rate)/2;
                    double a = k*width/half, b = k*width*half;
                    if (b > maximum) { b = maximum; a = b/ratio; }
                    low = size_t(std::floor(a/width)); high = std::min(s.length/2, size_t(std::floor(b/width)));
                }
                value = 0; for (size_t j = low; j <= high; ++j) value += magnitude[j];
                value /= high-low+1;
            }
            average[k] = s.alpha == 0.f ? T(value) : T(s.alpha*std::abs(average[k])+(1.f-s.alpha)*value);
        }
        return average;
    }
};

struct RequestRecord { int64_t application = -1, first = -1, replaced = -1; };
struct Publication {
    size_t generation, endpoint, sample, bins, history_start;
    std::string accuracy;
    double maximum_error, maximum_reference;
};

/// @brief Separate replay collector; no lifecycle trace work occurs while timing.
template<typename T, typename Engine>
struct Audit {
    Suite plan;
    std::vector<RequestRecord> requests;
    std::vector<Publication> publications;
    Oracle<T> oracle;
    size_t expected_sample = 0, expected_endpoint = 0, expected_generation = 0, expected_history = 0;
    size_t pending = 0, event = 0;
    Settings settings;
    std::string memory_policy, provider_instances, resource_contract;
    template<typename A> void timed_instance(const A& a) {
        memory_policy = a.engine.policy(); provider_instances = a.engine.provider_json(); resource_contract = a.info_json();
    }
    explicit Audit(const Config& c) : plan(suite(c)), requests(plan.events.size()), settings(plan.initial) {}
    void operator()(const Adapter<T, Engine>& a, const std::vector<float>& input, size_t sample) {
        require(sample == expected_sample++, "Transition replay skipped input");
        size_t request = 0, replacement = 0, applied = 0;
        if (event < plan.events.size() && plan.events[event].sample == sample) {
            replacement = pending; pending = plan.events[event++].generation; request = pending;
            if (replacement) requests[replacement-1].replaced = int64_t(pending);
        }
        if (sample == expected_endpoint) {
            if (pending) {
                const auto& next = plan.events[pending-1].settings;
                if (settings.length != next.length) expected_history = sample;
                settings = next; expected_generation = applied = pending; pending = 0;
                requests[applied-1].application = int64_t(sample);
            }
        }
        const size_t publication = expected_endpoint+(Engine::immediate() ? 0 : settings.hop-1);
        require(a.requested == request && a.replaced == replacement && a.applied == applied,
            "Transition request/application identity differs");
        require(a.generation == expected_generation && a.endpoint == expected_endpoint
            && a.history_start == expected_history, "Transition frame identity differs");
        require(a.published() == (sample == publication), "Missing or extra transition publication");
        if (a.published()) {
            const auto expected = oracle.spectrum(settings, input, expected_endpoint, expected_history, Engine::float_bands());
            AnalysisAccuracy accuracy;
            accuracy.compare(expected, [&](size_t k) { return a.engine.output[k]; }, expected_endpoint);
            publications.push_back({expected_generation, expected_endpoint, sample, expected.size(), expected_history, accuracy.json(), accuracy.maximum_error, accuracy.maximum_reference});
            if (expected_generation && requests[expected_generation-1].first < 0)
                requests[expected_generation-1].first = int64_t(sample);
        }
        if (sample == expected_endpoint+settings.hop-1) expected_endpoint += settings.hop;
    }
    void finish(const Config& c) const {
        require(expected_sample == plan.horizon && event == plan.events.size() && !publications.empty(),
            "Incomplete transition replay");
        const char* path = std::getenv("PAPER_TRANSITION_PATH");
        require(path && *path, "PAPER_TRANSITION_PATH must name the transition sidecar");
        std::ofstream out(path); require(bool(out), "Cannot open transition sidecar");
        out.precision(17);
        out << "{\"schema\":\"fourier-transitions-v1\",\"suite\":\"interactive-v1\",\"backend\":\"" << c.backend
            << "\",\"control\":" << (c.transition_control ? "true" : "false") << ",\"horizon\":" << plan.horizon
            << ",\"reference_precision\":\"binary64 FFT; long-double direct DFT at N<=256\""
            << ",\"reference_mantissa_bits\":" << std::numeric_limits<double>::digits
            << ",\"direct_dft_mantissa_bits\":" << std::numeric_limits<long double>::digits
            << ",\"block\":" << c.block << ",\"rate\":" << c.rate << ",\"instance\":0,\"channel\":0"
            << ",\"time_origin\":\"absolute input sample; request before input; startup is zero padded\""
            << ",\"retention\":\"length clears input and EMA; other settings retain both; old complete spectrum may remain visible\""
            << ",\"latch\":\"latest request at frame boundary; pending replacement; active frame never cancelled\""
            << ",\"memory_policy\":\"" << memory_policy << "\",\"provider_instances\":" << provider_instances << ",\"immediate\":" << (Engine::immediate() ? "true" : "false")
            << ",\"resource_contract\":" << resource_contract << ",\"initial\":" << settings_json(plan.initial) << ",\"events\":[";
        for (size_t i = 0; i < plan.events.size(); ++i) {
            if (i) out << ',';
            const auto& e = plan.events[i]; const auto& r = requests[i];
            out << "{\"generation\":" << e.generation << ",\"request_sample\":" << e.sample
                << ",\"reason\":\"" << e.reason << "\",\"settings\":" << settings_json(e.settings)
                << ",\"application_sample\":" << r.application << ",\"first_publication_sample\":" << r.first
                << ",\"replaced_by\":" << r.replaced << ",\"outcome\":\""
                << (r.replaced >= 0 ? "replaced" : r.first >= 0 ? "published" : r.application >= 0 ? "applied-no-publication" : "pending-no-response") << "\"}";
        }
        out << "],\"publications\":[";
        for (size_t i = 0; i < publications.size(); ++i) {
            if (i) out << ',';
            const auto& p = publications[i];
            out << "{\"instance\":0,\"channel\":0,\"generation\":" << p.generation << ",\"endpoint\":" << p.endpoint
                << ",\"publication_sample\":" << p.sample << ",\"history_start\":" << p.history_start
                << ",\"bins\":" << p.bins << ",\"max_abs_error\":" << p.maximum_error
                << ",\"max_reference\":" << p.maximum_reference << ",\"accuracy\":" << p.accuracy << '}';
        }
        out << "]}\n"; out.close(); require(bool(out), "Cannot write transition sidecar");
    }
};

/// @brief Reference wrapper keeps replay state available after stream's value copy.
template<typename AuditType>
struct AuditReference {
    AuditType* target;
    template<typename A> void timed_instance(const A& a) { target->timed_instance(a); }
    template<typename A> void operator()(const A& a, const std::vector<float>& input, size_t sample) {
        (*target)(a, input, sample);
    }
};
template<typename T, typename Engine>
void run(const Config& c) {
    Audit<T, Engine> audit(c);
    stream<Adapter<T, Engine>>(c, AuditReference<Audit<T, Engine>>{&audit});
    if (c.resources) return;
    audit.finish(c);
}
/// @brief Small native-capable preflight; complete numerical/lifecycle replay only.
template<typename T, typename Engine>
void verify() {
    Config c{};
    c.backend = "core-float"; c.pass = "callback"; c.state = "startup"; c.alignment = "aligned";
    c.n = 128; c.hop = 16; c.block = 16; c.callbacks = 26;
    c.count = c.voices = 1; c.rate = 48000; c.transition_suite = "interactive-v1";
    const auto input = signal();
    Adapter<T, Engine> adapter(c);
    Audit<T, Engine> audit(c);
    for (size_t sample = 0; sample < c.callbacks*c.block; ++sample) {
        adapter.process(input[sample]);
        audit(adapter, input, sample);
    }
    require(audit.requests[6].replaced == 8 && audit.requests[6].application == -1
        && audit.requests.back().application == -1 && audit.requests.back().first == -1,
        "Transition preflight omitted replacement or no-response outcome");
}
}  // namespace Transition
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_TRANSITIONS_HPP_
