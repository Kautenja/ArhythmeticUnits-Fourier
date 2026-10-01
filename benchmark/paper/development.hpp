// Native development campaigns, raw artifacts and compatible baseline comparison.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_DEVELOPMENT_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_DEVELOPMENT_HPP_

#include <cerrno>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <random>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include <jansson.h>
#include "development_matrix.hpp"
#include "workload_json.hpp"

namespace Paper {
namespace Development {

using Json = std::shared_ptr<json_t>;
inline Json own(json_t* value) {
    require(value != nullptr, "JSON allocation or parsing failed");
    return Json(value, json_decref);
}
inline Json object() { return own(json_object()); }
inline void set(const Json& j, const char* key, const std::string& value) {
    require(json_object_set_new(j.get(), key, json_string(value.c_str())) == 0, "Invalid JSON string");
}
inline void set(const Json& j, const char* key, double value) {
    require(std::isfinite(value) && json_object_set_new(j.get(), key, json_real(value)) == 0,
        "Invalid JSON number");
}
inline void set(const Json& j, const char* key, const Json& value) {
    require(json_object_set(j.get(), key, value.get()) == 0, "Cannot store JSON value");
}
inline std::string string(json_t* j) {
    require(json_is_string(j), "Expected JSON string");
    return json_string_value(j);
}
inline std::string dump(json_t* j) {
    char* raw = json_dumps(j, JSON_SORT_KEYS | JSON_COMPACT | JSON_ENCODE_ANY);
    require(raw != nullptr, "Cannot encode JSON");
    const std::string text(raw); std::free(raw); return text;
}
inline Json read_json(const std::string& path) {
    json_error_t error{};
    json_t* j = json_load_file(path.c_str(), JSON_REJECT_DUPLICATES, &error);
    if (!j) throw std::runtime_error(path + ": " + error.text);
    return own(j);
}
inline void write_json(const std::string& path, const Json& j) {
    require(json_dump_file(j.get(), path.c_str(), JSON_INDENT(2) | JSON_SORT_KEYS) == 0,
        "Cannot write result JSON");
}
inline std::string read_text(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    require(bool(in), ("Cannot read " + path).c_str());
    std::ostringstream out; out << in.rdbuf();
    require(!in.bad(), "File read failed"); return out.str();
}
inline void write_text(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    out << text; out.close(); require(bool(out), "File write failed");
}

/// @brief Stable byte fingerprint, not a cryptographic archival hash.
inline std::string fingerprint(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    require(bool(in), ("Cannot fingerprint " + path).c_str());
    uint64_t hash = UINT64_C(14695981039346656037);
    char bytes[65536];
    while (in) {
        in.read(bytes, sizeof(bytes));
        for (std::streamsize i = 0; i < in.gcount(); ++i) {
            hash ^= static_cast<unsigned char>(bytes[i]); hash *= UINT64_C(1099511628211);
        }
    }
    require(!in.bad(), "Fingerprint read failed");
    std::ostringstream out; out << std::hex << std::setfill('0') << std::setw(16) << hash;
    return out.str();
}

/// @brief Fixed read-only provenance commands only; no user text enters a shell.
inline std::string capture(const char* command) {
#ifdef _WIN32
    FILE* pipe = _popen(command, "r");
#else
    FILE* pipe = popen(command, "r");
#endif
    require(pipe != nullptr, "Cannot read build provenance");
    std::string result; char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
#ifdef _WIN32
    const int status = _pclose(pipe);
#else
    const int status = pclose(pipe);
#endif
    require(status == 0, "Provenance command failed; run from the repository root");
    return result;
}

inline Json source_identity() {
    auto result = object();
    std::istringstream paths(capture("git ls-files --cached --others --exclude-standard -- src benchmark mk Makefile docs/whitepaper/benchmarks test/paper"));
    std::string path;
    while (std::getline(paths, path)) {
        // Uncommitted moves leave deleted paths in the Git index.
        struct stat status{};
        if (stat(path.c_str(), &status) == 0) set(result, path.c_str(), fingerprint(path));
        else require(errno == ENOENT, "Cannot inspect benchmark source");
    }
    return result;
}

inline Json environment() {
    auto j = object();
    set(j, "compiler", __VERSION__);
    set(j, "rack_build", read_text(".build/rack-config"));
    set(j, "paper_build", read_text(".build/benchmark/paper-config"));
    set(j, "registry", own(json_loads(registry_json, 0, nullptr)));
#ifdef _WIN32
    const char* host = std::getenv("COMPUTERNAME");
    set(j, "host", host ? host : "unknown-windows-host");
#else
    set(j, "host", capture("uname -nsmr"));
#endif
#ifdef PAPER_RACK_LIBRARY
    set(j, "rack_library_fnv1a64", fingerprint(PAPER_RACK_LIBRARY));
#endif
#ifdef PAPER_FFTW_DIRECTORY
    set(j, "fftw_float_fnv1a64", fingerprint(std::string(PAPER_FFTW_DIRECTORY)+"/lib/libfftw3f.a"));
    set(j, "fftw_double_fnv1a64", fingerprint(std::string(PAPER_FFTW_DIRECTORY)+"/lib/libfftw3.a"));
#endif
    return j;
}

inline Config config_from_json(json_t* j) {
    require(json_is_object(j), "Workload must be an object");
    Config c = base();
    if (json_object_get(j, "workload_schema")) parse_workload_controls(c, j, false);
    const char* key; json_t* value;
    json_object_foreach(j, key, value) {
        const std::string name(key);
        if (c.workload_schema == 3 && control_fields().count(name)) continue;
        if (name == "backend") c.backend = string(value);
        else if (name == "pass_name") c.pass = string(value);
        else if (name == "alignment") c.alignment = string(value);
        else if (name == "state") c.state = string(value);
        else {
            require(json_is_integer(value) && json_integer_value(value) >= 0
                && json_integer_value(value) <= 1000000, "Invalid workload integer");
            const size_t v = size_t(json_integer_value(value));
            if (name == "n") c.n = v;
            else if (name == "hop") c.hop = v;
            else if (name == "block") c.block = v;
            else if (name == "count") c.count = v;
            else if (name == "load") c.load = v;
            else if (name == "voices") c.voices = v;
            else if (name == "rate") c.rate = float(v);
            else if (name == "cache_mib") c.cache_mib = v;
            else if (name == "callback_offset") c.callback_offset = v;
            else if (name == "smooth") { require(v <= 1, "Invalid smoothing flag"); c.smooth = v; }
            else throw std::runtime_error("Unknown workload field: " + name);
        }
    }
    return c;
}

inline std::vector<Config> matrix(const Options& o) {
    auto rows = builtin(o);
    if (!o.config.empty()) {
        const auto j = read_json(o.config);
        require(json_is_array(j.get()), "--config expects a frozen JSON workload array");
        rows.clear(); size_t i; json_t* value;
        json_array_foreach(j.get(), i, value) rows.push_back(config_from_json(value));
    }
    return select(rows, o);
}

inline Json config_json(const Config& c) {
    auto j = object();
    set(j, "backend", c.backend); set(j, "pass_name", c.pass);
    set(j, "alignment", c.alignment); set(j, "state", c.state);
    set(j, "n", c.n); set(j, "hop", c.hop); set(j, "block", c.block);
    set(j, "count", c.count); set(j, "load", c.load); set(j, "voices", c.voices);
    set(j, "rate", c.rate); set(j, "smooth", c.smooth ? 1 : 0);
    set(j, "cache_mib", c.cache_mib); set(j, "callback_offset", c.callback_offset);
    set(j, "callbacks", c.callbacks); set(j, "warm_hops", c.warm_hops);
    if (c.workload_schema == 3) {
        auto controls = own(json_loads(workload_controls_json(c).c_str(), 0, nullptr));
        require(json_object_update(j.get(), controls.get()) == 0, "Cannot store workload controls");
    }
    set(j, "cost_unit", c.pass == "callback" || c.pass == "throughput"
        ? "ns/engine-sample" : "ns/transform");
    set(j, "contract", own(json_loads(contract_json(c).c_str(), 0, nullptr)));
    return j;
}

inline Json summary_json(const Summary& s) {
    auto j = object();
    set(j, "observations", s.observations); set(j, "samples", s.samples);
    set(j, "cost", s.cost); set(j, "p50_ns", s.p50); set(j, "p95_ns", s.p95);
    set(j, "p99_ns", s.p99); set(j, "observed_max_ns", s.maximum);
    set(j, "timer_p99_ns", s.timer_p99); set(j, "budget_exceedances", s.exceedances);
    return j;
}

/// @brief Each repetition contributes equally; callback rows are not replicates.
inline Json aggregate(const std::vector<Summary>& repetitions) {
    require(!repetitions.empty(), "Missing repetitions");
    auto j = object();
    std::vector<double> costs, p99;
    double total = 0, maximum = 0;
    for (const auto& s : repetitions) {
        costs.push_back(s.cost); p99.push_back(s.p99); total += s.cost;
        maximum = std::max(maximum, s.maximum);
    }
    set(j, "mean_cost", total/repetitions.size());
    set(j, "min_cost", *std::min_element(costs.begin(), costs.end()));
    set(j, "max_cost", *std::max_element(costs.begin(), costs.end()));
    std::sort(p99.begin(), p99.end());
    set(j, "median_p99_ns", (p99[(p99.size()-1)/2]+p99[p99.size()/2])/2);
    set(j, "observed_max_ns", maximum);
    return j;
}

/// @brief Fail closed before measuring if baseline contracts or environment differ.
inline void compatible(const Json& baseline, const Json& current) {
    require(json_number_value(json_object_get(baseline.get(), "schema")) == 1
        && string(json_object_get(baseline.get(), "status")) == "complete"
        && string(json_object_get(baseline.get(), "evidence")) == "development-shared-process",
        "Baseline is incomplete or belongs to another measurement protocol");
    for (const char* key : {"environment", "workloads", "repeats", "preflight"})
        require(json_equal(json_object_get(baseline.get(), key), json_object_get(current.get(), key)),
            (std::string("Incompatible baseline: ")+key).c_str());
    if (json_object_get(baseline.get(), "execution_policy") || json_object_get(current.get(), "execution_policy"))
        require(json_equal(json_object_get(baseline.get(), "execution_policy"),
                           json_object_get(current.get(), "execution_policy")),
            "Incompatible baseline: execution policy");
}

/// @brief Detect missing/corrupt retained files before trusting a baseline table.
inline void check_artifacts(const Json& baseline, const std::string& directory) {
    if (auto* token = json_object_get(baseline.get(), "execution_guard_token")) {
        auto guard = read_json(directory+"/execution-guard.json");
        require(string(json_object_get(guard.get(), "status")) == "complete" &&
            string(json_object_get(guard.get(), "token")) == string(token),
            "Baseline execution guard is incomplete or belongs to another run");
    }
    auto* runs = json_object_get(baseline.get(), "runs");
    auto* workloads = json_object_get(baseline.get(), "workloads");
    auto* results = json_object_get(baseline.get(), "results");
    const double repeats = json_number_value(json_object_get(baseline.get(), "repeats"));
    require(repeats >= 1 && repeats <= 100 && repeats == std::floor(repeats)
        && json_is_object(workloads) && json_object_size(workloads)
        && json_is_object(results) && json_object_size(results) == json_object_size(workloads)
        && json_is_array(runs) && json_array_size(runs) == json_object_size(workloads)*repeats,
        "Incomplete baseline repetitions/results");
    std::set<std::pair<std::string, size_t>> seen;
    size_t index; json_t* row;
    json_array_foreach(runs, index, row) {
        const auto id = string(json_object_get(row, "workload"));
        const double repeat = json_number_value(json_object_get(row, "repeat"));
        require(json_object_get(workloads, id.c_str()) && json_object_get(results, id.c_str())
            && json_is_number(json_object_get(row, "repeat")) && repeat >= 0 && repeat < repeats
            && repeat == std::floor(repeat) && seen.insert({id, size_t(repeat)}).second,
            "Duplicate or invalid baseline repetition");
        std::vector<std::string> fields{"raw", "numerical_report"};
        if (json_object_get(baseline.get(), "execution_policy")) fields.push_back("execution");
        for (const auto& field : fields) {
            const auto file = string(json_object_get(row, field.c_str()));
            require(!file.empty() && file.find_first_of("/\\") == std::string::npos
                && file != "." && file != "..", "Invalid baseline artifact path");
            const auto hash = string(json_object_get(row, (std::string(field)+"_fnv1a64").c_str()));
            require(fingerprint(directory+"/"+file) == hash, "Baseline artifact fingerprint differs");
        }
    }
}

/// @brief Restore stream/sink state on success and exceptions; outside DSP timing.
struct Capture {
    std::streambuf* old;
    std::ostringstream diagnostics;
    explicit Capture(std::function<void(const std::vector<Row>&)> sink) {
        require(!result_sink(), "Nested result capture");
        result_sink() = std::move(sink); old = std::cerr.rdbuf(diagnostics.rdbuf());
    }
    ~Capture() { std::cerr.rdbuf(old); result_sink() = {}; }
};

inline void help() {
    std::cout << "C++ development benchmarks (run from repository root)\n"
        << "  make benchmark-fast [BENCHMARK_DEV_ARGS='--backend core-float']\n"
        << "  make benchmark-full\n"
        << "  paper --development --profile fast|full --output NEW_DIRECTORY [options]\n"
        << "Options: --list, --help, --verify (full preflight), --backend ID,\n"
        << "  --pass callback|throughput|complete|incremental, --n N, --hop H,\n"
        << "  --repeats R, --hops HOPS, --frames FRAMES, --warm-hops HOPS, --seed S,\n"
        << "  --config FROZEN_ARRAY.json, --baseline PREVIOUS_DIRECTORY, --label TEXT\n"
        << "Filters select existing workloads; they do not override settings.\n"
        << "Defaults: fast 3 repeats/32 hops/32 frames/8 warm; full 3/128/128/32.\n"
        << "Results are descriptive shared-process development evidence.\n"
        << "Use the publication archiver for independent-process/session confirmation.\n";
}

/// @brief Serial native campaign. No forced builds or child measurement processes.
template<typename Execute, typename Verify, typename FastVerify>
int run(int argc, char** argv, const std::string& executable,
        Execute execute, Verify verify, FastVerify fast_verify) {
    for (int i = 0; i < argc; ++i) if (std::string(argv[i]) == "--help") { help(); return 0; }
    const auto o = options(argc, argv);
    const auto configs = matrix(o);
    if (o.list) {
        for (const auto& c : configs) std::cout << identity(c) << '\n';
        if (o.profile == "fast")
            for (const auto& d : backend_registry)
                if (!d.available && (std::string(d.id) == "vdsp-analysis-float"
                    || std::string(d.id) == "vdsp-native-batch-float" || std::string(d.id) == "fftw-native-batch-float"))
                    std::cout << "Omitted " << d.id << ": " << d.reason << '\n';
        std::cout << configs.size() << " workloads, " << configs.size()*o.repeats << " repetitions\n";
        return 0;
    }
    require(!o.output.empty(), "Use --output with a new directory (or --list)");
    for (const auto& c : configs) {
        const auto requested = Execution::Policy::configured(c.pass);
        require(c.workload_schema < 3 || c.execution_regime == requested.regime, "Workload/execution regime mismatch");
    }
    const auto policy = Execution::Policy::configured(configs.front().pass);
    const auto settle_seconds = Execution::number("PAPER_SESSION_SETTLE_SECONDS", 180, 3600);
    require(settle_seconds >= 180, "Session settling must be at least 180 seconds");
    const auto guard_token = Execution::environment("PAPER_GUARD_TOKEN");
#if defined(__APPLE__) && !defined(PAPER_FIXTURE_CLOCK)
    require(!guard_token.empty(), "Use the protected development launcher on macOS");
#endif
    const auto began = Clock::now();
    auto manifest = object(), workloads = object(), results = object();
    auto sources = source_identity();
    set(manifest, "schema", 1); set(manifest, "status", "running");
    set(manifest, "evidence", "development-shared-process");
    set(manifest, "profile", o.profile); set(manifest, "label", o.label);
    const auto now = std::time(nullptr);
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    set(manifest, "started_utc", timestamp);
    auto invocation = own(json_array());
    for (int i = 0; i < argc; ++i) json_array_append_new(invocation.get(), json_string(argv[i]));
    set(manifest, "arguments", invocation);
    set(manifest, "preflight", o.verify || o.profile == "full" ? "full" : "focused");
    set(manifest, "repeats", o.repeats); set(manifest, "seed", o.seed);
    set(manifest, "revision", capture("git rev-parse HEAD"));
    set(manifest, "git_status", capture("git status --short"));
    set(manifest, "environment", environment()); set(manifest, "sources_fnv1a64", sources);
    auto execution_policy = object();
    set(execution_policy, "schema", 1); set(execution_policy, "regime", policy.regime);
    set(execution_policy, "process_settle_ms", policy.settle_ms);
    set(execution_policy, "throughput_chunks", policy.chunks);
    set(execution_policy, "session_settle_seconds", settle_seconds);
    set(execution_policy, "thread_policy", "inherit"); set(execution_policy, "fpu_policy", "inherit");
    set(execution_policy, "overrun", "catch-up-no-drop-no-rebase");
    set(execution_policy, "warmup", "after-process-settling");
    set(execution_policy, "conditioning", "outside-compute-inside-release-to-finish");
    set(manifest, "execution_policy", execution_policy);
    if (!guard_token.empty()) set(manifest, "execution_guard_token", guard_token);
    set(manifest, "executable_fnv1a64", fingerprint(executable));
    for (const auto& c : configs) set(workloads, identity(c).c_str(), config_json(c));
    set(manifest, "workloads", workloads);
    Json baseline;
    if (!o.baseline.empty()) {
        baseline = read_json(o.baseline+"/results.json"); compatible(baseline, manifest);
        check_artifacts(baseline, o.baseline);
    }
#ifdef _WIN32
    const int created = _mkdir(o.output.c_str());
#else
    const int created = mkdir(o.output.c_str(), 0755);
#endif
    require(created == 0, "Output directory must be new and its parent must exist");
    write_json(o.output+"/results.json", manifest);
    try {
        write_text(o.output+"/source.patch", capture("git diff HEAD --binary -- src benchmark mk Makefile docs/whitepaper/benchmarks test/paper"));
        std::cout << "Development " << o.profile << ": " << configs.size() << " workloads x "
            << o.repeats << " repetitions; preflight..." << std::endl;
        {
            // Retain preflight diagnostics independently from per-workload reports.
            std::ostringstream preflight;
            auto* previous = std::cout.rdbuf(preflight.rdbuf());
            try {
                if (o.verify || o.profile == "full") verify(); else fast_verify();
            } catch (...) {
                std::cout.rdbuf(previous);
                write_text(o.output+"/preflight.txt", preflight.str()+"Preflight failed.\n");
                throw;
            }
            std::cout.rdbuf(previous);
            write_text(o.output+"/preflight.txt", preflight.str()+"Preflight passed.\n");
        }
        const double preflight_seconds = elapsed(began, Clock::now())/1e9;
        std::vector<std::pair<size_t, size_t>> jobs;
        for (size_t r = 0; r < o.repeats; ++r)
            for (size_t i = 0; i < configs.size(); ++i) jobs.emplace_back(i, r);
        std::mt19937 random(static_cast<uint32_t>(o.seed));
        std::shuffle(jobs.begin(), jobs.end(), random);
        auto order = own(json_array());
        std::map<std::string, std::vector<Summary>> summaries;
        size_t ordinal = 0;
        std::cout << "Preflight complete; settling for " << settle_seconds << " seconds..." << std::endl;
        const auto interval = Execution::session_settle(settle_seconds);
        auto stabilization = object();
        set(stabilization, "seconds", settle_seconds);
        set(stabilization, "start_ns", own(json_integer(interval.first)));
        set(stabilization, "finish_ns", own(json_integer(interval.second)));
        set(manifest, "session_stabilization", stabilization);
        for (const auto& job : jobs) {
            const auto& c = configs[job.first];
            const auto id = identity(c);
            set(manifest, "active_workload", id); set(manifest, "active_repeat", job.second);
            const auto stem = "workload-"+std::to_string(job.first)+"-repeat-"+std::to_string(job.second);
            Summary summary; bool received = false;
            std::string diagnostics;
            {
                Execution::OutputPath execution_path(o.output+"/"+stem+".execution.json");
                Capture capture_rows([&](const std::vector<Row>& rows) {
                    require(!received, "Multiple result batches"); received = true;
                    summary = summarize(c, rows);
                    std::ofstream raw(o.output+"/"+stem+".csv");
                    print_csv(rows, raw); raw.close(); require(bool(raw), "Raw CSV write failed");
                });
                try { execute(c, false); }
                catch (...) {
                    write_text(o.output+"/"+stem+".stderr", capture_rows.diagnostics.str());
                    throw;
                }
                diagnostics = capture_rows.diagnostics.str();
            }
            require(received, "Backend emitted no measurements");
            write_text(o.output+"/"+stem+".stderr", diagnostics);
            summaries[id].push_back(summary);
            auto record = summary_json(summary);
            set(record, "workload", id); set(record, "repeat", job.second); set(record, "raw", stem+".csv");
            set(record, "numerical_report", stem+".stderr");
            set(record, "execution", stem+".execution.json");
            set(record, "execution_fnv1a64", fingerprint(o.output+"/"+stem+".execution.json"));
            set(record, "raw_fnv1a64", fingerprint(o.output+"/"+stem+".csv"));
            set(record, "numerical_report_fnv1a64", fingerprint(o.output+"/"+stem+".stderr"));
            json_array_append(order.get(), record.get());
            std::cout << '[' << ++ordinal << '/' << jobs.size() << "] " << c.backend
                << ' ' << c.pass << " N=" << c.n << " H=" << c.hop << " cost="
                << std::setprecision(6) << summary.cost
                << (c.pass == "callback" || c.pass == "throughput" ? " ns/sample" : " ns/transform") << '\n';
        }
        require(json_equal(sources.get(), source_identity().get()), "Sources changed during measurement");
        require(json_equal(json_object_get(manifest.get(), "environment"), environment().get()),
            "Build or dependencies changed during measurement");
        require(string(json_object_get(manifest.get(), "executable_fnv1a64")) == fingerprint(executable),
            "Executable changed during measurement");
        for (const auto& item : summaries) set(results, item.first.c_str(), aggregate(item.second));
        std::ofstream summary_csv(o.output+"/summary.csv");
        summary_csv << "workload,cost_unit,mean_cost,min_repeat_cost,max_repeat_cost,median_callback_p99_ns,observed_max_interval_ns\n";
        for (const auto& c : configs) {
            const auto id = identity(c);
            auto* result = json_object_get(results.get(), id.c_str());
            summary_csv << '"' << id << "\"," << (c.pass == "callback" || c.pass == "throughput"
                ? "ns/engine-sample" : "ns/transform") << std::setprecision(17);
            for (const char* key : {"mean_cost", "min_cost", "max_cost"})
                summary_csv << ',' << json_number_value(json_object_get(result, key));
            summary_csv << ',';
            if (c.pass == "callback") summary_csv << json_number_value(json_object_get(result, "median_p99_ns"));
            summary_csv << ',' << json_number_value(json_object_get(result, "observed_max_ns")) << '\n';
        }
        summary_csv.close(); require(bool(summary_csv), "Summary write failed");
        set(manifest, "results", results); set(manifest, "runs", order);
        set(manifest, "preflight_seconds", preflight_seconds);
        set(manifest, "wall_seconds", elapsed(began, Clock::now())/1e9);
        if (baseline) {
            std::ofstream comparison(o.output+"/comparison.csv");
            comparison << "workload,candidate_over_baseline_cost,candidate_over_baseline_callback_p99\n";
            for (const auto& c : configs) {
                const auto id = identity(c);
                const auto before = json_object_get(json_object_get(baseline.get(), "results"), id.c_str());
                const auto after = json_object_get(results.get(), id.c_str());
                const double cost = json_number_value(json_object_get(before, "mean_cost"));
                const double tail = json_number_value(json_object_get(before, "median_p99_ns"));
                require(cost > 0 && tail > 0, "Invalid baseline summary");
                const double ratio = json_number_value(json_object_get(after, "mean_cost"))/cost;
                comparison << '"' << id << "\"," << std::setprecision(17) << ratio << ',';
                if (c.pass == "callback")
                    comparison << json_number_value(json_object_get(after, "median_p99_ns"))/tail;
                comparison << '\n';
                std::cout << "ratio " << std::setprecision(4) << ratio << "  " << id << '\n';
            }
            comparison.close(); require(bool(comparison), "Comparison write failed");
        }
        json_object_del(manifest.get(), "active_workload"); json_object_del(manifest.get(), "active_repeat");
        set(manifest, "status", "complete"); write_json(o.output+"/results.json", manifest);
        std::cout << "Completed in " << elapsed(began, Clock::now())/1e9
            << " s. Raw data and descriptive results: " << o.output << '\n';
        return 0;
    } catch (const std::exception& error) {
        set(manifest, "status", "failed"); set(manifest, "error", std::string(error.what()));
        write_json(o.output+"/results.json", manifest);
        throw;
    }
}

}  // namespace Development
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_DEVELOPMENT_HPP_
