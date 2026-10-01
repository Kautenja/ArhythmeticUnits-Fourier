// Save/load and undo regression checks for Fourier and Spectre settings.
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

#include "host.hpp"
#include <memory>
#include <set>
#include "../../src/SpectrumAnalyzer.cpp"
#include "../../src/Spectrogram.cpp"
#include "catch_amalgamated.hpp"

Plugin* plugin_instance = nullptr;

namespace {

/// Supply the engine context required by the real module constructor.
struct RackContext {
    rack::Context context;
    rack::plugin::Plugin plugin;

    RackContext() {
        rack::contextSet(&context);
        context.engine = new rack::engine::Engine;
        context.engine->setSampleRate(48000.f);
        context.history = new rack::history::State;
        plugin.slug = "ArhythmeticUnits-Fourier";
        plugin.version = "2.2.0";
        plugin.addModel(modelSpectrumAnalyzer);
        plugin.addModel(modelSpectrogram);
        rack::plugin::plugins.push_back(&plugin);
    }

    ~RackContext() {
        rack::plugin::plugins.pop_back();
        rack::contextSet(nullptr);
    }
};

/// Register a real module so Rack's history can resolve its persistent ID.
struct RegisteredModule {
    std::unique_ptr<rack::engine::Module> module;

    explicit RegisteredModule(rack::plugin::Model* model) : module(model->createModule()) {
        APP->engine->addModule(module.get());
    }

    ~RegisteredModule() {
        APP->engine->removeModule(module.get());
    }
};

/// Own Jansson values even when a regression assertion fails.
using Json = std::unique_ptr<json_t, decltype(&json_decref)>;

}  // namespace

TEST_CASE("Factory presets load complete state and share names across modules") {
    RackContext context;
    const float sample_rate = GENERATE(32000.f, 44100.f, 48000.f, 96000.f);
    context.context.engine->setSampleRate(sample_rate);
    std::set<std::string> names[2];
    const rack::plugin::Model* models[] = {modelSpectrumAnalyzer, modelSpectrogram};
    for (int model_index = 0; model_index < 2; ++model_index) {
        const auto model = models[model_index];
        for (const auto& path : rack::system::getEntries("presets/" + model->slug)) {
            CAPTURE(path, sample_rate);
            REQUIRE(rack::system::getExtension(path) == ".vcvm");
            const auto name = rack::system::getFilename(path);
            CHECK(name.find_first_of(" \t\n") == std::string::npos);
            names[model_index].insert(name);
            Json preset(json_load_file(path.c_str(), JSON_REJECT_DUPLICATES, nullptr), json_decref);
            REQUIRE(preset);
            std::unique_ptr<rack::engine::Module> module(
                model_index == 0 ? modelSpectrumAnalyzer->createModule() : modelSpectrogram->createModule());
            auto params = json_object_get(preset.get(), "params");
            REQUIRE(json_is_array(params));
            const bool legacy = model_index == 1 &&
                !json_object_get(json_object_get(preset.get(), "data"), "intensity_scale");
            REQUIRE(json_array_size(params) == module->params.size() - (model_index == 1 ? (legacy ? 4 : 2) : 0));
            std::set<int> ids;
            size_t index;
            json_t* param;
            json_array_foreach(params, index, param) {
                REQUIRE(json_is_integer(json_object_get(param, "id")));
                const int id = json_integer_value(json_object_get(param, "id"));
                REQUIRE(id >= 0);
                REQUIRE(id < static_cast<int>(module->params.size()));
                REQUIRE(ids.insert(id).second);
                auto value = json_object_get(param, "value");
                REQUIRE(json_is_number(value));
                REQUIRE(std::isfinite(json_number_value(value)));
                auto quantity = module->getParamQuantity(id);
                CHECK(json_number_value(value) >= quantity->minValue);
                // Full-band bounds may exceed Nyquist at low sample rates.
                const int high_id = model_index == 0
                    ? SpectrumAnalyzer::PARAM_HIGH_FREQUENCY : Spectrogram::PARAM_HIGH_FREQUENCY;
                const float maximum = id == high_id
                    ? std::max(20000.f, quantity->maxValue) : quantity->maxValue;
                CHECK(json_number_value(value) <= maximum);
                module->params[id].setValue(quantity->minValue);
            }
            Json dirty(module->dataToJson(), json_decref);
            const char* key;
            json_t* value;
            json_object_foreach(dirty.get(), key, value) {
                json_object_set_new(dirty.get(), key, json_is_boolean(value)
                    ? json_boolean(!json_boolean_value(value)) : json_integer(6));
            }
            module->dataFromJson(dirty.get());
            REQUIRE_NOTHROW(module->fromJson(preset.get()));
            json_array_foreach(params, index, param) {
                const int id = json_integer_value(json_object_get(param, "id"));
                const auto quantity = module->getParamQuantity(id);
                const float expected = rack::math::clamp(
                    json_number_value(json_object_get(param, "value")),
                    quantity->minValue, quantity->maxValue);
                CHECK(module->params[id].getValue() == Catch::Approx(expected));
            }
            Json restored(module->dataToJson(), json_decref);
            Json expected_data(json_deep_copy(json_object_get(preset.get(), "data")), json_decref);
            if (legacy) {
                json_object_set_new(expected_data.get(), "intensity_scale", json_string("legacy_linear"));
                CHECK(module->params[Spectrogram::PARAM_COLOR_FLOOR].getValue() == -90.f);
                CHECK(module->params[Spectrogram::PARAM_COLOR_CEILING].getValue() == 0.f);
            }
            CHECK(json_equal(restored.get(), expected_data.get()));
            CHECK(json_is_true(json_object_get(restored.get(), "is_running")));
        }
    }
    REQUIRE(names[0].size() == 5);
    REQUIRE(names[1].erase("DecibelInspection.vcvm") == 1);
    CHECK(names[0] == names[1]);
    for (const auto& name : names[0]) {
        CAPTURE(name);
        std::unique_ptr<rack::engine::Module> fourier(modelSpectrumAnalyzer->createModule());
        std::unique_ptr<rack::engine::Module> spectre(modelSpectrogram->createModule());
        Json first(json_load_file(("presets/SpectrumAnalyzer/" + name).c_str(), 0, nullptr), json_decref);
        Json second(json_load_file(("presets/Spectrogram/" + name).c_str(), 0, nullptr), json_decref);
        REQUIRE(first);
        REQUIRE(second);
        fourier->fromJson(first.get());
        spectre->fromJson(second.get());
        const int shared_ids[][2] = {
            {SpectrumAnalyzer::PARAM_INPUT_GAIN, Spectrogram::PARAM_INPUT_GAIN},
            {SpectrumAnalyzer::PARAM_WINDOW_FUNCTION, Spectrogram::PARAM_WINDOW_FUNCTION},
            {SpectrumAnalyzer::PARAM_FREQUENCY_SCALE, Spectrogram::PARAM_FREQUENCY_SCALE},
            {SpectrumAnalyzer::PARAM_TIME_SMOOTHING, Spectrogram::PARAM_TIME_SMOOTHING},
            {SpectrumAnalyzer::PARAM_FREQUENCY_SMOOTHING, Spectrogram::PARAM_FREQUENCY_SMOOTHING},
            {SpectrumAnalyzer::PARAM_LOW_FREQUENCY, Spectrogram::PARAM_LOW_FREQUENCY},
            {SpectrumAnalyzer::PARAM_HIGH_FREQUENCY, Spectrogram::PARAM_HIGH_FREQUENCY},
            {SpectrumAnalyzer::PARAM_SLOPE, Spectrogram::PARAM_SLOPE}
        };
        for (const auto& ids : shared_ids)
            CHECK(fourier->params[ids[0]].getValue() == spectre->params[ids[1]].getValue());
        for (int channel = 1; channel < SpectrumAnalyzer::NUM_CHANNELS; ++channel)
            CHECK(fourier->params[channel].getValue() == fourier->params[0].getValue());
        CHECK(json_equal(json_object_get(json_object_get(first.get(), "data"), "is_ac_coupled"),
            json_object_get(json_object_get(second.get(), "data"), "is_ac_coupled")));
    }
}

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
         value < static_cast<int>(Fourier::ColorMap::Function::NumFunctions);
         ++value) {
        CAPTURE(value);
        module.color_map = static_cast<Fourier::ColorMap::Function>(value);
        Json saved(module.dataToJson(), json_decref);
        REQUIRE(json_is_integer(json_object_get(saved.get(), "color_map")));
        CHECK(json_integer_value(json_object_get(saved.get(), "color_map")) == value);
        module.color_map = Fourier::ColorMap::Function::Magma;
        module.dataFromJson(saved.get());
        CHECK(static_cast<int>(module.color_map) == value);
        CHECK_NOTHROW(Fourier::ColorMap::color_map(module.color_map, 0.5f));
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
        module.color_map = Fourier::ColorMap::Function::Gray;
        module.dataFromJson(saved.get());
        CHECK(module.color_map == Fourier::ColorMap::Function::Magma);
        CHECK_NOTHROW(Fourier::ColorMap::color_map(module.color_map, 0.5f));
    }

    Json saved(json_object(), json_decref);
    json_object_set_new(saved.get(), "color_map",
        json_integer(static_cast<int>(Fourier::ColorMap::Function::NumFunctions)));
    module.color_map = Fourier::ColorMap::Function::Gray;
    module.dataFromJson(saved.get());
    CHECK(module.color_map == Fourier::ColorMap::Function::Magma);
    CHECK_NOTHROW(Fourier::ColorMap::color_map(module.color_map, 0.5f));
}

TEST_CASE("Menu setting changes participate in Rack undo and redo") {
    RackContext context;
    const int model_index = GENERATE(0, 1);
    const auto model = model_index == 0 ? modelSpectrumAnalyzer : modelSpectrogram;
    RegisteredModule registered(model);
    auto module = registered.module.get();
    // Freeze and use a non-default gain to catch unintended state changes.
    Json state(module->dataToJson(), json_decref);
    json_object_set_new(state.get(), "is_running", json_false());
    module->dataFromJson(state.get());
    module->params[0].setValue(0.75f);
    const char* fourier_keys[] = {"is_fill_enabled", "is_bezier_enabled", "is_ac_coupled"};
    const char* spectre_keys[] = {"is_ac_coupled", "color_map", "intensity_scale"};
    const auto keys = model == modelSpectrumAnalyzer ? fourier_keys : spectre_keys;
    const int count = 3;
    for (int i = 0; i < count; ++i) {
        CAPTURE(model->slug, keys[i]);
        Json before(APP->engine->moduleToJson(module), json_decref);
        auto old_value = json_object_get(json_object_get(before.get(), "data"), keys[i]);
        Json value(json_is_string(old_value) ? json_string("legacy_linear") : json_is_boolean(old_value)
            ? json_boolean(!json_boolean_value(old_value))
            : json_integer(static_cast<int>(Fourier::ColorMap::Function::Gray)), json_decref);
        const auto action_count = context.context.history->actions.size();
        Fourier::set_module_setting(module, "change test setting", keys[i], json_incref(value.get()));
        REQUIRE(context.context.history->actions.size() == action_count + 1);
        CHECK(context.context.history->getUndoName() == "change test setting");
        Json after(APP->engine->moduleToJson(module), json_decref);
        Json expected(json_deep_copy(before.get()), json_decref);
        json_object_set(json_object_get(expected.get(), "data"), keys[i], value.get());
        CHECK(json_equal(after.get(), expected.get()));

        context.context.history->undo();
        Json undone(APP->engine->moduleToJson(module), json_decref);
        CHECK(json_equal(undone.get(), before.get()));
        REQUIRE(context.context.history->canRedo());
        // Choosing the current value must preserve redo and avoid a new action.
        Fourier::set_module_setting(module, "no change", keys[i], json_incref(old_value));
        CHECK(context.context.history->actions.size() == action_count + 1);
        REQUIRE(context.context.history->canRedo());
        context.context.history->redo();
        Json redone(APP->engine->moduleToJson(module), json_decref);
        CHECK(json_equal(redone.get(), after.get()));
    }
    // History entries resolve IDs rather than retaining a raw module pointer.
    APP->engine->removeModule(module);
    CHECK_NOTHROW(context.context.history->undo());
    CHECK_NOTHROW(context.context.history->redo());
    APP->engine->addModule(module);
}

TEST_CASE("Spectre preserves numeric identities and defaults new state at full Rack load") {
    RackContext context;
    std::unique_ptr<Spectrogram> module(static_cast<Spectrogram*>(modelSpectrogram->createModule()));
    const int ids[] = {Spectrogram::PARAM_INPUT_GAIN, Spectrogram::PARAM_RUN,
        Spectrogram::PARAM_WINDOW_FUNCTION, Spectrogram::PARAM_FREQUENCY_SCALE,
        Spectrogram::PARAM_TIME_SMOOTHING, Spectrogram::PARAM_FREQUENCY_SMOOTHING,
        Spectrogram::PARAM_LOW_FREQUENCY, Spectrogram::PARAM_HIGH_FREQUENCY,
        Spectrogram::PARAM_SLOPE, Spectrogram::PARAM_COLOR_FLOOR, Spectrogram::PARAM_COLOR_CEILING,
        Spectrogram::PARAM_LINEAR_FLOOR, Spectrogram::PARAM_LINEAR_CEILING};
    for (int i = 0; i < 13; ++i) CHECK(ids[i] == i);
    CHECK(Spectrogram::INPUT_SIGNAL == 0);
    CHECK(Spectrogram::LIGHT_RUN == 0);
    CHECK(Spectrogram::NUM_OUTPUTS == 0);
    REQUIRE(module->intensity_scale == Spectrogram::Intensity::Scale::Decibels);
    Json saved(module->toJson(), json_decref);
    for (const auto malformed : {"null", "true", "3", "[]", "{}", "\"bad\""}) {
        CAPTURE(malformed);
        module->params[9].setValue(-10.f);
        module->params[10].setValue(24.f);
        module->intensity_scale = Spectrogram::Intensity::Scale::Decibels;
        Json patch(json_deep_copy(saved.get()), json_decref);
        json_object_set_new(json_object_get(patch.get(), "data"), "intensity_scale",
            json_loads(malformed, JSON_DECODE_ANY, nullptr));
        json_object_del(patch.get(), "params");
        module->fromJson(patch.get());
        CHECK(module->intensity_scale == Spectrogram::Intensity::Scale::Linear);
        CHECK(module->color_floor() == -90.f);
        CHECK(module->color_ceiling() == 0.f);
        // Also exercise malformed individual entries, not just absent arrays.
        json_object_set(patch.get(), "params", json_object_get(saved.get(), "params"));
        for (int id : {9, 10}) json_object_set_new(
            json_array_get(json_object_get(patch.get(), "params"), id), "value",
            json_loads(malformed, JSON_DECODE_ANY, nullptr));
        module->fromJson(patch.get());
        CHECK(module->color_floor() == Catch::Approx(std::string(malformed) == "3" ? 2.9f : -90.f));
        CHECK(module->color_ceiling() == (std::string(malformed) == "3" ? 3.f : 0.f));
    }
    json_object_del(saved.get(), "data");
    json_object_del(saved.get(), "params");
    module->params[9].setValue(-1.f);
    module->fromJson(saved.get());
    CHECK(module->intensity_scale == Spectrogram::Intensity::Scale::Linear);
    CHECK(module->color_floor() == -90.f);

    for (const auto mode : {Spectrogram::Intensity::Scale::Decibels,
                           Spectrogram::Intensity::Scale::Linear}) {
        module->intensity_scale = mode;
        module->params[9].setValue(-67.3f);
        module->params[10].setValue(12.4f);
        Json roundtrip(module->toJson(), json_decref);
        module->onReset();
        CHECK(module->intensity_scale == Spectrogram::Intensity::Scale::Decibels);
        module->fromJson(roundtrip.get());
        CHECK(module->intensity_scale == mode);
        CHECK(module->color_floor() == Catch::Approx(-67.3f));
        CHECK(module->color_ceiling() == Catch::Approx(12.4f));
    }
    for (const int id : {9, 10}) {
        CHECK_FALSE(module->getParamQuantity(id)->randomizeEnabled);
        CHECK_FALSE(module->getParamQuantity(id)->snapEnabled);
    }
}

TEST_CASE("Spectre sanitizes external endpoints and Rack reset restores display defaults") {
    RackContext context;
    RegisteredModule registered(modelSpectrogram);
    auto& module = *static_cast<Spectrogram*>(registered.module.get());
    for (const float value : {-1000.f, 1000.f, std::numeric_limits<float>::infinity(),
                             std::numeric_limits<float>::quiet_NaN()}) {
        for (int id : {9, 10}) {
            module.params[id].setValue(value);
            auto quantity = module.getParamQuantity(id);
            CHECK(Spectrogram::Intensity::finite(quantity->getValue()));
            CHECK(quantity->getValue() >= quantity->minValue);
            CHECK(quantity->getValue() <= quantity->maxValue);
            CHECK(quantity->getDisplayValueString().find("nan") == std::string::npos);
        }
    }
    module.intensity_scale = Spectrogram::Intensity::Scale::Linear;
    APP->engine->resetModule(&module);
    CHECK(module.color_floor() == -90.f);
    CHECK(module.color_ceiling() == 0.f);
    CHECK(module.intensity_scale == Spectrogram::Intensity::Scale::Decibels);
}

TEST_CASE("Vertical color range handles change independently with one undoable drag") {
    RackContext context;
    RegisteredModule registered(modelSpectrogram);
    auto& module = *static_cast<Spectrogram*>(registered.module.get());
    Json state(module.dataToJson(), json_decref);
    json_object_set_new(state.get(), "is_running", json_false());
    module.dataFromJson(state.get());
    for (const int id : {9, 10}) {
        SpectreIntensityHandle handle;
        handle.module = &module;
        handle.paramId = id;
        handle.step();
        CHECK(handle.visible);
        CHECK_FALSE(module.getParamQuantity(id)->smoothEnabled);
        const float before = module.getParamQuantity(id)->getValue();
        const int other = id == 9 ? 10 : 9;
        const float untouched = module.params[other].getValue();
        Widget::DragStartEvent start;
        start.button = GLFW_MOUSE_BUTTON_LEFT;
        handle.onDragStart(start);
        handle.drag_by(-5.f, 0);
        handle.drag_by(-5.f, 0);
        CHECK(module.params[id].getValue() == Catch::Approx(before + 144.f / Fourier::PanelLayout::intensity_bar_height() * 10.f));
        CHECK(module.params[other].getValue() == untouched);
        Widget::DragEndEvent end;
        end.button = GLFW_MOUSE_BUTTON_LEFT;
        const auto count = APP->history->actions.size();
        handle.onDragEnd(end);
        CHECK(APP->history->actions.size() == count + 1);
        const float after = module.params[id].getValue();
        APP->history->undo();
        CHECK(module.params[id].getValue() == before);
        APP->history->redo();
        CHECK(module.params[id].getValue() == after);
        handle.onDragStart(start);
        handle.drag_by(-5.f, RACK_MOD_CTRL);
        CHECK(module.params[id].getValue() == Catch::Approx(after + 144.f / Fourier::PanelLayout::intensity_bar_height() * 0.5f));
        handle.onDragEnd(end);
        // Typed values and Rack's native parameter reset use the same quantity.
        module.getParamQuantity(id)->setDisplayValueString(id == 9 ? "-72.5" : "12.5");
        CHECK(module.params[id].getValue() == (id == 9 ? -72.5f : 12.5f));
        handle.resetAction();
        CHECK(module.params[id].getValue() == (id == 9 ? -90.f : 0.f));
        APP->history->undo();
        CHECK(module.params[id].getValue() == (id == 9 ? -72.5f : 12.5f));
        APP->history->redo();
        CHECK(module.params[id].getValue() == (id == 9 ? -90.f : 0.f));
    }
    Json after(module.dataToJson(), json_decref);
    CHECK(json_is_false(json_object_get(after.get(), "is_running")));
}

TEST_CASE("Panel palette and scale choices are undoable and preserve independent ranges") {
    RackContext context;
    RegisteredModule registered(modelSpectrogram);
    auto& module = *static_cast<Spectrogram*>(registered.module.get());
    SpectreIntensityLegend control(&module);
    module.getParamQuantity(10)->setDisplayValueString("-20");
    CHECK(module.color_ceiling() == -20.f);
    control.select_scale(1);
    CHECK(control.linear());
    SpectreIntensityHandle handle;
    handle.module = &module;
    handle.paramId = Spectrogram::PARAM_LINEAR_CEILING;
    handle.step();
    CHECK(handle.visible);
    CHECK_FALSE(module.getParamQuantity(handle.paramId)->smoothEnabled);
    handle.drag_by(Fourier::PanelLayout::intensity_bar_height() / 4.f, 0);
    CHECK(module.linear_ceiling() == Catch::Approx(0.5f));
    module.getParamQuantity(11)->setDisplayValueString("10");
    CHECK(module.linear_floor() == Catch::Approx(0.1f));
    control.select_palette(static_cast<size_t>(Fourier::ColorMap::Function::Gray));
    CHECK(module.color_map == Fourier::ColorMap::Function::Gray);
    APP->history->undo();
    CHECK(module.color_map == Fourier::ColorMap::Function::Magma);
    APP->history->redo();
    CHECK(module.color_map == Fourier::ColorMap::Function::Gray);
    control.select_scale(0);
    CHECK_FALSE(control.linear());
    CHECK(module.color_ceiling() == -20.f);
    CHECK(module.linear_floor() == Catch::Approx(0.1f));
    CHECK(module.linear_ceiling() == Catch::Approx(0.5f));
    handle.step();
    CHECK_FALSE(handle.visible);
    const auto count = APP->history->actions.size();
    control.select_scale(0);
    CHECK(APP->history->actions.size() == count);
    APP->history->undo();
    CHECK(control.linear());
    APP->history->redo();
    CHECK_FALSE(control.linear());
    SpectreIntensityLegend preview(nullptr);
    preview.select_scale(1);
    preview.select_palette(0);
    CHECK(APP->history->actions.size() == count);
}

TEST_CASE("Both intensity ranges allow quiet ceilings and prevent crossed limits") {
    RackContext context;
    RegisteredModule registered(modelSpectrogram);
    auto& module = *static_cast<Spectrogram*>(registered.module.get());
    for (const bool linear : {false, true}) {
        const int floor = linear ? 11 : 9, ceiling = floor + 1;
        const float gap = linear ? 0.001f : 0.1f;
        module.intensity_scale = linear ? Spectrogram::Intensity::Scale::Linear : Spectrogram::Intensity::Scale::Decibels;
        SpectreIntensityHandle handle;
        handle.module = &module;
        handle.paramId = ceiling;
        handle.drag_by(Fourier::PanelLayout::intensity_bar_height() * (linear ? 0.25f : 30.f / 144.f), 0);
        CHECK(module.params[ceiling].getValue() == Catch::Approx(linear ? 0.5f : -30.f).margin(0.00001));
        module.getParamQuantity(floor)->setValue(100.f);
        CHECK(module.params[floor].getValue() == Catch::Approx(module.params[ceiling].getValue() - gap));
        module.getParamQuantity(ceiling)->setDisplayValueString("-1000");
        CHECK(module.params[ceiling].getValue() > module.params[floor].getValue());
        Json saved(module.toJson(), json_decref);
        // Full load handles reversed arrays and repairs invalid ranges deterministically.
        auto params = json_object_get(saved.get(), "params");
        json_object_set_new(json_array_get(params, floor), "value", json_real(linear ? 1.5 : 20.));
        json_object_set_new(json_array_get(params, ceiling), "value", json_real(linear ? 0.2 : -20.));
        module.fromJson(saved.get());
        CHECK(module.params[floor].getValue() == Catch::Approx((linear ? 0.2f : -20.f) - gap));
        Json normal(module.toJson(), json_decref);
        Json reversed(json_array(), json_decref);
        for (int i = json_array_size(params) - 1; i >= 0; --i)
            json_array_append(reversed.get(), json_array_get(params, i));
        json_object_set(saved.get(), "params", reversed.get());
        module.fromJson(saved.get());
        Json restored(module.toJson(), json_decref);
        CHECK(json_equal(normal.get(), restored.get()));
    }
}

TEST_CASE("Old presets and module reset discard stale ranges in both modes") {
    RackContext context;
    RegisteredModule registered(modelSpectrogram);
    auto& module = *static_cast<Spectrogram*>(registered.module.get());
    module.params[9].setValue(-115.f);
    module.params[10].setValue(-110.f);
    module.params[11].setValue(1.5f);
    module.params[12].setValue(1.6f);
    APP->engine->resetModule(&module);
    CHECK(module.color_floor() == -90.f);
    CHECK(module.color_ceiling() == 0.f);
    CHECK(module.linear_floor() == 0.f);
    CHECK(module.linear_ceiling() == 1.f);
    module.params[11].setValue(0.4f);
    module.params[12].setValue(0.5f);
    Json preset(json_load_file("presets/Spectrogram/Harmonics.vcvm", 0, nullptr), json_decref);
    REQUIRE(preset);
    module.fromJson(preset.get());
    CHECK(module.intensity_scale == Spectrogram::Intensity::Scale::Linear);
    CHECK(module.linear_floor() == 0.f);
    CHECK(module.linear_ceiling() == 1.f);
}
