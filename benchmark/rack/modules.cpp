// Factory-created modules: operating states, presets, controls, and lifecycle.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include "../../src/SpectrumAnalyzer.cpp"
#include "../../src/Spectrogram.cpp"
#include "catch_amalgamated.hpp"
#include "../fixtures.hpp"

Plugin* plugin_instance = nullptr;

namespace {
using Json = std::unique_ptr<json_t, decltype(&json_decref)>;

/// @brief Provide the factory and JSON lookup environment without a Rack window.
struct RackContext {
    rack::Context context;
    rack::plugin::Plugin plugin;
    explicit RackContext(float rate = 48000.f) {
        Json manifest(json_load_file("plugin.json", 0, nullptr), json_decref);
        REQUIRE(manifest);
        plugin.slug = json_string_value(json_object_get(manifest.get(), "slug"));
        plugin.version = json_string_value(json_object_get(manifest.get(), "version"));
        plugin.addModel(modelSpectrumAnalyzer);
        plugin.addModel(modelSpectrogram);
        rack::plugin::plugins.push_back(&plugin);
        rack::contextSet(&context);
        context.engine = new rack::engine::Engine;
        context.engine->setSampleRate(rate);
    }
    ~RackContext() {
        rack::plugin::plugins.pop_back();
        rack::contextSet(nullptr);
    }
};

/// @brief Keep registration paired with removal even if a benchmark throws.
struct Registration {
    rack::engine::Module* module;
    explicit Registration(rack::engine::Module* value) : module(value) {
        APP->engine->addModule(module);
    }
    ~Registration() { APP->engine->removeModule(module); }
};

template<typename Module> rack::plugin::Model* model();
template<> rack::plugin::Model* model<SpectrumAnalyzer>() { return modelSpectrumAnalyzer; }
template<> rack::plugin::Model* model<Spectrogram>() { return modelSpectrogram; }

/// @brief Create through the exported model factory, including constructor reset.
template<typename Module>
std::unique_ptr<Module> create_module() {
    return std::unique_ptr<Module>(static_cast<Module*>(model<Module>()->createModule()));
}

size_t length(SpectrumAnalyzer& module) { return module.get_window_length(); }
size_t length(Spectrogram&) { return Spectrogram::N_FFT; }

/// @brief Deterministic port driver; synthesis/allocation stays outside timing.
/// @details Each voice/port gets a different phase. Dividing by voice count
/// keeps the polyphonic sum within the same voltage bound as mono input.
template<typename Module>
struct Driver {
    static constexpr size_t BLOCK = 4096;
    Module& module;
    int voices;
    std::array<std::vector<std::array<float, 16>>, Module::NUM_INPUTS> input;
    rack::engine::Module::ProcessArgs args = {};

    Driver(Module& value, int channels, bool silence = false) : module(value), voices(channels) {
        const auto signal = BenchmarkFixtures::signal<float>(BLOCK);
        args.sampleRate = module.get_sample_rate();
        args.sampleTime = 1.f / args.sampleRate;
        for (size_t port = 0; port < input.size(); ++port) {
            input[port].resize(BLOCK);
            module.inputs[Module::INPUT_SIGNAL + port].channels = voices;
            for (size_t i = 0; i < BLOCK; ++i)
                for (int voice = 0; voice < voices; ++voice)
                    input[port][i][voice] = silence ? 0.f :
                        5.f * signal[(i + 173*port + 29*voice)%BLOCK] / voices;
            // Disconnected ports retain nonzero storage, as Rack may do.
            if (!voices) module.inputs[Module::INPUT_SIGNAL + port].setVoltage(5.f);
        }
    }

    void run(size_t samples = BLOCK) {
        for (const auto& port : input) Catch::Benchmark::keep_memory(port.data());
        for (size_t i = 0; i < samples; ++i) {
            for (size_t port = 0; port < input.size(); ++port)
                for (int voice = 0; voice < voices; ++voice)
                    module.inputs[Module::INPUT_SIGNAL + port].setVoltage(input[port][i%BLOCK][voice], voice);
            module.process(args);
            ++args.frame;
        }
        Catch::Benchmark::deoptimize_value(module);
    }

    void warm() {
        // Fill the longest retained frame and settle filters/time smoothing.
        for (size_t i = 0; i < 32 + (length(module) + 2*module.get_hop_length())/BLOCK; ++i) run();
    }

    void freeze() {
        module.params[Module::PARAM_RUN].setValue(1.f);
        run(1);
        module.params[Module::PARAM_RUN].setValue(0.f);
        run(2*module.get_hop_length());
        Json state(module.dataToJson(), json_decref);
        REQUIRE(json_is_false(json_object_get(state.get(), "is_running")));
    }
};

/// @brief Sanity-check publication independently of timed processing.
void check_output(SpectrumAnalyzer& module, bool signal) {
    const auto& output = module.consume_display_spectrum();
    REQUIRE(output.count == length(module)/2+1);
    for (const auto& lane : output.points) {
        // Presets may change FFT length and move peaks to different bins.
        const auto peak = std::max_element(lane.begin(), lane.begin() + output.count,
            [](const Vec& a, const Vec& b) { return a.y < b.y; });
        if (signal) REQUIRE(std::isfinite(peak->y));
        else REQUIRE(peak->y < -10.f); // Default dB scale maps silence to -infinity.
    }
}
void check_output(Spectrogram& module, bool signal) {
    const size_t index = (module.get_hop_index() + Spectrogram::N_STFT - 1) % Spectrogram::N_STFT;
    const auto* output = module.consume_display_column(index, true);
    REQUIRE(output != nullptr);
    REQUIRE(output->revision > 0);
    const float peak = *std::max_element(output->values.begin(), output->values.end());
    if (signal) REQUIRE(peak > 0.f);
    else REQUIRE(peak == 0.f);
}

void check_reset(SpectrumAnalyzer& module) {
    REQUIRE(module.consume_display_spectrum().count == 0);
}
void check_reset(Spectrogram& module) {
    for (size_t i = 0; i < Spectrogram::N_STFT; ++i) {
        const auto* column = module.consume_display_column(i, true);
        REQUIRE(column != nullptr);
        REQUIRE(column->revision > 0);
        REQUIRE(column->values[7] == 0.f);
    }
}

/// @brief Load every shipped preset once; no filesystem work is timed.
std::vector<Json> presets(rack::plugin::Model* model) {
    auto paths = rack::system::getEntries("presets/" + model->slug);
    std::sort(paths.begin(), paths.end());
    std::vector<Json> result;
    for (const auto& path : paths) {
        if (rack::system::getExtension(path) != ".vcvm") continue;
        Json state(json_load_file(path.c_str(), JSON_REJECT_DUPLICATES, nullptr), json_decref);
        REQUIRE(state);
        // Suppress Rack's old-version log when measuring historical presets.
        json_object_set_new(state.get(), "version", json_string(model->plugin->version.c_str()));
        json_object_set_new(state.get(), "benchmark_name",
            json_string(rack::system::getFilename(path).c_str()));
        result.push_back(std::move(state));
    }
    REQUIRE(result.size() >= 2);
    return result;
}

/// @brief Change real panel settings; processing performs deferred rebuild work.
template<typename Module>
void live_controls(Module& module, bool alternate) {
    module.set_window_function(alternate ? Fourier::Window::Function::Hann
                                        : Fourier::Window::Function::Flattop);
    module.set_frequency_smoothing(alternate ? FrequencySmoothing::None : FrequencySmoothing::_1_3);
    module.set_time_smoothing(alternate ? 0.f : 0.1f);
    module.is_ac_coupled = alternate;
    for (int port = 0; port < Module::NUM_INPUTS; ++port)
        module.params[Module::PARAM_INPUT_GAIN + port].setValue(alternate ? 0.5f : 1.f);
}
void live_length(SpectrumAnalyzer& module, bool alternate) {
    module.set_window_length(alternate ? 128 : 16384);
    module.set_hop_length(alternate ? 240 : 1440);
}
void live_length(Spectrogram&, bool) {}

}  // namespace

TEMPLATE_TEST_CASE("Module operating states at host sample rates", "[modules][process]",
        SpectrumAnalyzer, Spectrogram) {
    RackContext context;
    for (const float rate : {44100.f, 96000.f, 192000.f}) {
        context.context.engine->setSampleRate(rate);
        for (const std::string mode : {"disconnected", "silence", "mono", "poly16", "frozen"}) {
            auto module = create_module<TestType>();
            Driver<TestType> driver(*module, mode == "disconnected" ? 0 : mode == "poly16" ? 16 : 1,
                mode == "silence");
            driver.warm();
            if (mode == "frozen") driver.freeze();
            const bool signal = mode != "disconnected" && mode != "silence";
            check_output(*module, signal);
            BENCHMARK(model<TestType>()->slug + " " + mode + " rate=" + std::to_string(int(rate))
                    + " N=" + std::to_string(length(*module))
                    + " H=" + std::to_string(module->get_hop_length()) + " / 4096 samples") { driver.run(); };
            check_output(*module, signal);
            Json state(module->dataToJson(), json_decref);
            REQUIRE(json_is_true(json_object_get(state.get(), "is_running")) == (mode != "frozen"));
        }
    }
}

TEMPLATE_TEST_CASE("Shipped presets through complete module processing", "[modules][presets]",
        SpectrumAnalyzer, Spectrogram) {
    RackContext context;
    const auto states = presets(model<TestType>());
    for (const auto& state : states) {
        auto module = create_module<TestType>();
        module->fromJson(state.get());
        Driver<TestType> driver(*module, 16);
        driver.warm();
        BENCHMARK(model<TestType>()->slug + " "
                + json_string_value(json_object_get(state.get(), "benchmark_name"))
                + " N=" + std::to_string(length(*module))
                + " H=" + std::to_string(module->get_hop_length()) + " / 4096 samples poly16 48kHz") {
            driver.run();
        };
        check_output(*module, true);
    }
}

TEMPLATE_TEST_CASE("Live module controls include deferred analysis rebuilds", "[modules][controls]",
        SpectrumAnalyzer, Spectrogram) {
    RackContext context;
    auto module = create_module<TestType>();
    Driver<TestType> driver(*module, 16);
    driver.warm();
    bool alternate = false;
    BENCHMARK(model<TestType>()->slug + " live settings+process / 32768 samples poly16 48kHz") {
        alternate = !alternate;
        live_controls(*module, alternate);
        live_length(*module, alternate);
        driver.run(32768);
    };
    REQUIRE(module->get_window_function() == (alternate ? Fourier::Window::Function::Hann
                                                       : Fourier::Window::Function::Flattop));
    check_output(*module, true);
}

TEMPLATE_TEST_CASE("Module factory construction and reset", "[modules][lifecycle]",
        SpectrumAnalyzer, Spectrogram) {
    RackContext context;
    for (const float rate : {44100.f, 192000.f}) {
        context.context.engine->setSampleRate(rate);
        const std::string label = model<TestType>()->slug + " rate=" + std::to_string(int(rate));
        BENCHMARK(label + " factory create+destroy / one module") {
            auto module = create_module<TestType>();
            Catch::Benchmark::deoptimize_value(*module);
        };
        auto module = create_module<TestType>();
        Driver<TestType> driver(*module, 1);
        driver.warm();
        module->params[TestType::PARAM_INPUT_GAIN].setValue(0.5f);
        BENCHMARK(label + " reset event / parameters and callback") {
            // Dispatch the modern host event, which resets parameters before
            // invoking the module's legacy no-argument override.
            static_cast<rack::engine::Module*>(module.get())->onReset(
                rack::engine::Module::ResetEvent{});
            Catch::Benchmark::deoptimize_value(*module);
        };
        Json state(module->dataToJson(), json_decref);
        REQUIRE(json_is_true(json_object_get(state.get(), "is_running")));
        REQUIRE(module->is_ac_coupled);
        REQUIRE(module->params[TestType::PARAM_INPUT_GAIN].getValue() ==
            module->getParamQuantity(TestType::PARAM_INPUT_GAIN)->getDefaultValue());
        check_reset(*module);
    }
}

TEMPLATE_TEST_CASE("Sample rate callbacks after maximum capacity preparation", "[modules][sample-rate]",
        SpectrumAnalyzer, Spectrogram) {
    RackContext context(192000.f);
    auto module = create_module<TestType>();
    // Registration makes Engine::setSampleRate dispatch the actual host event.
    Registration registered(module.get());
    BENCHMARK(model<TestType>()->slug + " host rate change 44100/192000 / one event, capacity warm") {
        const float rate = module->get_sample_rate() == 192000.f ? 44100.f : 192000.f;
        context.context.engine->setSampleRate(rate);
        Catch::Benchmark::deoptimize_value(*module);
    };
    const float rate = context.context.engine->getSampleRate();
    REQUIRE(module->get_sample_rate() == rate);
    REQUIRE(module->getParamQuantity(TestType::PARAM_HIGH_FREQUENCY)->maxValue == rate/2.f);
}

TEMPLATE_TEST_CASE("Complete module saved state", "[modules][serialization]",
        SpectrumAnalyzer, Spectrogram) {
    RackContext context;
    const auto states = presets(model<TestType>());
    auto module = create_module<TestType>();
    module->fromJson(states.front().get());
    BENCHMARK(model<TestType>()->slug + " toJson+free / params and custom state") {
        Json saved(module->toJson(), json_decref);
        Catch::Benchmark::deoptimize_value(saved.get());
    };
    size_t next = 0;
    BENCHMARK(model<TestType>()->slug + " fromJson / alternating preloaded presets") {
        module->fromJson(states[next].get());
        next = (next+1)%states.size();
        Catch::Benchmark::deoptimize_value(*module);
    };
    Json saved(module->toJson(), json_decref);
    REQUIRE(json_array_size(json_object_get(saved.get(), "params")) == module->params.size());
    auto restored = create_module<TestType>();
    restored->fromJson(saved.get());
    Json roundtrip(restored->toJson(), json_decref);
    REQUIRE(json_equal(saved.get(), roundtrip.get()));
}
