// Exercise the real streaming/sidecar path using only synthetic timestamps.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#define PAPER_FIXTURE_CLOCK
#include "../../benchmark/paper/protocol.hpp"

struct Driver {
    size_t samples = 0;
    bool fail;
    explicit Driver(const Paper::Config& c) : fail(c.load == 1) {}
    void process(float value) {
        Paper::require(std::isfinite(value), "Invalid input");
        Paper::require(!fail || samples < 128+17, "Fixture processing failure");
        ++samples;
    }
    void barrier() const {}
    void check() const { Paper::require(samples == 128+10*17, "Warmup or measured sample order changed"); }
};

// Publication members are compiled although the control has no replay.
// The real control follows the same static protocol.
struct FixtureDriver : Driver {
    explicit FixtureDriver(const Paper::Config& c) : Driver(c) {}
    bool published() const { return false; }
    size_t delay() const { return 0; }
};

int main(int argc, char** argv) {
    Paper::require(argc == 2, "Expected pass");
    Paper::Config c{};
    c.backend = "driver"; c.pass = argv[1]; c.alignment = "aligned"; c.state = "steady";
    const bool fail = c.pass == "failure";
    if (fail) { c.pass = "callback"; c.load = 1; }
    c.n = 128; c.hop = 32; c.block = 17; c.count = c.voices = 1; c.callbacks = 10; c.rate = 44100;
    const auto interval = Paper::Execution::session_settle(180);
    Paper::require(interval.second-interval.first == 180000000100LL, "Synthetic gate changed");
    try {
        Paper::Execution::Session session(c.pass);
        Paper::stream<FixtureDriver>(c);
        session.finish();
    } catch (const std::runtime_error& error) {
        if (!fail) throw;
        std::cerr << error.what() << '\n';
        return 1;
    }
    Paper::require(!fail, "Fixture did not fail");
}
