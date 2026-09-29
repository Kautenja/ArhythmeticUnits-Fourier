// Save/load regression checks for Fourier's custom module settings.
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
