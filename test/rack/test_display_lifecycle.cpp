// Spectre texture ownership checks using a headless NanoVG renderer.
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

#include <cmath>
#include <map>
#include <memory>
#include "../../src/Spectrogram.cpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

Plugin* plugin_instance = nullptr;

namespace {

/// Real NanoVG context with an instrumented texture backend instead of OpenGL.
struct TestRenderer {
    struct Texture { int width; int height; int type; };
    std::map<int, Texture> textures;
    int next_id = 1;
    int created = 0;
    int updated = 0;
    int deleted = 0;
    int invalid_accesses = 0;
    bool fail_creation = false;
    NVGcontext* vg = nullptr;

    TestRenderer() {
        NVGparams params = {};
        params.userPtr = this;
        params.renderCreate = [](void*, void*) { return 1; };
        params.renderCreateTexture = [](void* ptr, int type, int w, int h,
                                        int, const unsigned char*) {
            auto& self = *static_cast<TestRenderer*>(ptr);
            if (type == NVG_TEXTURE_RGBA && self.fail_creation) return 0;
            const int id = self.next_id++;
            self.textures.emplace(id, Texture{w, h, type});
            if (type == NVG_TEXTURE_RGBA) ++self.created;
            return id;
        };
        params.renderDeleteTexture = [](void* ptr, int id) {
            auto& self = *static_cast<TestRenderer*>(ptr);
            auto texture = self.textures.find(id);
            if (texture == self.textures.end()) { ++self.invalid_accesses; return 0; }
            if (texture->second.type == NVG_TEXTURE_RGBA) ++self.deleted;
            self.textures.erase(texture);
            return 1;
        };
        params.renderUpdateTexture = [](void* ptr, int id, int, int, int, int,
                                        const unsigned char*) {
            auto& self = *static_cast<TestRenderer*>(ptr);
            if (!self.textures.count(id)) { ++self.invalid_accesses; return 0; }
            ++self.updated;
            return 1;
        };
        params.renderGetTextureSize = [](void* ptr, int id, int* w, int* h) {
            auto& self = *static_cast<TestRenderer*>(ptr);
            auto texture = self.textures.find(id);
            if (texture == self.textures.end()) { ++self.invalid_accesses; return 0; }
            *w = texture->second.width;
            *h = texture->second.height;
            return 1;
        };
        params.renderFill = [](void*, NVGpaint*, NVGcompositeOperationState,
            NVGscissor*, float, const float*, const NVGpath*, int) {};
        params.renderStroke = [](void*, NVGpaint*, NVGcompositeOperationState,
            NVGscissor*, float, float, const NVGpath*, int) {};
        vg = nvgCreateInternal(&params, nullptr);
        REQUIRE(vg);
    }

    ~TestRenderer() { nvgDeleteInternal(vg); }

    void draw(SpectralImageDisplay& display) {
        rack::widget::Widget::DrawArgs args = {};
        args.vg = vg;
        display.draw_spectrogram(args);
    }

    void destroy_context(SpectralImageDisplay& display) {
        rack::widget::Widget::ContextDestroyEvent event;
        event.vg = vg;
        display.onContextDestroy(event);
    }
};

/// Supply the engine required by Spectre without opening a Rack window.
struct RackContext {
    rack::Context context;
    RackContext() {
        rack::contextSet(&context);
        context.engine = new rack::engine::Engine;
        context.engine->setSampleRate(48000.f);
    }
    ~RackContext() { rack::contextSet(nullptr); }
};

}  // namespace

TEST_CASE("Spectre recreates its texture after NanoVG context destruction") {
    RackContext context;
    Spectrogram module;
    // Capture a non-silent signal, then freeze before recreating the context.
    // Simulate Rack connecting a mono cable; setChannels ignores unpatched ports.
    module.inputs[Spectrogram::INPUT_SIGNAL].channels = 1;
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = 48000.f;
    args.sampleTime = 1.f / args.sampleRate;
    for (int sample = 0; sample < 8192; ++sample) {
        module.inputs[Spectrogram::INPUT_SIGNAL].setVoltage(
            5.f * std::sin(2.f * M_PI * 1000.f * sample * args.sampleTime));
        module.process(args);
    }
    json_t* state = module.dataToJson();
    json_object_set_new(state, "is_running", json_false());
    module.dataFromJson(state);
    json_decref(state);
    const auto history = module.get_coefficients();
    bool has_signal = false;
    for (const auto& spectrum : history)
        for (const auto value : spectrum)
            has_signal = has_signal || std::abs(value) > 0.f;
    REQUIRE(has_signal);
    SpectralImageDisplay display(&module);
    display.setSize(Vec(465, 350));
    for (int cycle = 0; cycle < 3; ++cycle) {
        CAPTURE(cycle);
        TestRenderer renderer;
        rack::widget::Widget::ContextCreateEvent event;
        event.vg = renderer.vg;
        display.onContextCreate(event);
        renderer.draw(display);
        CHECK(renderer.created == 1);
        renderer.draw(display);
        CHECK(renderer.created == 1);
        CHECK(renderer.updated == 1);
        renderer.destroy_context(display);
        CHECK(renderer.deleted == 1);
        renderer.destroy_context(display);
        CHECK(renderer.deleted == 1);
        CHECK(renderer.invalid_accesses == 0);
        CHECK(bool(module.get_coefficients() == history));
        json_t* saved = module.dataToJson();
        CHECK(json_is_false(json_object_get(saved, "is_running")));
        json_decref(saved);
    }
}

TEST_CASE("Spectre releases textures when the widget is deleted") {
    RackContext context;
    Spectrogram module;
    TestRenderer renderer;
    {
        SpectralImageDisplay display(&module);
        display.setSize(Vec(465, 350));
        renderer.draw(display);
        REQUIRE(renderer.created == 1);
    }
    CHECK(renderer.deleted == 1);
    CHECK(renderer.invalid_accesses == 0);
    // Browser previews and widgets removed before their first draw own no image.
    {
        SpectralImageDisplay preview(nullptr);
        renderer.destroy_context(preview);
    }
    CHECK(renderer.deleted == 1);
    CHECK(renderer.invalid_accesses == 0);
}

TEST_CASE("Spectre retries failed texture creation without using an invalid handle") {
    RackContext context;
    Spectrogram module;
    TestRenderer renderer;
    {
        SpectralImageDisplay display(&module);
        display.setSize(Vec(465, 350));
        renderer.fail_creation = true;
        renderer.draw(display);
        CHECK(renderer.created == 0);
        CHECK(renderer.invalid_accesses == 0);
        renderer.fail_creation = false;
        renderer.draw(display);
        CHECK(renderer.created == 1);
        CHECK(renderer.invalid_accesses == 0);
    }
    CHECK(renderer.deleted == 1);
}

TEST_CASE("Spectre uses the owning context when switching between live renderers") {
    RackContext context;
    Spectrogram module;
    TestRenderer first;
    TestRenderer second;
    {
        SpectralImageDisplay display(&module);
        display.setSize(Vec(465, 350));
        first.draw(display);
        second.draw(display);
        CHECK(first.created == 1);
        CHECK(first.deleted == 1);
        CHECK(second.created == 1);
        CHECK(second.updated == 0);
    }
    CHECK(second.deleted == 1);
    CHECK(first.invalid_accesses == 0);
    CHECK(second.invalid_accesses == 0);
}
