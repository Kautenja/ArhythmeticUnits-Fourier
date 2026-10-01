// A manually composed self-illuminating framebuffer layer.
//
// Copyright 2026 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_CACHED_DISPLAY_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_CACHED_DISPLAY_HPP_

#include <array>
#include <functional>
#include <string>
#include <vector>
#include "rack.hpp"

namespace Fourier {

/// @brief Cache static display artwork while leaving overlays on the live layer.
/// @details Attach as a child for Rack lifecycle events, then call draw_cached()
/// explicitly on layer 1. Rack handles zoom, pixel ratio and context invalidation.
struct CachedDisplay : rack::widget::FramebufferWidget {
    using Key = std::array<float, 5>;
    struct Artwork : rack::widget::Widget {
        std::function<void(const DrawArgs&)> render;
        void draw(const DrawArgs& args) override { render(args); }
    };
    struct Label {
        rack::math::Vec position;
        std::string text;
        int alignment;
    };
    /// Cached text layout; glyphs are drawn in the live context to avoid missing
    /// glyphs observed when rendering text during framebuffer rebuilds.
    std::vector<Label> labels;
    Artwork* artwork;
    Key key{};
    bool initialized = false;

    explicit CachedDisplay(std::function<void(const DrawArgs&)> render) {
        artwork = new Artwork;
        artwork->render = [this, render](const DrawArgs& args) {
            labels.clear();
            render(args);
        };
        addChild(artwork);
    }

    /// @brief Invalidate on display size or content changes, without rendering.
    void prepare(rack::math::Vec size, const Key& next_key) {
        if (!initialized || !size.equals(box.size) || next_key != key) {
            box.size = artwork->box.size = size;
            key = next_key;
            initialized = true;
            setDirty();
        }
    }

    /// @brief Composite once at the caller's chosen position in the draw order.
    void draw_cached(const DrawArgs& args) {
        rack::widget::FramebufferWidget::draw(args);
    }

    /// @brief Record a label while rebuilding the static artwork.
    void add_label(rack::math::Vec position, const std::string& text, int alignment) {
        labels.push_back({position, text, alignment});
    }

    /// @brief Reuse label strings/positions while rendering glyphs in this context.
    void draw_labels(const DrawArgs& args, int font, float size, NVGcolor color) {
        nvgFontFaceId(args.vg, font);
        nvgFontSize(args.vg, size);
        nvgFillColor(args.vg, color);
        for (const auto& label : labels) {
            nvgTextAlign(args.vg, label.alignment);
            nvgText(args.vg, label.position.x, label.position.y, label.text.c_str(), nullptr);
        }
    }

    // Suppress automatic layer traversal; the parent composes this cache once.
    void draw(const DrawArgs&) override {}
    void drawLayer(const DrawArgs&, int) override {}
};

}  // namespace Fourier
#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_CACHED_DISPLAY_HPP_
