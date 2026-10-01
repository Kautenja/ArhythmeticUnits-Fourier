// Pinned Rack-engine integration, prepared graphs and single-consumer observations.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_ENGINE_HOST_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_ENGINE_HOST_HPP_
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#ifndef _WIN32
#include <sys/resource.h>
#endif
#include "native_modules.hpp"
#include "development.hpp"
#include "module_lifecycle_reference.hpp"

namespace Paper {
namespace EngineHost {
using Development::own;
using Development::set;
using Development::dump;
using Development::string;
using Development::config_from_json;
struct Event { size_t block; std::string kind; };
struct Plan {
    Config c = Development::base();
    std::string native;
    size_t threads = 1, analyzers = 1, background = 0, consumer_hz = 0;
    size_t stall_every = 0, stall_polls = 2, blocks = 128, warm_hops = 2;
    std::vector<Event> events;
};
inline Plan parse(json_t* json) {
    require(json_is_object(json) && json_integer_value(json_object_get(json, "schema")) == 1, "Unknown engine profile schema");
    Plan p; p.c = config_from_json(json_object_get(json, "workload"));
    const char* key; json_t* value;
    json_object_foreach(json, key, value) {
        const std::string name(key);
        if (name == "schema" || name == "workload") continue;
        if (name == "native") { p.native = string(value); continue; }
        if (name == "events") {
            require(json_is_array(value), "Engine events must be an array");
            size_t i; json_t* event;
            json_array_foreach(value, i, event) {
                require(json_is_object(event) && json_object_size(event) == 2, "Unknown engine event fields");
                auto block = json_object_get(event, "block");
                require(json_is_integer(block) && json_integer_value(block) >= 0, "Invalid event block");
                p.events.push_back({size_t(json_integer_value(block)), string(json_object_get(event, "kind"))});
            }
            continue;
        }
        require(json_is_integer(value) && json_integer_value(value) >= 0 && json_integer_value(value) <= 1000000, "Invalid engine profile integer");
        const size_t v = json_integer_value(value);
        if (name == "threads") p.threads = v;
        else if (name == "analyzers") p.analyzers = v;
        else if (name == "background") p.background = v;
        else if (name == "consumer_hz") p.consumer_hz = v;
        else if (name == "stall_every") p.stall_every = v;
        else if (name == "stall_polls") p.stall_polls = v;
        else if (name == "blocks") p.blocks = v;
        else if (name == "warm_hops") p.warm_hops = v;
        else throw std::runtime_error("Unknown engine profile field: "+name);
    }
    p.c.callbacks = p.blocks; p.c.warm_hops = p.warm_hops;
    validate_backend(p.c);
    require(p.c.workload_schema == 3 && p.c.count == 1 && p.c.load == 0 && p.c.cache_mib == 0
        && p.c.callback_offset == 0 && p.c.pass == "callback" && p.c.rate == 48000
        && (p.c.state == "startup" || p.c.state == "steady"),
        "Engine profiles require explicit isolated 48 kHz module controls");
    require(p.threads == 1 || p.threads == 4, "Engine threads must be 1 or 4");
    require(p.analyzers == 0 || p.analyzers == 1 || p.analyzers == 4 || p.analyzers == 16, "Invalid engine analyzer count");
    require(p.c.block == 64 || p.c.block == 256, "Engine blocks must be 64 or 256");
    require(p.background == 0 || p.background == 16 || p.background == 64, "Background sweep must remain fixed across contenders");
    require(p.consumer_hz == 0 || p.consumer_hz == 30 || p.consumer_hz == 60, "Unsupported consumer cadence");
    require(p.blocks >= 2 && p.blocks <= 100000 && p.warm_hops <= 128 && p.stall_polls <= 10, "Engine run is outside prepared bounds");
    require(!p.stall_every || (p.consumer_hz && p.stall_every >= 2), "Stalls require a consumer");
    for (size_t i = 0; i < p.events.size(); ++i) {
        const auto& e = p.events[i];
        require(e.block < p.blocks && (!i || p.events[i-1].block < e.block), "Events must be ordered at distinct blocks");
        require(e.kind == "reset" || e.kind == "freeze" || e.kind == "resume" || e.kind == "sample-rate"
            || e.kind == "window" || e.kind == "band" || e.kind == "geometry", "Unknown lifecycle event");
    }
    require(p.events.empty() || (p.c.execution_regime == "continuous" && !p.consumer_hz),
        "Lifecycle diagnostics are separate continuous passes without a concurrent consumer");
    const std::string kind(backend_descriptor(p.c.backend).kind);
    require(kind == "fourier" || kind == "spectre", "Engine requires complete module paths");
    if (!p.native.empty()) {
        const auto& d = backend_descriptor(p.native);
        require(std::string(d.kind) == "native-analysis" && std::string(d.precision) == "float"
            && d.channels == (kind == "fourier" ? 4 : 1), "Native module channel/precision mismatch");
    }
    return p;
}

struct Snapshot {
    uint64_t sequence = 0, checksum = 0;
    int64_t endpoint = -1, published_at = -1;
    size_t column = 0, bins = 0;
};
inline uint64_t mix(uint64_t hash, float value) {
    uint32_t bits; std::memcpy(&bits, &value, sizeof(bits));
    return (hash^bits)*UINT64_C(1099511628211);
}
inline Snapshot inspect(const SpectrumAnalyzer::DisplaySpectrum& s, size_t = 0) {
    require(s.count <= SpectrumAnalyzer::MAX_FFT/2+1, "Torn Fourier snapshot count");
    Snapshot r; r.sequence = s.sequence; r.endpoint = s.endpoint; r.published_at = s.published_at; r.bins = s.count;
    for (size_t c = 0; c < 4; ++c) for (size_t k = 0; k < s.count; ++k) {
        require(std::isfinite(s.points[c][k].x) && !std::isnan(s.points[c][k].y), "Invalid Fourier snapshot");
        r.checksum = mix(mix(r.checksum, s.points[c][k].x), s.points[c][k].y);
    }
    return r;
}
inline Snapshot inspect(const Spectrogram::DisplayColumn& s, size_t column) {
    Snapshot r; r.sequence = s.revision; r.endpoint = s.endpoint; r.published_at = s.published_at;
    r.column = column; r.bins = s.values.size();
    for (float value : s.values) { require(std::isfinite(value), "Invalid Spectre snapshot"); r.checksum = mix(r.checksum, value); }
    return r;
}
/// @brief Borrowed synchronous callback; no heap-backed function wrapper in polls.
struct Consume {
    const void* context;
    void (*callback)(const void*, const Snapshot&);
    template<typename F> Consume(const F& f) : context(&f), callback([](const void* p, const Snapshot& s) {
        (*static_cast<const F*>(p))(s);
    }) {}
    void operator()(const Snapshot& s) const { callback(context, s); }
};
template<typename SnapshotType>
void consume_stable(const SnapshotType& value, size_t column, const Consume& accept) {
    const auto before = inspect(value, column); accept(before);
    const auto after = inspect(value, column);
    require(before.sequence == after.sequence && before.checksum == after.checksum
        && before.endpoint == after.endpoint && before.published_at == after.published_at
        && before.bins == after.bins, "Producer modified a consumer-owned snapshot");
}
inline void consume(SpectrumAnalyzer& m, const Consume& accept) { consume_stable(m.consume_display_spectrum(), 0, accept); }
inline void consume(Spectrogram& m, const Consume& accept) {
    for (size_t i = 0; i < Spectrogram::N_STFT; ++i) if (auto s = m.consume_display_column(i)) consume_stable(*s, i, accept);
}
template<typename K> void consume(NativeFourier<K>& m, const Consume& accept) { consume_stable(m.consume(), 0, accept); }
template<typename K> void consume(NativeSpectre<K>& m, const Consume& accept) {
    for (size_t i = 0; i < Spectrogram::N_STFT; ++i) if (auto s = m.consume(i)) consume_stable(*s, i, accept);
}

struct Node : rack::engine::Module {
    size_t calls = 0;
    std::vector<Snapshot> publications;
    virtual void audit(int64_t) = 0;
    virtual std::string accuracy_json() = 0;
    Node() { config(0, 0, 0, 0); }
    void onReset() override { change("reset"); }
    virtual void read(const Consume&) = 0;
    virtual void change(const std::string&) = 0;
    virtual std::string controls() = 0;
};
/// @brief Host mutations occur only after stepBlock returns and workers wait.
template<typename Module> void controls_event(Module& m, const std::string& kind) {
    if (kind == "reset") m.onReset();  // Explicit state reset; retain panel quantities.
    else if (kind == "freeze" || kind == "resume") {
        auto data = own(json_object());
        json_object_set_new(data.get(), "is_running", json_boolean(kind == "resume")); m.dataFromJson(data.get());
    } else if (kind == "sample-rate") m.onSampleRateChange();
    else if (kind == "window") m.set_window_function(Fourier::Window::Function::BlackmanHarris);
    else if (kind == "band") m.set_frequency_smoothing(FrequencySmoothing::_1_3);
    else if (kind == "geometry") { m.set_slope(3); m.set_low_frequency(20); m.set_high_frequency(16000); }
}

/// @brief Only untimed replay consumes and compares every published output.
template<typename Module, typename ReadSnapshots, typename ReadValues>
void audit_frame(ModuleLifecycleReference<Module>& oracle, const std::vector<float>& input,
        int64_t frame, uint64_t sequence, uint64_t& last_sequence, ReadSnapshots read,
        ReadValues values, std::vector<Snapshot>& publications) {
    const bool complete = oracle.step(input, oracle.host.cursor-1, frame);
    require((sequence != last_sequence) == complete, "Lifecycle publication cadence differs");
    last_sequence = sequence;
    if (!complete) return;
    bool found = false;
    read([&](const Snapshot& snapshot) {
        if (snapshot.sequence != sequence) return;
        require(snapshot.endpoint == oracle.endpoint_frame && snapshot.published_at == frame,
            "Lifecycle endpoint/publication timestamp differs");
        publications.push_back(snapshot); found = true;
    });
    require(found, "Published snapshot missing from replay");
    for (size_t c = 0; c < oracle.lanes.size(); ++c) oracle.compare(c, values(c, oracle.geometry));
    ++oracle.accuracy.publications;
}
inline std::vector<float> replay_values(SpectrumAnalyzer& m, size_t channel, const ModuleGeometry& geometry, size_t n) {
    return module_magnitudes(m.consume_display_spectrum(), geometry, channel, n);
}
inline std::vector<float> replay_values(Spectrogram& m, size_t channel, const ModuleGeometry&, size_t n) {
    return module_magnitudes(m, channel, n);
}
template<typename K>
std::vector<float> replay_values(NativeFourier<K>& m, size_t channel, const ModuleGeometry& geometry, size_t n) {
    return module_magnitudes(m.consume(), geometry, channel, n);
}
template<typename K>
std::vector<float> replay_values(NativeSpectre<K>& m, size_t, const ModuleGeometry&, size_t) {
    const auto& s = m.mailboxes[(m.column+Spectrogram::N_STFT-1)%Spectrogram::N_STFT].current();
    return {s.values.begin(), s.values.end()};
}
template<typename Module>
std::string oracle_json(const ModuleLifecycleReference<Module>* oracle) {
    require(oracle, "Missing engine oracle");
    std::ostringstream out;
    out << "{\"publications\":" << oracle->accuracy.publications
        << ",\"values\":" << oracle->accuracy.checked << ",\"analysis\":";
    out << oracle->accuracy.analysis.json();
    out << '}'; return out.str();
}

template<typename ModuleType>
struct ProductionNode : Node {
    Host<ModuleType> host;
    std::vector<float> input;
    std::unique_ptr<ModuleLifecycleReference<ModuleType>> oracle;
    uint64_t last_sequence;
    ProductionNode(const Config& c, bool verify) : host(c), input(signal(c)), last_sequence(host.module.benchmark_publications()) {
        if (verify) oracle.reset(new ModuleLifecycleReference<ModuleType>(host));
    }
    void process(const ProcessArgs& args) override {
        host.args = args; host.process(input_sample(host.config, input, host.cursor)); ++calls;
    }
    void read(const Consume& accept) override { consume(host.module, accept); }
    void audit(int64_t frame) override {
        if (oracle) audit_frame(*oracle, input, frame, host.module.benchmark_publications(), last_sequence,
            [&](const Consume& accept) { read(accept); },
            [&](size_t c, const ModuleGeometry& g) { return replay_values(host.module, c, g, host.config.n); }, publications);
    }
    std::string accuracy_json() override { return oracle_json(oracle.get()); }
    void change(const std::string& kind) override {
        controls_event(host.module, kind);
        host.config.rate = host.module.get_sample_rate(); host.config.hop = host.module.get_hop_length();
        if (oracle) oracle->change(kind);
        last_sequence = host.module.benchmark_publications();
    }
    std::string controls() override { return module_settings_json(host.module, host.config); }
};

template<typename Adapter>
struct NativeNode : Node {
    Adapter adapter;
    std::vector<float> input;
    std::unique_ptr<ModuleLifecycleReference<typename Adapter::Module>> oracle;
    uint64_t last_sequence;
    NativeNode(const Config& c, const std::string& backend, bool verify) : adapter(c, backend), input(signal(c)), last_sequence(adapter.sequence) {
        if (verify) oracle.reset(new ModuleLifecycleReference<typename Adapter::Module>(adapter.host,
            std::string(backend_descriptor(backend).schedule) == "immediate", true));
    }
    void process(const ProcessArgs& args) override {
        adapter.process(args, input_sample(adapter.host.config, input, adapter.host.cursor)); ++calls;
    }
    void read(const Consume& accept) override { consume(adapter, accept); }
    void audit(int64_t frame) override {
        if (oracle) audit_frame(*oracle, input, frame, adapter.sequence, last_sequence,
            [&](const Consume& accept) { read(accept); },
            [&](size_t c, const ModuleGeometry& g) { return replay_values(adapter, c, g, adapter.host.config.n); }, publications);
    }
    std::string accuracy_json() override { return oracle_json(oracle.get()); }
    void change(const std::string& kind) override {
        const bool running = adapter.running;
        controls_event(adapter.host.module, kind);
        if (kind == "reset" || kind == "sample-rate") adapter.reset_analysis(kind == "reset");
        if (kind == "sample-rate") adapter.running = running;
        if (kind == "freeze" || kind == "resume") adapter.running = kind == "resume";
        if (oracle) oracle->change(kind);
        last_sequence = adapter.sequence;
    }
    std::string controls() override {
        auto value = own(json_loads(module_settings_json(adapter.host.module, adapter.host.config).c_str(), 0, nullptr));
        set(value, "native", own(json_loads(adapter.analysis->info_json().c_str(), 0, nullptr)));
        set(value, "storage_limitation", "Retains unused production analyzer/display storage in control shell");
        return dump(value.get());
    }
};

inline std::unique_ptr<Node> node(const Plan& p, bool verify) {
    const bool fourier = std::string(backend_descriptor(p.c.backend).kind) == "fourier";
    if (p.native.empty()) {
        if (fourier) return std::unique_ptr<Node>(new ProductionNode<SpectrumAnalyzer>(p.c, verify));
        return std::unique_ptr<Node>(new ProductionNode<Spectrogram>(p.c, verify));
    }
    const auto& d = backend_descriptor(p.native); const std::string provider(d.provider);
    if (provider == "pffft") {
        if (p.native.find("unordered") != std::string::npos)
            return std::unique_ptr<Node>(new NativeNode<NativeSpectre<PffftNative<true>>>(p.c, p.native, verify));
        return std::unique_ptr<Node>(new NativeNode<NativeSpectre<PffftNative<false>>>(p.c, p.native, verify));
    }
#ifdef PAPER_HAVE_VDSP
    if (provider == "vdsp") {
        if (fourier) return std::unique_ptr<Node>(new NativeNode<NativeFourier<VdspNative<float, 4>>>(p.c, p.native, verify));
        return std::unique_ptr<Node>(new NativeNode<NativeSpectre<VdspNative<float>>>(p.c, p.native, verify));
    }
#endif
#ifdef PAPER_HAVE_FFTW
    if (provider == "fftw") {
        if (fourier) return std::unique_ptr<Node>(new NativeNode<NativeFourier<FftwNative<float, 4>>>(p.c, p.native, verify));
        return std::unique_ptr<Node>(new NativeNode<NativeSpectre<FftwNative<float>>>(p.c, p.native, verify));
    }
#endif
    throw std::runtime_error("Unsupported native module provider");
}
struct Load : rack::engine::Module {
    Background background;
    explicit Load(size_t filters) : background(filters) { config(0,0,0,0); }
    void process(const ProcessArgs&) override { background.process(.125f); }
};

/// @brief One master caller; workers and graph are destroyed after consumers join.
struct Graph {
    Context context;
    Plan plan;
    std::vector<std::unique_ptr<Node>> nodes;
    std::unique_ptr<Load> load;
    const int previous_threads;
    const bool previous_meter;
    explicit Graph(const Plan& p, bool verify = false) : context(p.c.rate), plan(p), previous_threads(rack::settings::threadCount),
        previous_meter(rack::settings::cpuMeter) {
        rack::settings::threadCount = p.threads; rack::settings::cpuMeter = false;
        context.context.engine->stepBlock(1); // Launch workers before plans/settling; no analyzers yet.
        try {
            for (size_t i = 0; i < p.analyzers; ++i) {
                nodes.push_back(node(p, verify)); auto& item = nodes.back(); item->id = i+1;
                context.context.engine->addModule(item.get());
                const size_t offset = p.c.alignment == "staggered" ? i*p.c.hop/p.analyzers : 0;
                rack::engine::Module::ProcessArgs args{}; args.sampleRate = p.c.rate; args.sampleTime = 1.f/p.c.rate;
                for (size_t s = 0; s < offset; ++s) { args.frame = int64_t(s)-int64_t(offset); item->process(args); item->audit(args.frame); }
            }
            if (p.background) { load.reset(new Load(p.background)); load->id = 100; context.context.engine->addModule(load.get()); }
        } catch (...) { detach(); rack::settings::threadCount = previous_threads; rack::settings::cpuMeter = previous_meter; throw; }
    }
    void detach() {
        if (load) context.context.engine->removeModule(load.get());
        for (auto& n : nodes) context.context.engine->removeModule(n.get());
        context.context.engine->setMasterModule(nullptr);
    }
    ~Graph() {
        detach(); delete context.context.engine; context.context.engine = nullptr;
        rack::settings::threadCount = previous_threads; rack::settings::cpuMeter = previous_meter;
    }
    void step(size_t samples) { context.context.engine->stepBlock(int(samples)); }
    int64_t frame() { return context.context.engine->getFrame(); }
    void event(const std::string& kind) {
        if (kind == "sample-rate") context.context.engine->setSampleRate(96000);
        for (auto& n : nodes) {
            if (kind == "reset") context.context.engine->resetModule(n.get());
            else n->change(kind);
        }
    }
};

struct Consumption { size_t node; Snapshot snapshot; int64_t before, after; bool drain; };
/// @brief Only this object consumes mailboxes. Metadata never reads live controls.
struct Consumer {
    Graph& graph;
    std::atomic<int64_t>& progress;
    std::vector<Consumption> rows;
    std::vector<std::vector<uint64_t>> seen;
    std::vector<uint64_t> initial, latest;
    std::atomic<bool> stop{false}, ready{false}, go{false};
    std::thread thread;
    std::mutex mutex;
    std::condition_variable start_condition;
    std::exception_ptr failure;
    explicit Consumer(Graph& g, std::atomic<int64_t>& p) : graph(g), progress(p),
        seen(g.nodes.size(), std::vector<uint64_t>(Spectrogram::N_STFT)), initial(g.nodes.size()), latest(g.nodes.size()) {
        // At most one update per engine sample plus reset column publications.
        const size_t capacity = g.nodes.size()*(g.plan.blocks*g.plan.c.block/120+1024+512*g.plan.events.size());
        rows.reserve(capacity);
    }
    void poll(bool baseline = false, bool drain = false) {
        for (size_t i = 0; i < graph.nodes.size(); ++i) {
            const auto before = progress.load(std::memory_order_acquire);
            graph.nodes[i]->read([&](const Snapshot& value) {
                require(value.column < seen[i].size(), "Invalid snapshot column");
                if (value.sequence <= seen[i][value.column]) return;
                seen[i][value.column] = value.sequence; latest[i] = std::max(latest[i], value.sequence);
                if (baseline) return;
                if (value.sequence <= initial[i]) return;
                require(rows.size() < rows.capacity(), "Preallocated consumer observation capacity exceeded");
                rows.push_back({i, value, before, progress.load(std::memory_order_acquire), drain});
            });
        }
        if (baseline) initial = latest;
    }
    void start() {
        thread = std::thread([this]() {
            try {
                ready.store(true, std::memory_order_release);
                { std::unique_lock<std::mutex> lock(mutex); start_condition.wait(lock, [&]() { return go.load() || stop.load(); }); }
                if (stop.load(std::memory_order_acquire)) return;
                poll(true); ready.store(false, std::memory_order_release);
                size_t tick = 0; const auto epoch = std::chrono::steady_clock::now();
                while (!stop.load(std::memory_order_acquire)) {
                    ++tick;
                    if (graph.plan.stall_every && tick%graph.plan.stall_every == 0) tick += graph.plan.stall_polls;
                    std::this_thread::sleep_until(epoch+std::chrono::nanoseconds(Execution::offset_ns(tick, graph.plan.consumer_hz)));
                    if (!stop.load(std::memory_order_acquire)) poll();
                }
                poll(false, true);
            } catch (...) { failure = std::current_exception(); ready.store(false, std::memory_order_release); }
        });
        while (!ready.load(std::memory_order_acquire)) std::this_thread::yield();
    }
    void begin() { { std::lock_guard<std::mutex> lock(mutex); go.store(true); } start_condition.notify_one();
        while (ready.load(std::memory_order_acquire)) std::this_thread::yield();
        if (failure) std::rethrow_exception(failure); }
    void finish() { { std::lock_guard<std::mutex> lock(mutex); stop.store(true); } start_condition.notify_one(); if (thread.joinable()) thread.join(); if (failure) std::rethrow_exception(failure); }
    ~Consumer() { { std::lock_guard<std::mutex> lock(mutex); stop.store(true); } start_condition.notify_one(); if (thread.joinable()) thread.join(); }
};

inline int64_t cpu_ns() {
#ifdef PAPER_FIXTURE_CLOCK
    static int64_t count = 0; return ++count*100;
#elif defined(_WIN32)
    throw std::runtime_error("Engine process CPU observations are not implemented on Windows");
#else
    rusage usage{}; require(getrusage(RUSAGE_SELF, &usage) == 0, "Cannot read process CPU cost");
    return (int64_t(usage.ru_utime.tv_sec)+usage.ru_stime.tv_sec)*1000000000LL
        +(int64_t(usage.ru_utime.tv_usec)+usage.ru_stime.tv_usec)*1000;
#endif
}
/// @brief Replaying one sample per block exposes every publication without timing.
inline std::string verify(const Plan& p, const std::vector<Consumption>& consumed = {}) {
    Graph graph(p, true);
    const size_t warm = ((p.warm_hops*p.c.hop+p.c.block-1)/p.c.block)*p.c.block;
    auto step = [&]() {
        const auto frame = graph.frame(); graph.step(1);
        for (size_t i = 0; i < graph.nodes.size(); ++i) {
            try { graph.nodes[i]->audit(frame); }
            catch (const std::exception& e) { throw std::runtime_error(p.c.backend+"/"+p.native+" node="+std::to_string(i)
                +" frame="+std::to_string(frame)+": "+e.what()); }
        }
    };
    for (size_t i = 0; i < warm; ++i) step();
    size_t event = 0;
    const auto origin = graph.frame();
    for (size_t b = 0; b < p.blocks; ++b) {
        if (event < p.events.size() && p.events[event].block == b) graph.event(p.events[event++].kind);
        for (size_t i = 0; i < p.c.block; ++i) step();
    }
    // Every consumed immutable snapshot must match a complete independently
    // checked publication. This is after timing and uses a different graph.
    // Native replanning can change rounding; ownership hashes are checked while
    // the original consumer holds its snapshot, not against a fresh plan.
    for (const auto& row : consumed) {
        require(row.node < graph.nodes.size(), "Unknown consumed node");
        const auto& trace = graph.nodes[row.node]->publications;
        const auto match = std::find_if(trace.begin(), trace.end(), [&](const Snapshot& s) { return s.sequence == row.snapshot.sequence; });
        require(match != trace.end() && match->endpoint == row.snapshot.endpoint && match->published_at == row.snapshot.published_at
            && match->column == row.snapshot.column && match->bins == row.snapshot.bins,
            "Concurrent snapshot differs from independent complete-frame replay");
    }
    std::ostringstream out;
    out << "{\"policy\":\"rack-module-replay-v1\",\"origin\":" << origin << ",\"nodes\":[";
    for (size_t i = 0; i < graph.nodes.size(); ++i) {
        if (i) out << ',';
        auto& n = graph.nodes[i];
        out << "{\"accuracy\":" << n->accuracy_json() << ",\"controls\":" << n->controls() << ",\"publications\":[";
        bool comma = false;
        for (const auto& r : n->publications) {
            if (comma) out << ','; comma = true;
            out << "{\"sequence\":" << r.sequence << ",\"endpoint\":" << r.endpoint
                << ",\"published_at\":" << r.published_at << ",\"column\":" << r.column << ",\"bins\":" << r.bins << '}';
        }
        out << "]}";
    }
    out << "]}"; return out.str();
}

struct LifecycleCost { size_t block; int64_t ns; std::string kind; };
/// @brief Full Rack stepBlock wall intervals; no oracle or mailbox poll on workers.
inline std::string measure(const Plan& p) {
    std::ostringstream out;
    std::vector<Consumption> consumption;
    {
        Graph graph(p);
        std::atomic<int64_t> progress{graph.frame()};
        std::unique_ptr<Consumer> consumer;
        if (p.consumer_hz) { consumer.reset(new Consumer(graph, progress)); consumer->start(); }
        Execution::Session session("callback");
        require(session.policy.regime == p.c.execution_regime, "Engine execution regime mismatch");
        auto& observations = session.reserve(p.blocks);
        std::vector<size_t> rates(p.blocks);
        std::vector<LifecycleCost> lifecycle; lifecycle.reserve(p.events.size());
        session.settle();
        const size_t warm_blocks = (p.warm_hops*p.c.hop+p.c.block-1)/p.c.block;
        for (size_t i = 0; i < warm_blocks; ++i) graph.step(p.c.block);
        progress.store(graph.frame(), std::memory_order_release);
        if (consumer) consumer->begin();
        const auto origin = graph.frame();
        size_t block = 0, event = 0;
        Execution::SystemClock clock;
        const auto cpu_begin = cpu_ns();
        Execution::measure(session.policy, "callback", p.blocks, p.c.block, p.c.rate, clock,
            [&](size_t count) { graph.step(count); progress.store(graph.frame(), std::memory_order_release); ++block; },
            [&]() {
                if (event < p.events.size() && p.events[event].block == block) {
                    const auto& change = p.events[event++]; const auto start = clock.now();
                    graph.event(change.kind); const auto finish = clock.now();
                    lifecycle.push_back({block, finish-start, change.kind});
                }
                rates[block] = graph.context.context.engine->getSampleRate();
            }, observations);
        const auto cpu_cost = cpu_ns()-cpu_begin;
        if (consumer) { consumer->finish(); consumption = std::move(consumer->rows); }
        for (size_t i = 0; i < graph.nodes.size(); ++i) {
            const size_t offset = p.c.alignment == "staggered" ? i*p.c.hop/p.analyzers : 0;
            require(graph.nodes[i]->calls == offset+(warm_blocks+p.blocks)*p.c.block, "Lost engine processing calls");
        }
        session.measured(); session.finish();
        out << "{\"schema\":1,\"policy\":\"rack-engine-observations-v1\",\"fixture\":";
#ifdef PAPER_FIXTURE_CLOCK
        out << "true";
#else
        out << "false";
#endif
        out << ",\"origin\":" << origin << ",\"aggregate_cpu_ns\":" << cpu_cost << ",\"observations\":[";
        for (size_t i = 0; i < observations.size(); ++i) {
            if (i) out << ','; const auto& r = observations[i];
            out << "{\"block\":" << i << ",\"frame\":" << origin+r.sample << ",\"samples\":" << r.samples
                << ",\"rate\":" << rates[i] << ",\"duration_ns\":" << r.finish-r.start
                << ",\"wake_ns\":" << r.wake << ",\"start_ns\":" << r.start << ",\"finish_ns\":" << r.finish
                << ",\"release_ns\":" << r.release << ",\"deadline_ns\":" << r.deadline << '}';
        }
        out << "],\"lifecycle\":[";
        for (size_t i = 0; i < lifecycle.size(); ++i) {
            if (i) out << ','; const auto& r = lifecycle[i];
            out << "{\"block\":" << r.block << ",\"kind\":\"" << r.kind << "\",\"duration_ns\":" << r.ns << '}';
        }
        out << "],\"consumption\":[";
        for (size_t i = 0; i < consumption.size(); ++i) {
            if (i) out << ','; const auto& r = consumption[i];
            out << "{\"node\":" << r.node << ",\"sequence\":" << r.snapshot.sequence
                << ",\"endpoint\":" << r.snapshot.endpoint << ",\"published_at\":" << r.snapshot.published_at
                << ",\"checksum\":" << r.snapshot.checksum << ",\"stable\":true"
                << ",\"before\":" << r.before << ",\"after\":" << r.after << ",\"drain\":" << (r.drain ? "true" : "false") << '}';
        }
        out << ']';
    }
    out << ",\"replay\":" << verify(p, consumption) << '}'; return out.str();
}

/// @brief C++ lifecycle allocations in a separate single-thread graph, no clocks.
inline std::string resources(Plan p) {
#ifndef PAPER_ALLOCATION_AUDIT
    throw std::runtime_error("Engine allocation replay requires paper-audit");
#else
    p.threads = 1; p.consumer_hz = 0;
    Graph graph(p);
    std::ostringstream out;
    out << "{\"policy\":\"rack-lifecycle-allocations-v1\",\"scope\":\"serialized C++ new/delete only; native allocator calls unavailable\",\"events\":[";
    for (size_t i = 0; i < p.warm_hops; ++i) graph.step(p.c.hop);
    size_t event = 0;
    for (size_t b = 0; b < p.blocks; ++b) {
        if (event < p.events.size() && p.events[event].block == b) {
            const auto& e = p.events[event];
            PaperResources::reset_phase(); PaperResources::active = true;
            try { graph.event(e.kind); } catch (...) { PaperResources::active = false; throw; }
            PaperResources::active = false;
            if (event++) out << ',';
            out << "{\"block\":" << b << ",\"kind\":\"" << e.kind << "\",\"allocations\":"
                << PaperResources::counts.allocations << ",\"allocated_bytes\":" << PaperResources::counts.allocated << '}';
        }
        graph.step(p.c.block);
    }
    out << "]}"; return out.str();
#endif
}

inline int run(const std::string& path, bool verify_only) {
    const auto json = Development::read_json(path); const auto plan = parse(json.get());
#ifdef PAPER_ALLOCATION_AUDIT
    throw std::runtime_error("Engine timing/parallel replay requires the uninstrumented paper binary");
#endif
    if (verify_only) { std::cout << verify(plan) << '\n'; return 0; }
    require(!Execution::environment("PAPER_HOST_EXECUTION_PATH").empty(), "Retain engine execution sidecar with PAPER_HOST_EXECUTION_PATH");
    Execution::output_override() = Execution::environment("PAPER_HOST_EXECUTION_PATH");
    const auto result = measure(plan);
    std::cout << "{\"profile\":" << dump(json.get()) << ",\"result\":" << result << "}\n";
    return 0;
}
}  // namespace EngineHost
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_ENGINE_HOST_HPP_
