// Actual Rack scheduling and all-output replay; measurement clocks are synthetic.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#define PAPER_FIXTURE_CLOCK
#include "../../benchmark/paper/engine_host.hpp"

template<typename Borrow>
void held_snapshot(Paper::EngineHost::Graph& graph, Borrow borrow) {
    using namespace Paper;
    std::atomic<int> stage{0}; std::exception_ptr failure;
    std::thread consumer([&]() {
        try {
            const auto& snapshot = borrow();
            const auto before = EngineHost::inspect(snapshot);
            stage.store(1, std::memory_order_release);
            while (stage.load(std::memory_order_acquire) != 2) std::this_thread::yield();
            const auto after = EngineHost::inspect(snapshot);
            require(before.sequence == after.sequence && before.checksum == after.checksum,
                "Producer/reset overwrote a consumer-owned snapshot");
        } catch (...) { failure = std::current_exception(); stage.store(1, std::memory_order_release); }
    });
    while (!stage.load(std::memory_order_acquire)) std::this_thread::yield();
    graph.step(3*graph.plan.c.hop); graph.event("reset"); graph.step(3*graph.plan.c.hop);
    stage.store(2, std::memory_order_release); consumer.join();
    if (failure) std::rethrow_exception(failure);
}

int main(int argc, char** argv) {
    using namespace Paper;
    try {
        if (argc == 3) return EngineHost::run(argv[2], std::string(argv[1]) == "verify");
        EngineHost::Plan p;
        auto& c = p.c;
        c.workload_schema = 3; c.backend = "fourier-default"; c.n = 2048; c.hop = 1440;
        c.window = "flattop"; c.temporal_mode = "module-seconds"; c.temporal_value = 0;
        c.active_ports = 4; c.fixture = "independent"; c.pass = "callback"; c.block = 256;
        c.state = "startup"; c.smooth = 0; c.count = 1; c.voices = 3; c.rate = 48000;
        p.blocks = 24; p.warm_hops = 2;
        for (const std::string native : {"", "vdsp-native4-batch-float", "vdsp-native4-hybrid-float"}) {
#ifndef PAPER_HAVE_VDSP
            if (!native.empty()) continue;
#endif
            p.native = native;
            for (size_t threads : {1, 4}) {
                p.threads = threads;
                for (size_t count : {1, 4, 16}) {
                    p.analyzers = count; c.alignment = count == 1 ? "aligned" : "staggered";
                    EngineHost::verify(p);
                }
            }
        }
        p.analyzers = 1; p.threads = 4; c.alignment = "aligned";
        c.backend = "fourier"; c.n = 16384; c.hop = 240; c.window = "blackman-harris";
        c.octave = 1./3; c.temporal_value = .1; c.active_ports = 2;
        for (const std::string native : {"", "vdsp-native4-batch-float", "vdsp-native4-hybrid-float"}) {
#ifndef PAPER_HAVE_VDSP
            if (!native.empty()) continue;
#endif
            p.native = native; EngineHost::verify(p);
        }
        c.n = 2048; c.hop = 1440; c.octave = 0; c.temporal_value = 0;
        p.blocks = 96;
        p.events = {{5,"freeze"},{12,"resume"},{20,"window"},{28,"band"},{36,"geometry"},{44,"reset"},{54,"sample-rate"}};
        for (const std::string native : {"", "vdsp-native4-batch-float", "vdsp-native4-hybrid-float"}) {
#ifndef PAPER_HAVE_VDSP
            if (!native.empty()) continue;
#endif
            p.native = native; EngineHost::verify(p);
        }
        c.backend = "spectre-default"; c.n = 2048; c.hop = 1024; c.active_ports = 1;
        c.window = "flattop"; c.fixture = "decay"; c.decay_samples = 4096;
        for (const std::string native : {"", "pffft-native-batch-float", "pffft-native-hybrid-float"}) {
            p.native = native; EngineHost::verify(p);
        }
        p.events.clear(); p.native.clear(); c.backend = "fourier"; c.hop = 1440; c.active_ports = 4;
        c.fixture = "independent";
        {
            EngineHost::Graph graph(p); graph.step(c.hop);
            auto& module = dynamic_cast<EngineHost::ProductionNode<SpectrumAnalyzer>&>(*graph.nodes[0]).host.module;
            held_snapshot(graph, [&]() -> const SpectrumAnalyzer::DisplaySpectrum& { return module.consume_display_spectrum(); });
        }
#ifdef PAPER_HAVE_VDSP
        p.native = "vdsp-native4-hybrid-float";
        {
            EngineHost::Graph graph(p); graph.step(c.hop);
            auto& module = dynamic_cast<EngineHost::NativeNode<NativeFourier<VdspNative<float, 4>>>&>(*graph.nodes[0]).adapter;
            held_snapshot(graph, [&]() -> const SpectrumAnalyzer::DisplaySpectrum& { return module.consume(); });
        }
#endif
        std::cout << "Rack engine/module replay passed without benchmark timing\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
