// Save/load regression checks for Fourier and Spectre's custom settings.
//
// Copyright 2026 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <memory>
#include "../../src/SpectrumAnalyzer.cpp"
#include "../../src/Spectrogram.cpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

Plugin* plugin_instance = nullptr;

namespace {

/// Supply the engine context required by the real module constructor.
struct RackContext {
    rack::Context context;

    RackContext() {
        rack::contextSet(&context);
        context.engine = new rack::engine::Engine;
        context.engine->setSampleRate(48000.f);
    }

    ~RackContext() {
        rack::contextSet(nullptr);
    }
};

/// Own Jansson values even when a regression assertion fails.
using Json = std::unique_ptr<json_t, decltype(&json_decref)>;

}  // namespace

TEST_CASE("Fourier saves and reloads each option independently of run state") {
    RackContext context;
    for (unsigned settings = 0; settings < 16; ++settings) {
        CAPTURE(settings);
        const bool running = settings & 1;
        const bool fill = settings & 2;
        const bool bezier = settings & 4;
        const bool ac_coupled = settings & 8;
        SpectrumAnalyzer source;
        Json run_state(json_object(), json_decref);
        json_object_set_new(run_state.get(), "is_running", json_boolean(running));
        source.dataFromJson(run_state.get());
        source.is_fill_enabled = fill;
        source.is_bezier_enabled = bezier;
        source.is_ac_coupled = ac_coupled;

        Json saved(source.dataToJson(), json_decref);
        const char* keys[] = {
            "is_running", "is_fill_enabled", "is_bezier_enabled", "is_ac_coupled"
        };
        const bool values[] = {running, fill, bezier, ac_coupled};
        REQUIRE(json_object_size(saved.get()) == 4);
        for (unsigned i = 0; i < 4; ++i) {
            CAPTURE(keys[i]);
            REQUIRE(json_is_boolean(json_object_get(saved.get(), keys[i])));
            CHECK(bool(json_boolean_value(json_object_get(saved.get(), keys[i]))) == values[i]);
        }

        SpectrumAnalyzer restored;
        // Opposite values ensure loading must restore every option.
        restored.is_fill_enabled = !fill;
        restored.is_bezier_enabled = !bezier;
        restored.is_ac_coupled = !ac_coupled;
        restored.dataFromJson(saved.get());
        CHECK(restored.is_fill_enabled == fill);
        CHECK(restored.is_bezier_enabled == bezier);
        CHECK(restored.is_ac_coupled == ac_coupled);
        Json resaved(restored.dataToJson(), json_decref);
        CHECK(json_equal(saved.get(), resaved.get()));
    }
}

TEST_CASE("Fourier retains defaults when loading missing custom settings") {
    RackContext context;
    SpectrumAnalyzer module;
    Json empty(json_object(), json_decref);
    module.dataFromJson(empty.get());
    CHECK_FALSE(module.is_fill_enabled);
    CHECK(module.is_bezier_enabled);
    CHECK(module.is_ac_coupled);
    Json saved(module.dataToJson(), json_decref);
    CHECK(json_is_true(json_object_get(saved.get(), "is_running")));
}

TEST_CASE("Spectre saves and reloads every supported color map") {
    RackContext context;
    Spectrogram module;
    for (int value = 0;
         value < static_cast<int>(Math::ColorMap::Function::NumFunctions);
         ++value) {
        CAPTURE(value);
        module.color_map = static_cast<Math::ColorMap::Function>(value);
        Json saved(module.dataToJson(), json_decref);
        REQUIRE(json_is_integer(json_object_get(saved.get(), "color_map")));
        CHECK(json_integer_value(json_object_get(saved.get(), "color_map")) == value);
        module.color_map = Math::ColorMap::Function::Magma;
        module.dataFromJson(saved.get());
        CHECK(static_cast<int>(module.color_map) == value);
        CHECK_NOTHROW(Math::ColorMap::color_map(module.color_map, 0.5f));
    }
}

TEST_CASE("Spectre defaults missing or invalid saved color maps to Magma") {
    RackContext context;
    Spectrogram module;
    const char* patches[] = {
        "{}",
        "{\"color_map\": -1}",
        "{\"color_map\": 4294967296}",
        "{\"color_map\": 9223372036854775807}",
        "{\"color_map\": -9223372036854775808}",
        "{\"color_map\": null}",
        "{\"color_map\": true}",
        "{\"color_map\": false}",
        "{\"color_map\": 1.0}",
        "{\"color_map\": 1.5}",
        "{\"color_map\": \"1\"}",
        "{\"color_map\": []}",
        "{\"color_map\": {}}"
    };
    for (const auto patch : patches) {
        CAPTURE(patch);
        Json saved(json_loads(patch, 0, nullptr), json_decref);
        REQUIRE(saved);
        module.color_map = Math::ColorMap::Function::Gray;
        module.dataFromJson(saved.get());
        CHECK(module.color_map == Math::ColorMap::Function::Magma);
        CHECK_NOTHROW(Math::ColorMap::color_map(module.color_map, 0.5f));
    }

    Json saved(json_object(), json_decref);
    json_object_set_new(saved.get(), "color_map",
        json_integer(static_cast<int>(Math::ColorMap::Function::NumFunctions)));
    module.color_map = Math::ColorMap::Function::Gray;
    module.dataFromJson(saved.get());
    CHECK(module.color_map == Math::ColorMap::Function::Magma);
    CHECK_NOTHROW(Math::ColorMap::color_map(module.color_map, 0.5f));
}
