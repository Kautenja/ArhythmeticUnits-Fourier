// Actual scalar/SIMD module processing without rendering or an audio device.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "../../src/SpectrumAnalyzer.cpp"
#include "../../src/Spectrogram.cpp"
#include "catch.hpp"
#include "../fixtures.hpp"

Plugin* plugin_instance = nullptr;

namespace {

/// @brief Own a headless Rack engine and restore the thread's context on exit.
struct RackContext {
    rack::Context context;
    RackContext() {
        rack::contextSet(&context);
        context.engine = new rack::engine::Engine;
        context.engine->setSampleRate(48000.f);
    }
    ~RackContext() { rack::contextSet(nullptr); }
};

/// @brief Time 4096 samples including port writes, filtering and publication.
/// @details Input synthesis, module construction, and warmup are not timed.
/// No consumer runs in the measured loop: producer mailbox exchanges remain
/// included, while display consumption/rendering and host scheduling do not.
template<typename Module>
void benchmark_module(Module& module, const std::string& name, size_t length) {
    const size_t block = 4096;
    for (const int voices : {1, 16}) {
        for (const bool smooth : {false, true}) {
            module.set_window_function(Fourier::Window::Function::Hann);
            module.set_frequency_smoothing(smooth ? FrequencySmoothing::_1_3
                                                 : FrequencySmoothing::None);
            module.set_time_smoothing(smooth ? 0.1f : 0.f);
            module.is_ac_coupled = true;
            std::array<std::vector<float>, Module::NUM_INPUTS> input;
            const auto signal = BenchmarkFixtures::signal<float>(block);
            for (size_t port = 0; port < input.size(); ++port) {
                input[port].resize(block);
                for (size_t i = 0; i < block; ++i)
                    input[port][i] = 5.f * signal[(i + 173*port)%block] / voices;
                module.inputs[Module::INPUT_SIGNAL + port].channels = voices;
            }
            rack::engine::Module::ProcessArgs args = {};
            args.sampleRate = module.get_sample_rate();
            args.sampleTime = 1.f / args.sampleRate;
            auto process = [&]() {
                for (auto& port : input) Catch::Benchmark::keep_memory(port.data());
                for (size_t i = 0; i < block; ++i) {
                    for (size_t port = 0; port < input.size(); ++port)
                        for (int voice = 0; voice < voices; ++voice)
                            module.inputs[Module::INPUT_SIGNAL + port].setVoltage(input[port][i], voice);
                    module.process(args);
                }
                Catch::Benchmark::deoptimize_value(module);
            };
            for (size_t i = 0; i < length/block + 32; ++i) process();
            BENCHMARK(name + " N=" + std::to_string(length)
                    + " H=" + std::to_string(module.get_hop_length())
                    + " ports=" + std::to_string(input.size())
                    + " voices=" + std::to_string(voices)
                    + (smooth ? " smooth=1/3+100ms" : " smooth=off")
                    + " / 4096 samples at 48kHz") { process(); };
        }
    }
}

}  // namespace

TEST_CASE("Fourier SIMD module engine blocks", "[rack][fourier]") {
    RackContext context;
    for (const size_t n : {128u, 2048u, 16384u}) {
        SpectrumAnalyzer module;
        module.set_window_length(n);
        module.set_hop_length(256);
        benchmark_module(module, "Fourier", n);
        const auto& spectrum = module.consume_display_spectrum();
        REQUIRE(spectrum.count == n/2+1);
        REQUIRE(std::isfinite(spectrum.points[0][7].y));
    }
}

TEST_CASE("Spectre scalar module engine blocks", "[rack][spectre]") {
    RackContext context;
    Spectrogram module;
    benchmark_module(module, "Spectre", Spectrogram::N_FFT);
    const size_t index = (module.get_hop_index() + Spectrogram::N_STFT - 1) % Spectrogram::N_STFT;
    const auto* column = module.consume_display_column(index);
    REQUIRE(column != nullptr);
    REQUIRE(column->revision > 0);
    REQUIRE(column->values[7] > 0.f);
}
