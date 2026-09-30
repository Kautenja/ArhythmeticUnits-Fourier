// Regression checks for development measurement units and comparison contracts.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <limits>
#include "catch_amalgamated.hpp"
#include "../../benchmark/paper/development.hpp"

using namespace Paper;
using namespace Paper::Development;

TEST_CASE("Development matrix keeps tuning and validation distinct", "[benchmark]") {
    Options o;
    const auto fast = matrix(o);
    bool vdsp = false;
    for (const auto& d : backend_registry)
        if (std::string(d.id) == "vdsp-analysis-float") vdsp = d.available;
    REQUIRE(fast.size() == (vdsp ? 40 : 36));
    for (const std::string backend : {"core-independent4-simd", "fourier", "spectre"})
        REQUIRE(std::any_of(fast.begin(), fast.end(), [&](const Config& c) { return c.backend == backend; }));
    for (const auto& c : fast) {
        REQUIRE(c.n == 2048);
        REQUIRE(c.hop == 1024);
        REQUIRE(c.callbacks*c.block == 32768);
    }
    o.profile = "full";
    const auto full = matrix(o);
    REQUIRE(full.size() > fast.size());
    REQUIRE(std::any_of(full.begin(), full.end(), [](const Config& c) { return c.n == 8192; }));
    REQUIRE(std::any_of(full.begin(), full.end(), [](const Config& c) { return c.hop == 509; }));
    o.backend = "core-float"; o.n = 8192; o.pass = "throughput";
    REQUIRE(matrix(o).size() == 2);
    o.n = 256;
    REQUIRE_THROWS(matrix(o));
}

TEST_CASE("Malformed workloads cannot hide behind defaults or filters", "[benchmark]") {
    Options o;
    Config c = base(); c.backend = "missing-provider";
    o.backend = "core-float";
    REQUIRE_THROWS(select({c}, o));
    c = base(); c.block = 0;
    REQUIRE_THROWS(select({c}, o));
    c = base(); c.hop = 0;
    REQUIRE_THROWS(select({c}, o));
    REQUIRE_THROWS(select({base(), base()}, Options{}));
    REQUIRE_THROWS(number("-1"));
    REQUIRE_THROWS(number("1x"));
    REQUIRE_THROWS(number(""));
    REQUIRE(number("20260929") == 20260929);
    auto malformed = own(json_loads("{\"smooth\":2}", 0, nullptr));
    REQUIRE_THROWS(config_from_json(malformed.get()));
    malformed = own(json_loads("{\"n\":2048.5}", 0, nullptr));
    REQUIRE_THROWS(config_from_json(malformed.get()));
    malformed = own(json_loads("{\"typo\":1}", 0, nullptr));
    REQUIRE_THROWS(config_from_json(malformed.get()));
}

TEST_CASE("CLI rejects ambiguous or invalid measurement options", "[benchmark]") {
    auto parse = [](std::vector<std::string> values) {
        std::vector<char*> args;
        for (auto& value : values) args.push_back(&value[0]);
        return options(int(args.size()), args.data());
    };
    REQUIRE_THROWS(parse({"--hops"}));
    REQUIRE_THROWS(parse({"--hops", "1"}));
    REQUIRE_THROWS(parse({"--frames", "0"}));
    REQUIRE_THROWS(parse({"--repeats", "0"}));
    REQUIRE_THROWS(parse({"--n", "0"}));
    REQUIRE_THROWS(parse({"--hop", "0"}));
    REQUIRE_THROWS(parse({"--profile", "confirmation"}));
    REQUIRE_THROWS(parse({"--profile", "fast", "--profile", "full"}));
    REQUIRE_THROWS(parse({"--typo", "32"}));
    REQUIRE(parse({"--profile", "full"}).hops == 128);
    REQUIRE(parse({"--hops", "4", "--profile", "full"}).hops == 4);
    REQUIRE(parse({"--seed", "20260929"}).seed == 20260929);
}

TEST_CASE("Explicit workload controls survive native JSON and identity boundaries", "[benchmark]") {
    Config c = base(); c.workload_schema = 3;
    c.fixture = "decay"; c.window = "blackman-harris"; c.temporal_mode = "module-seconds";
    c.temporal_value = 0.1; c.octave = 1.; c.fixture_seed = UINT32_MAX;
    const auto encoded = own(json_loads(workload_controls_json(c).c_str(), 0, nullptr));
    REQUIRE(identity(config_from_json(encoded.get())) == identity(c));
    Config retained = base();
    parse_workload_controls(retained, config_json(c).get(), false);
    REQUIRE(workload_controls_json(retained) == workload_controls_json(c));
    auto altered = own(json_deep_copy(encoded.get()));
    set(altered, "fixture_seed", own(json_integer(7)));
    REQUIRE(identity(config_from_json(altered.get())) != identity(c));
    for (const auto& field : control_fields()) {
        altered = own(json_deep_copy(encoded.get()));
        json_object_del(altered.get(), field.c_str());
        REQUIRE_THROWS(config_from_json(altered.get()));
    }
    altered = own(json_deep_copy(encoded.get()));
    set(altered, "workload_schema", own(json_integer(2)));
    REQUIRE_THROWS(config_from_json(altered.get()));
    altered = own(json_deep_copy(encoded.get()));
    set(altered, "fixture_seed", own(json_true()));
    REQUIRE_THROWS(config_from_json(altered.get()));
    c.callbacks = 512; c.temporal_mode = "alpha"; c.temporal_value = 1.;
    REQUIRE_THROWS(validate(c));
    c.temporal_value = .8;
    REQUIRE_NOTHROW(validate(c));
}

TEST_CASE("Callback cost normalizes by samples and preserves timer overhead", "[benchmark]") {
    Config c = base(); c.callbacks = 2; c.block = 64;
    const std::vector<Row> rows = {
        Row("timer", 0, 0, 0, 0, 10), Row("timer", 1, 0, 0, 0, 20),
        Row("callback", 0, 0, 0, 64, 640), Row("callback", 1, 0, 64, 64, 1280),
        Row("publication", 0, 0, 0, 0, 0)};
    const auto s = summarize(c, rows);
    REQUIRE(s.cost == 15);
    REQUIRE(s.observations == 2);
    REQUIRE(s.samples == 128);
    REQUIRE(s.p50 == 640);
    REQUIRE(s.p99 == 1280);
    REQUIRE(s.maximum == 1280);
    REQUIRE(s.timer_p99 == 20);
    c.callbacks = 3;
    REQUIRE_THROWS(summarize(c, rows));
    c.callbacks = 2;
    auto corrupt = rows;
    corrupt[2].ns = std::numeric_limits<double>::quiet_NaN();
    REQUIRE_THROWS(summarize(c, corrupt));
    REQUIRE_THROWS(summarize(c, {rows[0]}));
    REQUIRE_THROWS(summarize(c, {rows[2], rows[3]}));
}

TEST_CASE("Transform costs count transforms rather than implementation work units", "[benchmark]") {
    Config c = base(); c.pass = "complete"; c.callbacks = 2;
    auto s = summarize(c, {Row("timer", 0, 0, 0, 0, 20),
        Row("complete", 0, 0, 0, 10000, 100), Row("complete", 1, 0, 0, 10000, 300)});
    REQUIRE(s.cost == 200);
    s.cost = 400;
    Summary other = s; other.cost = 100;
    REQUIRE(json_number_value(json_object_get(aggregate({s, other}).get(), "mean_cost")) == 250);
}

TEST_CASE("Baseline comparison rejects changed contracts and incomplete evidence", "[benchmark]") {
    auto current = object();
    set(current, "schema", 1); set(current, "status", "complete");
    set(current, "evidence", "development-shared-process");
    set(current, "environment", object()); set(current, "preflight", "focused");
    set(current, "repeats", 3);
    auto workloads = object(); set(workloads, "fixture", config_json(matrix(Options{})[0]));
    set(current, "workloads", workloads);
    auto baseline = own(json_deep_copy(current.get()));
    REQUIRE_NOTHROW(compatible(baseline, current));
    set(baseline, "revision", "a different source revision is expected");
    REQUIRE_NOTHROW(compatible(baseline, current));
    set(baseline, "repeats", 2);
    REQUIRE_THROWS(compatible(baseline, current));
    baseline = own(json_deep_copy(current.get()));
    set(baseline, "status", "failed");
    REQUIRE_THROWS(compatible(baseline, current));
    baseline = own(json_deep_copy(current.get()));
    set(baseline, "environment", "different build");
    REQUIRE_THROWS(compatible(baseline, current));
    baseline = own(json_deep_copy(current.get()));
    set(baseline, "workloads", object());
    REQUIRE_THROWS(compatible(baseline, current));
    baseline = own(json_deep_copy(current.get()));
    auto policy = object(); set(policy, "regime", "continuous");
    set(current, "execution_policy", policy);
    REQUIRE_THROWS(compatible(baseline, current));
    set(baseline, "execution_policy", policy);
    REQUIRE_NOTHROW(compatible(baseline, current));
    auto changed_policy = object(); set(changed_policy, "regime", "paced");
    set(current, "execution_policy", changed_policy);
    REQUIRE_THROWS(compatible(baseline, current));
    REQUIRE_THROWS(check_artifacts(current, "/unused"));
}

TEST_CASE("Result sinks restore normal CLI output after a failed workload", "[benchmark]") {
    REQUIRE(!result_sink());
    const auto previous = std::cerr.rdbuf();
    try {
        Capture captured([](const std::vector<Row>&) { throw std::runtime_error("fixture failure"); });
        print({Row("timer", 0, 0, 0, 0, 1)});
        FAIL("Expected sink failure");
    } catch (const std::runtime_error&) {}
    REQUIRE(!result_sink());
    REQUIRE(std::cerr.rdbuf() == previous);
}
