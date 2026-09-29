// Fixed development workloads and descriptive statistics; no timing policy here.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_DEVELOPMENT_MATRIX_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_DEVELOPMENT_MATRIX_HPP_

#include <map>
#include <set>
#include <sstream>
#include "protocol.hpp"

namespace Paper {
namespace Development {

/// @brief Explicit run policy; repetitions share a process, never a session claim.
struct Options {
    std::string profile = "fast", backend, pass, output, baseline, config, label;
    size_t n = 0, hop = 0, repeats = 3, hops = 32, frames = 32, warm = 8, seed = 20260929;
    bool list = false, verify = false;
};

inline size_t number(const std::string& text) {
    require(!text.empty() && text.find_first_not_of("0123456789") == std::string::npos,
        "Expected an unsigned decimal integer");
    const auto value = std::stoull(text);
    require(value <= 1000000000, "Integer option exceeds 1000000000");
    return size_t(value);
}

inline Options options(int argc, char** argv) {
    Options o;
    bool hops = false, frames = false, warm = false;
    std::set<std::string> seen;
    for (int i = 0; i < argc; ++i) {
        const std::string key(argv[i]);
        require(seen.insert(key).second, "Duplicate option");
        if (key == "--list") { o.list = true; continue; }
        if (key == "--verify") { o.verify = true; continue; }
        require(i+1 < argc, "Option requires a value");
        const std::string value(argv[++i]);
        if (key == "--profile") o.profile = value;
        else if (key == "--backend") o.backend = value;
        else if (key == "--pass") o.pass = value;
        else if (key == "--output") o.output = value;
        else if (key == "--baseline") o.baseline = value;
        else if (key == "--config") o.config = value;
        else if (key == "--label") o.label = value;
        else if (key == "--n") { o.n = number(value); require(o.n >= 128, "Invalid FFT size filter"); }
        else if (key == "--hop") { o.hop = number(value); require(o.hop > 0, "Invalid hop filter"); }
        else if (key == "--repeats") o.repeats = number(value);
        else if (key == "--hops") { o.hops = number(value); hops = true; }
        else if (key == "--frames") { o.frames = number(value); frames = true; }
        else if (key == "--warm-hops") { o.warm = number(value); warm = true; }
        else if (key == "--seed") o.seed = number(value);
        else throw std::runtime_error("Unknown development option: " + key);
    }
    require(o.profile == "fast" || o.profile == "full", "Profile must be fast or full");
    if (o.profile == "full") {
        if (!hops) o.hops = 128;
        if (!frames) o.frames = 128;
        if (!warm) o.warm = 32;
    }
    require(o.repeats >= 1 && o.repeats <= 100 && o.hops >= 2 && o.hops <= 65536
        && o.frames >= 1 && o.warm <= 4096, "Invalid repetition/observation/warmup count");
    return o;
}

inline Config base() {
    Config c{};
    c.backend = "core-float"; c.pass = "callback"; c.alignment = "aligned";
    c.state = "steady"; c.n = 2048; c.hop = 1024; c.block = 64;
    c.count = 1; c.voices = 1; c.rate = 48000;
    return c;
}

/// @brief Stable identity includes all work and observation-window settings.
inline std::string identity(const Config& c) {
    std::ostringstream out;
    out << c.backend << '/' << c.pass << "/N=" << c.n << "/H=" << c.hop
        << "/B=" << c.block << "/count=" << c.count << '/' << c.alignment
        << "/load=" << c.load << "/smooth=" << c.smooth << "/voices=" << c.voices
        << "/rate=" << c.rate << '/' << c.state << "/cache=" << c.cache_mib
        << "/offset=" << c.callback_offset << "/observations=" << c.callbacks
        << "/warm=" << c.warm_hops;
    return out.str();
}

/// @brief Reject malformed arrays before arithmetic, allocation or backend lookup.
inline void validate(const Config& c) {
    require(c.n >= 128 && c.n <= 16384 && !(c.n&(c.n-1))
        && c.hop >= 1 && c.hop <= 65536 && c.block >= 1 && c.block <= 65536
        && c.count >= 1 && c.count <= 64 && c.load <= 4096
        && c.voices >= 1 && c.voices <= 16 && c.rate >= 8000 && c.rate <= 192000
        && c.cache_mib <= 256 && c.callbacks >= 1 && c.callbacks <= 1000000
        && c.warm_hops <= 4096, "Workload outside protocol bounds");
    require(c.alignment == "aligned" || c.alignment == "staggered", "Invalid alignment");
    require(c.state == "steady" || c.state == "startup" || c.state == "live", "Invalid state");
    require(c.pass != "phases" && c.pass != "steps",
        "Use the publication runner for phase/step observations; development reports total cost");
    validate_backend(c);
}

/// @brief Fast is a fixed tuning set; full also exercises unseen sizes and hops.
inline std::vector<Config> builtin(const Options& o) {
    std::vector<Config> rows;
    const std::vector<std::string> primary = {"core-float", "legacy-batch-float",
        "legacy-incremental-float", "pffft-analysis-float",
        "pffft-scheduled-batch-float", "pffft-hybrid-float"};
    const std::vector<size_t> sizes = o.profile == "fast" ? std::vector<size_t>{2048}
        : std::vector<size_t>{128, 512, 2048, 8192, 16384};
    for (size_t n : sizes) for (bool smooth : {false, true}) {
        for (const auto& backend : primary) for (const std::string pass : {"callback", "throughput"}) {
            Config c = base(); c.n = n; c.smooth = smooth; c.backend = backend; c.pass = pass;
            rows.push_back(c);
        }
    }
    if (o.profile == "full") {
        for (const auto& backend : primary) for (const std::string pass : {"callback", "throughput"}) {
            for (size_t hop : {37u, 257u, 509u}) {
                Config c = base(); c.backend = backend; c.pass = pass; c.hop = hop;
                c.state = "live"; c.smooth = true; rows.push_back(c);
            }
            Config c = base(); c.backend = backend; c.pass = pass;
            c.state = "startup"; rows.push_back(c);
            c.state = "steady"; c.count = 4; c.alignment = "staggered";
            c.block = 16; c.load = 64; rows.push_back(c);
        }
        // Exercise every compiled transform and the independent channel/module paths.
        for (const auto& d : backend_registry) {
            if (!d.available) continue;
            const std::string boundary(d.boundary), kind(d.kind);
            if (boundary != "transform" && boundary != "inverse-job" && boundary != "chain"
                    && kind != "analysis4" && kind != "core4" && boundary != "module"
                    && std::string(d.id) != "core-double") continue;
            Config c = base(); c.backend = d.id;
            if (boundary == "transform") c.pass = "complete";
            rows.push_back(c);
        }
        for (const std::string provider : {"fftw", "vdsp"}) {
            const std::string name = provider + "-analysis-float";
            for (const auto& d : backend_registry) if (name == d.id && d.available) {
                for (const std::string pass : {"callback", "throughput"}) {
                    Config c = base(); c.backend = name; c.pass = pass; rows.push_back(c);
                }
            }
        }
    }
    return rows;
}

inline std::vector<Config> select(std::vector<Config> rows, const Options& o) {
    std::vector<Config> selected;
    std::set<std::string> identities;
    for (auto c : rows) {
        // Validate every requested workload, even if a filter would hide it.
        backend_descriptor(c.backend);
        require(c.block && c.hop <= 65536, "Invalid block/hop");
        c.warm_hops = o.warm;
        c.callbacks = c.pass == "callback" || c.pass == "throughput"
            ? (o.hops*c.hop+c.block-1)/c.block : c.pass == "steps" ? 2 : o.frames;
        validate(c);
        require(identities.insert(identity(c)).second, "Duplicate workload");
        if ((!o.backend.empty() && c.backend != o.backend) || (!o.pass.empty() && c.pass != o.pass)
                || (o.n && c.n != o.n) || (o.hop && c.hop != o.hop)) continue;
        selected.push_back(c);
    }
    require(!selected.empty(), "Filters matched no workloads; use --list to inspect the matrix");
    return selected;
}

/// @brief Nearest-rank quantiles describe this observation window only.
inline double quantile(std::vector<double> values, double fraction) {
    require(!values.empty() && fraction > 0 && fraction <= 1, "Invalid quantile");
    std::sort(values.begin(), values.end());
    return values[size_t(std::ceil(values.size()*fraction))-1];
}

struct Summary {
    size_t observations = 0, samples = 0, exceedances = 0;
    double cost = 0, p50 = 0, p95 = 0, p99 = 0, maximum = 0, timer_p99 = 0;
};

/// @brief Cost is ns/engine sample for streams, ns/transform for total passes.
inline Summary summarize(const Config& c, const std::vector<Row>& rows) {
    std::vector<double> values, timer;
    Summary s;
    double total = 0;
    for (const auto& row : rows) {
        require(std::isfinite(row.ns) && row.ns >= 0, "Invalid timing observation");
        if (row.kind == "timer") timer.push_back(row.ns);
        if (row.kind != c.pass) continue;
        values.push_back(row.ns); total += row.ns; s.samples += row.samples;
        if (c.pass == "callback" && row.ns > 1e9*c.block/c.rate) ++s.exceedances;
    }
    require(!values.empty() && !timer.empty(), "Missing workload or timer observations");
    require(values.size() == (c.pass == "throughput" ? 1 : c.callbacks), "Missing timing intervals");
    const bool streaming = c.pass == "callback" || c.pass == "throughput";
    require(!streaming || s.samples == c.callbacks*c.block, "Measured sample window differs");
    s.observations = values.size();
    s.cost = total/(streaming ? s.samples : values.size());
    s.p50 = quantile(values, .5); s.p95 = quantile(values, .95); s.p99 = quantile(values, .99);
    s.maximum = *std::max_element(values.begin(), values.end()); s.timer_p99 = quantile(timer, .99);
    return s;
}

}  // namespace Development
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_DEVELOPMENT_MATRIX_HPP_
