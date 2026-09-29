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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DISPLAY_TEST_SUPPORT_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DISPLAY_TEST_SUPPORT_HPP_

#include <map>
#include <stdexcept>
#include <vector>
#include "rack.hpp"

namespace DisplayTest {

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
    std::vector<unsigned char> last_pixels;
    NVGcontext* vg = nullptr;

    TestRenderer() {
        NVGparams params = {};
        params.userPtr = this;
        params.renderCreate = [](void*, void*) { return 1; };
        params.renderCreateTexture = [](void* ptr, int type, int w, int h,
                                        int, const unsigned char* data) {
            auto& self = *static_cast<TestRenderer*>(ptr);
            if (type == NVG_TEXTURE_RGBA && self.fail_creation) return 0;
            const int id = self.next_id++;
            self.textures.emplace(id, Texture{w, h, type});
            if (type == NVG_TEXTURE_RGBA) {
                ++self.created;
                self.last_pixels.assign(data, data + w * h * 4);
            }
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
                                        const unsigned char* data) {
            auto& self = *static_cast<TestRenderer*>(ptr);
            if (!self.textures.count(id)) { ++self.invalid_accesses; return 0; }
            const auto& texture = self.textures.at(id);
            self.last_pixels.assign(data, data + texture.width * texture.height * 4);
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
        if (!vg) throw std::runtime_error("Cannot create test NanoVG context");
    }

    ~TestRenderer() { nvgDeleteInternal(vg); }

    template<typename Display>
    void draw(Display& display) {
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
        context.event = new rack::widget::EventState;
        context.engine->setSampleRate(48000.f);
    }
    ~RackContext() { rack::contextSet(nullptr); }
};

}  // namespace DisplayTest

#endif  // ARHYTHMETIC_UNITS_FOURIER_DISPLAY_TEST_SUPPORT_HPP_
