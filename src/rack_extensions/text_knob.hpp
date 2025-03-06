// A text based knob parameter.
//
// Copyright 2025 Arhythmetic Units
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

#include <string>
#include "rack.hpp"
#include "../structs.hpp"
#include "../dsp/math.hpp"

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_TEXT_KNOB_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_TEXT_KNOB_HPP_

/// @brief A knob that renders the label and value as text on the widget.
struct TextKnob : rack::app::Knob {
    struct {
        /// The text for the label.
        std::string text = "";
        /// The color of the font for the label.
        NVGcolor color = {{{1.f, 1.f, 1.f, 1.f}}};
        /// The size for the font
        float font_size = 10.f;
        /// The line height for the font.
        float line_height = 11.f;
        /// the font for rendering text on the display
        std::shared_ptr<rack::Font> font = APP->window->loadFont(
            rack::asset::plugin(plugin_instance, "res/Font/Arial/Bold.ttf")
        );
    } label, value;  // The label and value text.

    /// @brief Initialize a new text knob.
    TextKnob() {
        // Set the expected size of the widget from Sketch
        setSize(Vec(60, 30));
        // Set the range of the knob (mocks a Rogan knob)
        minAngle = 0.f * M_PI;
        maxAngle = 1.66f * M_PI;
        // Set the default colors for the label and value.
        label.color = {{{0.f / 255.f, 90.f / 255.f, 11.f / 255.f, 1.f}}};
        value.color = {{{0.f / 255.f, 215.f / 255.f, 26.f / 255.f, 1.f}}};
    }

    /// @brief Respond to changes of the parameter.
    void onChange(const ChangeEvent& e) override {
        auto param = getParamQuantity();
        if (param) {
            label.text = param->getLabel();
            for (char &c : label.text)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            value.text = param->getDisplayValueString() + param->getUnit();
        }
        rack::app::Knob::onChange(e);
    }

    /// @brief Draw the layer on the screen.
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {
            // render the label.
            nvgFontSize(args.vg, label.font_size);
            nvgFontFaceId(args.vg, label.font->handle);
            nvgFillColor(args.vg, label.color);
            nvgTextLineHeight(args.vg, label.line_height);
            nvgTextAlign(args.vg, NVG_ALIGN_TOP | NVG_ALIGN_CENTER);
            nvgText(args.vg, box.size.x / 2.f, 0, label.text.c_str(), NULL);
            // Render the value.
            nvgFontSize(args.vg, value.font_size);
            nvgFontFaceId(args.vg, value.font->handle);
            nvgFillColor(args.vg, value.color);
            nvgTextLineHeight(args.vg, value.line_height);
            nvgTextAlign(args.vg, NVG_ALIGN_TOP | NVG_ALIGN_CENTER);
            nvgText(args.vg, box.size.x / 2.f, 18, value.text.c_str(), NULL);
        }
        rack::app::Knob::drawLayer(args, layer);
    }
};

/// @brief A menu item for changing a parameter widget value.
struct ParamWidgetMenuItem : MenuItem {
    /// The parameter widget to update.
    rack::app::ParamWidget* param_widget = nullptr;
    /// The option for this menu item.
    float value;

    /// @brief Respond to the menu item being selected.
    inline void onAction(const event::Action& e) override {
        param_widget->getParamQuantity()->setValue(value);
    }
};

/// @brief A text knob with window function option selection when right-clicked.
struct WindowFunctionTextKnob : TextKnob {
    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        for (size_t i = 0; i < 15; i++) {  // TODO: get total number of windows automatically
            const auto label = Math::Window::name(static_cast<Math::Window::Function>(i));
            const auto check = CHECKMARK(getParamQuantity()->getValue() == i);
            auto item = createMenuItem<ParamWidgetMenuItem>(label, check);
            item->value = i;
            item->param_widget = this;
            menu->addChild(item);
        }
    }
};

/// @brief A text knob with window length option selection when right-clicked.
struct WindowLengthTextKnob : TextKnob {
    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        for (size_t i = 7; i < 15; i++) {
            const auto label = std::to_string(1 << i);
            const auto check = CHECKMARK(getParamQuantity()->getValue() == i);
            auto item = createMenuItem<ParamWidgetMenuItem>(label, check);
            item->value = i;
            item->param_widget = this;
            menu->addChild(item);
        }
    }
};

/// @brief A text knob with frequency scale option selection when right-clicked.
struct FrequencyScaleTextKnob : TextKnob {
    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        for (size_t i = 0; i < static_cast<size_t>(FrequencyScale::NumOptions); i++) {
            const auto label = to_string(static_cast<FrequencyScale>(i));
            const auto check = CHECKMARK(getParamQuantity()->getValue() == i);
            auto item = createMenuItem<ParamWidgetMenuItem>(label, check);
            item->value = i;
            item->param_widget = this;
            menu->addChild(item);
        }
    }
};

/// @brief A text knob with magnitude scale option selection when right-clicked.
struct MagnitudeScaleTextKnob : TextKnob {
    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        for (size_t i = 0; i < static_cast<size_t>(MagnitudeScale::NumOptions); i++) {
            const auto label = to_string(static_cast<MagnitudeScale>(i));
            const auto check = CHECKMARK(getParamQuantity()->getValue() == i);
            auto item = createMenuItem<ParamWidgetMenuItem>(label, check);
            item->value = i;
            item->param_widget = this;
            menu->addChild(item);
        }
    }
};

/// @brief A text knob with frequency smoothing option selection when right-clicked.
struct FrequencySmoothingTextKnob : TextKnob {
    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        for (size_t i = 0; i < static_cast<size_t>(FrequencySmoothing::NumOptions); i++) {
            const auto label = to_string(static_cast<FrequencySmoothing>(i));
            const auto check = CHECKMARK(getParamQuantity()->getValue() == i);
            auto item = createMenuItem<ParamWidgetMenuItem>(label, check);
            item->value = i;
            item->param_widget = this;
            menu->addChild(item);
        }
    }
};

#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_TEXT_KNOB_HPP_
