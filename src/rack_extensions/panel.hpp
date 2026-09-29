// Shared programmatic panels and control geometry for Fourier and Spectre.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_PANEL_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_PANEL_HPP_

#include "rack.hpp"
#include "panel_artwork.hpp"

namespace Fourier {

/// @brief The two fixed panel formats; dimensions are in Rack pixels (15 per HP).
enum class PanelKind { FOURIER, SPECTRE };

/// @brief Geometry shared by the painted panel and its interactive controls.
struct PanelLayout {
    /// @brief Fixed module size; preserve the existing 48 HP and 35 HP footprints.
    static rack::math::Vec size(PanelKind kind) {
        return rack::math::Vec(kind == PanelKind::FOURIER ? 720.f : 525.f, 380.f);
    }
    /// @brief Top-left port position for channel 0 through 3.
    static rack::math::Vec input(int channel = 0) {
        return rack::math::Vec(11.f, 30.f + 75.f * channel);
    }
    /// @brief Top-left gain knob position for channel 0 through 3.
    static rack::math::Vec gain(int channel = 0) {
        return rack::math::Vec(13.f, 66.f + 75.f * channel);
    }
    /// @brief Shared center of the Run button and its light.
    static rack::math::Vec run() { return rack::math::Vec(23.f, 346.f); }
    /// @brief Top-left display corner, inset from the panel and input strip.
    static rack::math::Vec display_position() { return rack::math::Vec(45.f, 15.f); }
    /// @brief Display footprint inside the fixed module size.
    static rack::math::Vec display_size(PanelKind kind) {
        return size(kind).minus(rack::math::Vec(60.f, 30.f));
    }
};

/// @brief A themed, cached panel with native geometry and fixed vector lettering.
/// @details UI-thread only. Rack owns framebuffer zoom/context invalidation;
/// theme and pixel-ratio changes also invalidate this panel's static artwork.
struct Panel : rack::widget::Widget {
 private:
    /// @brief Geometry and branding drawn only when the framebuffer is dirty.
    struct Artwork : rack::widget::Widget {
        /// Module format and the theme last observed on the UI thread.
        PanelKind kind;
        bool dark = false;

        explicit Artwork(PanelKind kind) : kind(kind) {}

        void draw(const DrawArgs& args) override {
            auto vg = args.vg;
            const NVGcolor ink = dark ? nvgRGB(255, 255, 255) : nvgRGB(0, 0, 0);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, 0.f, 0.f, box.size.x, box.size.y, 5.f);
            nvgFillColor(vg, dark ? nvgRGB(0, 0, 0) : nvgRGB(230, 230, 230));
            nvgFill(vg);

            const NVGcolor channels[] = {
                nvgRGB(255, 0, 0), nvgRGB(0, 255, 0),
                nvgRGB(0, 0, 255), nvgRGB(255, 255, 0)
            };
            const int count = kind == PanelKind::FOURIER ? 4 : 1;
            for (int channel = 0; channel < count; ++channel) {
                const auto input = PanelLayout::input(channel);
                nvgBeginPath(vg);
                // Retain the original second-channel artwork's one-pixel offset.
                const float offset = channel == 1 ? 1.f : 0.f;
                nvgRect(vg, input.x + 7.f, input.y + 10.f + offset, 8.f, 37.f);
                nvgFillColor(vg, kind == PanelKind::FOURIER ? channels[channel] :
                    (dark ? nvgRGB(230, 230, 230) : ink));
                nvgFill(vg);
            }

            nvgSave(vg);
            nvgTranslate(vg, box.size.x / 2.f - 59.8189f -
                (kind == PanelKind::SPECTRE ? 0.5f : 0.f), dark ? 365.3448f : 365.3226f);
            nvgFillColor(vg, ink);
            nvgSave(vg);
            nvgTranslate(vg, 0.f, dark ? 1.5372f : 1.5595f);
            PanelArtwork::brand_icon(vg);
            nvgRestore(vg);
            nvgTranslate(vg, 12.9457f, dark ? 1.5569f : 1.9403f);
            // The original dark artwork has slightly taller wordmark lettering.
            if (dark) nvgScale(vg, 1.f, 31.f / 29.f);
            PanelArtwork::brand_wordmark(vg);
            nvgRestore(vg);

            nvgSave(vg);
            nvgFillColor(vg, rack::color::alpha(ink, 0.8f));
            if (kind == PanelKind::FOURIER) {
                nvgTranslate(vg, 50.084f, 2.9531f);
                PanelArtwork::fourier(vg);
            } else {
                nvgTranslate(vg, 49.6328f, 2.7773f);
                PanelArtwork::spectre(vg);
            }
            nvgRestore(vg);

            nvgSave(vg);
            nvgFillColor(vg, rack::color::alpha(ink, 0.4f));
            if (kind == PanelKind::FOURIER) {
                nvgTranslate(vg, box.size.x - 139.9744f, 2.7529f);
                PanelArtwork::spectrum_analyzer(vg);
            } else {
                nvgTranslate(vg, box.size.x - 165.3855f, 2.7529f);
                PanelArtwork::spectrogram_visualizer(vg);
            }
            nvgRestore(vg);

            nvgSave(vg);
            const auto run = PanelLayout::run();
            nvgTranslate(vg, run.x - 9.895f, run.y - 28.54f);
            nvgFillColor(vg, ink);
            PanelArtwork::run(vg);
            nvgRestore(vg);
        }
    };

    /// Rack owns these child widgets for the entire panel lifetime.
    rack::widget::FramebufferWidget* cache;
    Artwork* artwork;

 public:
    /// @brief Create the artwork cache and Rack's standard panel border.
    explicit Panel(PanelKind kind) {
        box.size = PanelLayout::size(kind);
        cache = new rack::widget::FramebufferWidget;
        cache->box.size = box.size;
        addChild(cache);
        artwork = new Artwork(kind);
        artwork->dark = rack::settings::preferDarkPanels;
        artwork->box.size = box.size;
        cache->addChild(artwork);
        auto border = new rack::app::PanelBorder;
        border->box.size = box.size;
        cache->addChild(border);
    }

    /// @brief Refresh static artwork when the theme or oversampling changes.
    void step() override {
        const bool dark = rack::settings::preferDarkPanels;
        const float oversample = APP->window->pixelRatio < 2.f ? 2.f : 1.f;
        if (artwork->dark != dark || cache->oversample != oversample) {
            artwork->dark = dark;
            cache->oversample = oversample;
            cache->setDirty();
        }
        rack::widget::Widget::step();
    }
};

}  // namespace Fourier
#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_PANEL_HPP_
