// Native Rack panel inspection; requires a graphical desktop session.
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
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "../../src/Spectrogram.cpp"
#include "../../src/SpectrumAnalyzer.cpp"
Plugin* plugin_instance = nullptr;

/// @brief Render the real module widgets, optionally using archived SVG panels.
/// @details Run in a graphical desktop session; no audio device or patch is opened.
int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        std::cerr << "Usage: inspect_panels RACK_DIR PLUGIN_DIR OUTPUT_PREFIX [SVG_REFERENCE_DIR]\n";
        return 1;
    }
    rack::Context context;
    rack::contextSet(&context);
    context.engine = new rack::engine::Engine;
    context.engine->setSampleRate(48000.f);
    context.event = new rack::widget::EventState;
    context.history = new rack::history::State;
    rack::asset::systemDir = argv[1];
    rack::plugin::Plugin plugin;
    plugin.path = argv[2];
    plugin.slug = "ArhythmeticUnits-Fourier";
    plugin.version = "2.1.2";
    plugin.addModel(modelSpectrogram);
    plugin.addModel(modelSpectrumAnalyzer);
    rack::plugin::plugins.push_back(&plugin);
    plugin_instance = &plugin;
    if (!glfwInit()) return 2;
    context.window = new rack::window::Window;
    glfwHideWindow(context.window->win);
    glfwSetWindowSize(context.window->win, 1280, 410);
    // ModuleWidget::draw() uses the scene's selection state.
    context.scene = new rack::app::Scene;
    int result = 0;
    try {
        for (bool preview : {false, true}) {
            auto fourier = preview ? nullptr : static_cast<SpectrumAnalyzer*>(modelSpectrumAnalyzer->createModule());
            auto spectre = preview ? nullptr : static_cast<Spectrogram*>(modelSpectrogram->createModule());
            if (fourier) {
                fourier->id = 1;
                context.engine->addModule(fourier);
            }
            if (spectre) {
                spectre->id = 2;
                context.engine->addModule(spectre);
            }
            std::unique_ptr<SpectrumAnalyzerWidget> fw(new SpectrumAnalyzerWidget(fourier));
            std::unique_ptr<SpectrogramWidget> sw(new SpectrogramWidget(spectre));
            if (argc == 5) {
                const std::string reference = argv[4];
                fw->setPanel(createPanel(reference + "/SpectrumAnalyzer-Light.svg",
                    reference + "/SpectrumAnalyzer-Dark.svg"));
                sw->setPanel(createPanel(reference + "/Spectrogram-Light.svg",
                    reference + "/Spectrogram-Dark.svg"));
            }
            if (fw->box.size.x != 720.f || sw->box.size.x != 525.f ||
                fw->box.size.y != 380.f || sw->box.size.y != 380.f)
                throw std::runtime_error("Module dimensions changed");
            auto frequency_control = dynamic_cast<TextKnob*>(
                fw->getParam(SpectrumAnalyzer::PARAM_FREQUENCY_SCALE));
            auto spectre_frequency_control = dynamic_cast<TextKnob*>(
                sw->getParam(Spectrogram::PARAM_FREQUENCY_SCALE));
            if (!frequency_control || !spectre_frequency_control)
                throw std::runtime_error("Missing frequency controls");
            SpectrumAnalyzerDisplay* fourier_display = nullptr;
            SpectralImageDisplay* spectre_display = nullptr;
            for (auto child : fw->children)
                if (auto display = dynamic_cast<SpectrumAnalyzerDisplay*>(child)) fourier_display = display;
            for (auto child : sw->children)
                if (auto display = dynamic_cast<SpectralImageDisplay*>(child)) spectre_display = display;
            if (!fourier_display || !spectre_display)
                throw std::runtime_error("Missing display widgets");
            if (!preview) {
                rack::engine::Module::ProcessArgs process = {};
                process.sampleRate = 48000.f;
                process.sampleTime = 1.f / process.sampleRate;
                for (int channel = 0; channel < 4; ++channel)
                    fourier->inputs[SpectrumAnalyzer::INPUT_SIGNAL + channel].channels = 1;
                spectre->inputs[Spectrogram::INPUT_SIGNAL].channels = 1;
                for (int i = 0; i < 100000; ++i) {
                    for (int channel = 0; channel < 4; ++channel) {
                        const float voltage = 5.f * std::sin(2.f * M_PI *
                            (250.f * (1 << channel)) * i * process.sampleTime);
                        fourier->inputs[SpectrumAnalyzer::INPUT_SIGNAL + channel].setVoltage(voltage);
                    }
                    spectre->inputs[Spectrogram::INPUT_SIGNAL].setVoltage(
                        5.f * std::sin(2.f * M_PI * 1000.f * i * process.sampleTime));
                    fourier->process(process);
                    spectre->process(process);
                }
            }
            sw->step();
            for (const int id : {Spectrogram::PARAM_COLOR_FLOOR, Spectrogram::PARAM_COLOR_CEILING}) {
                auto handle = dynamic_cast<SpectreIntensityHandle*>(sw->getParam(id));
                if (!handle || handle->box.pos.x < 6.f || handle->box.getBottomRight().x > 69.f ||
                    handle->box.pos.y < 106.f || handle->box.getBottomRight().y > 300.f)
                    throw std::runtime_error("Intensity handle bounds escaped the left strip");
            }
            const auto control = Fourier::PanelLayout::intensity_control();
            if (control.pos.x < 6.f || control.pos.y < 106.f ||
                control.getBottomRight().x > 69.f || control.getBottomRight().y > 300.f)
                throw std::runtime_error("Color control bounds escaped the left strip");
            SpectreIntensityLegend* legend = nullptr;
            for (auto child : sw->children)
                if (auto candidate = dynamic_cast<SpectreIntensityLegend*>(child)) legend = candidate;
            if (!legend) throw std::runtime_error("Missing color control");
            // The screen margins, bar, and endpoint labels must not light either row.
            const Vec hover_points[] = {Vec(25.f, 14.f), Vec(25.f, 32.f), Vec(25.f, 110.f),
                Vec(3.f, 14.f), Vec(25.f, 43.f), Vec(25.f, 48.f), Vec(0.f, 0.f)};
            for (int i = 0; i < 7; ++i) {
                rack::widget::EventContext target;
                Widget::HoverEvent hover;
                hover.context = &target;
                hover.pos = control.pos.plus(hover_points[i]);
                sw->onHover(hover);
                context.event->setHoveredWidget(target.target);
                if (legend->hovered_choice != (!preview && i < 2 ? i : -1))
                    throw std::runtime_error("Color menu highlight escaped its own row");
            }
            context.event->setHoveredWidget(nullptr);
            if (legend->hovered_choice != -1)
                throw std::runtime_error("Color menu highlight survived pointer exit");
            if (!preview) {
                rack::ui::Menu context_menu;
                sw->appendContextMenu(&context_menu);
                for (auto child : context_menu.children)
                    if (auto item = dynamic_cast<rack::ui::MenuItem*>(child))
                        if (item->text == "Intensity scale" || item->text == "Color Map")
                            throw std::runtime_error("Duplicate color choice in context menu");
                // Clicking the gap beneath the dropdowns must not open either menu.
                const auto child_count = context.scene->children.size();
                rack::widget::EventContext target;
                Widget::ButtonEvent press;
                press.context = &target;
                press.pos = control.pos.plus(Vec(25.f, 43.f));
                press.button = GLFW_MOUSE_BUTTON_LEFT;
                press.action = GLFW_PRESS;
                sw->onButton(press);
                if (context.scene->children.size() != child_count)
                    throw std::runtime_error("Color screen gap opened a dropdown");
                // Route presses through the complete widget tree, then use the
                // actual Rack drag callbacks and history rather than setting params.
                for (const int id : {Spectrogram::PARAM_COLOR_FLOOR, Spectrogram::PARAM_COLOR_CEILING,
                                     Spectrogram::PARAM_LINEAR_FLOOR, Spectrogram::PARAM_LINEAR_CEILING}) {
                    spectre->intensity_scale = id >= Spectrogram::PARAM_LINEAR_FLOOR ?
                        Spectrogram::Intensity::Scale::Linear : Spectrogram::Intensity::Scale::Decibels;
                    sw->step();
                    auto handle = sw->getParam(id);
                    const float before = spectre->params[id].getValue();
                    rack::widget::EventContext target;
                    Widget::ButtonEvent press;
                    press.context = &target;
                    press.pos = handle->box.getCenter();
                    press.button = GLFW_MOUSE_BUTTON_LEFT;
                    press.action = GLFW_PRESS;
                    press.mods = 0;
                    sw->onButton(press);
                    if (target.target != handle)
                        throw std::runtime_error("Color handle did not receive the panel press");
                    Widget::DragStartEvent start;
                    start.button = GLFW_MOUSE_BUTTON_LEFT;
                    handle->onDragStart(start);
                    Widget::DragMoveEvent move;
                    move.button = GLFW_MOUSE_BUTTON_LEFT;
                    move.mouseDelta = Vec(0.f, -5.f);
                    handle->onDragMove(move);
                    Widget::DragEndEvent end;
                    end.button = GLFW_MOUSE_BUTTON_LEFT;
                    handle->onDragEnd(end);
                    if (spectre->params[id].getValue() <= before)
                        throw std::runtime_error("Color handle drag did not change its parameter");
                    context.history->undo();
                    if (spectre->params[id].getValue() != before)
                        throw std::runtime_error("Color handle undo did not restore its parameter");
                }
            }
            if (spectre) {
                // Open both actual dropdowns through the panel, choose an item,
                // and verify the action and undo before destroying the overlay.
                for (const bool palette : {true, false}) {
                    rack::widget::EventContext target;
                    Widget::ButtonEvent press;
                    press.context = &target;
                    press.pos = control.pos.plus(Vec(25.f, palette ? 12.f : 33.f));
                    press.button = GLFW_MOUSE_BUTTON_LEFT;
                    press.action = GLFW_PRESS;
                    press.mods = 0;
                    sw->onButton(press);
                    if (!dynamic_cast<SpectreIntensityLegend*>(target.target))
                        throw std::runtime_error("Color dropdown did not receive the panel press");
                    auto overlay = dynamic_cast<rack::ui::MenuOverlay*>(context.scene->children.back());
                    if (!overlay || overlay->children.empty())
                        throw std::runtime_error("Color dropdown did not create an overlay");
                    auto menu = dynamic_cast<rack::ui::Menu*>(overlay->children.front());
                    if (!menu || menu->children.size() != (palette ? 7u : 2u))
                        throw std::runtime_error("Color dropdown choices are incomplete");
                    auto item = dynamic_cast<rack::ui::MenuItem*>(menu->children.front());
                    if (!item || item->text != (palette ? "Viridis" : "Decibels"))
                        throw std::runtime_error("Unexpected dropdown order");
                    Widget::ActionEvent action;
                    item->onAction(action);
                    if ((palette && spectre->color_map != Fourier::ColorMap::Function::Viridis) ||
                        (!palette && spectre->intensity_scale != Spectrogram::Intensity::Scale::Decibels))
                        throw std::runtime_error("Dropdown action failed");
                    context.history->undo();
                    if ((palette && spectre->color_map != Fourier::ColorMap::Function::Magma) ||
                        (!palette && spectre->intensity_scale != Spectrogram::Intensity::Scale::Linear))
                        throw std::runtime_error("Dropdown undo failed");
                    context.scene->removeChild(overlay);
                    delete overlay;
                }
                spectre->intensity_scale = Spectrogram::Intensity::Scale::Decibels;
            }
            std::vector<unsigned char> dark_pixels;
            // Exercise hover events on the actual widget through Rack's event state.
            for (int scenario = 0; scenario < 36; ++scenario) {
                rack::settings::preferDarkPanels = scenario == 1 || scenario == 3 || scenario >= 5;
                const float zoom = scenario >= 18 ? (scenario % 3 == 0 ? 1.f :
                    (scenario % 3 == 1 ? 0.75f : 0.5f)) :
                    (scenario == 7 ? 0.5f : (scenario == 2 ? 0.75f : 1.f));
                if (scenario >= 18) {
                    rack::settings::preferDarkPanels = (scenario / 3) % 2;
                    if (spectre) {
                        spectre->intensity_scale = scenario >= 24 ?
                            Spectrogram::Intensity::Scale::Linear : Spectrogram::Intensity::Scale::Decibels;
                        spectre->params[Spectrogram::PARAM_COLOR_FLOOR].setValue(-120.f);
                        spectre->params[Spectrogram::PARAM_COLOR_CEILING].setValue(24.f);
                    }
                }
                context.event->setHoveredWidget(scenario == 5 ? frequency_control : nullptr);
                if (frequency_control->hovered != (scenario == 5 && !preview))
                    throw std::runtime_error("Text control hover state is incorrect");
                if (scenario >= 8 && scenario < 18) {
                    const bool spectre_hover = scenario >= 13;
                    auto owner = spectre_hover ? static_cast<ModuleWidget*>(sw.get()) : fw.get();
                    auto display = spectre_hover ? static_cast<Widget*>(spectre_display) : fourier_display;
                    const float left = spectre_hover ? 40.f : 35.f;
                    const Vec positions[] = {Vec(100.f, 150.f), Vec(left - 1.f, 150.f),
                        Vec(100.f, 19.f), Vec(display->box.size.x - 15.f, 150.f), Vec(100.f, 300.f)};
                    rack::widget::EventContext hover_context;
                    Widget::HoverEvent hover;
                    hover.context = &hover_context;
                    hover.pos = display->box.pos.plus(positions[(scenario - 8) % 5]);
                    owner->onHover(hover);
                    context.event->setHoveredWidget(hover_context.target);
                    const bool inside = (scenario == 8 || scenario == 13) && !preview;
                    if ((hover_context.target == display) != inside)
                        throw std::runtime_error("Display hover target escaped the plot rectangle");
                }
                if (scenario >= 30) {
                    rack::settings::preferDarkPanels = scenario >= 33;
                    rack::widget::EventContext hover_context;
                    Widget::HoverEvent hover;
                    hover.context = &hover_context;
                    hover.pos = control.pos.plus(hover_points[scenario % 3]);
                    sw->onHover(hover);
                    context.event->setHoveredWidget(hover_context.target);
                }
                if (scenario == 4) {
                    rack::widget::Widget::ContextDestroyEvent destroy;
                    destroy.vg = context.window->vg;
                    fw->onContextDestroy(destroy);
                    sw->onContextDestroy(destroy);
                    delete context.window;
                    context.window = new rack::window::Window;
                    glfwHideWindow(context.window->win);
                    glfwSetWindowSize(context.window->win, 1280, 410);
                    rack::widget::Widget::ContextCreateEvent create;
                    create.vg = context.window->vg;
                    fw->onContextCreate(create);
                    sw->onContextCreate(create);
                }
                int available_width, available_height;
                glfwGetFramebufferSize(context.window->win, &available_width, &available_height);
                const float native_ratio = available_width / 1280.f;
                const float ratio = scenario == 3 ? 1.f : native_ratio;
                const int width = static_cast<int>(1280 * ratio);
                const int height = static_cast<int>(410 * ratio);
                if (width > available_width || height > available_height)
                    throw std::runtime_error("Window framebuffer is too small for inspection");
                context.window->pixelRatio = ratio;
                // Rack limits framebuffer rebuilds per frame, so allow every
                // panel, screw, port, knob and display to populate its cache.
                for (int frame = 0; frame < 40; ++frame) {
                    fw->step();
                    sw->step();
                    if (frequency_control->label.text != "FREQ SCALE" ||
                        spectre_frequency_control->label.text != "FREQ SCALE")
                        throw std::runtime_error("Frequency labels diverged after step");
                    auto vg = context.window->vg;
                    nvgBeginFrame(vg, 1280, 410, ratio);
                    rack::widget::Widget::DrawArgs args = {};
                    args.vg = vg;
                    args.clipBox = Rect(Vec(0, 0), Vec(1280, 410));
                    for (auto widget : {static_cast<rack::app::ModuleWidget*>(fw.get()),
                                        static_cast<rack::app::ModuleWidget*>(sw.get())}) {
                        context.window->fbCount() = 0;
                        nvgSave(vg);
                        nvgTranslate(vg, widget == fw.get() ? 10.f : 740.f, 15.f);
                        nvgScale(vg, zoom, zoom);
                        widget->draw(args);
                        widget->drawLayer(args, 1);
                        nvgRestore(vg);
                    }
                    glViewport(0, 0, width, height);
                    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    nvgEndFrame(vg);
                    glFinish();
                    if (glGetError() != GL_NO_ERROR) throw std::runtime_error("OpenGL error");
                }
                for (auto widget : {static_cast<rack::app::ModuleWidget*>(fw.get()),
                                    static_cast<rack::app::ModuleWidget*>(sw.get())}) {
                    auto cache = dynamic_cast<rack::widget::FramebufferWidget*>(
                        widget->getPanel()->children.front());
                    if (!cache || cache->dirty || cache->getImageHandle() <= 0)
                        throw std::runtime_error("Panel framebuffer did not settle");
                }
                std::vector<unsigned char> pixels(width * height * 3);
                glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
                const std::string filename = std::string(argv[3]) +
                    (preview ? "-preview-" : "-live-") + std::to_string(scenario) + ".ppm";
                std::ofstream output(filename, std::ios::binary);
                output << "P6\n" << width << ' ' << height << "\n255\n";
                for (int y = height - 1; y >= 0; --y)
                    output.write(reinterpret_cast<const char*>(pixels.data() + y * width * 3), width * 3);
                if (!output) throw std::runtime_error("Cannot write " + filename);
                if (scenario == 1) dark_pixels = pixels;
                if (scenario == 5 && (pixels == dark_pixels) != preview)
                    throw std::runtime_error("Hover must highlight live controls only");
                if (scenario == 5) {
                    // NanoVG's glyph antialiasing extends beyond the nominal box.
                    const auto bounds = frequency_control->box.grow(Vec(1.f, 1.f));
                    for (int y = 0; y < height; ++y) {
                        for (int x = 0; x < width; ++x) {
                            const Vec point((x + 0.5f) / ratio - 10.f,
                                (height - y - 0.5f) / ratio - 15.f);
                            if (bounds.contains(point)) continue;
                            const auto offset = (y * width + x) * 3;
                            for (int color = 0; color < 3; ++color) {
                                if (pixels[offset + color] != dark_pixels[offset + color])
                                    throw std::runtime_error("Hover changed pixels outside its control");
                            }
                        }
                    }
                }
                if (scenario == 6 && pixels != dark_pixels)
                    throw std::runtime_error("Hover highlighting did not clear on leave");
                if (scenario >= 8 && scenario < 18) {
                    const bool inside = (scenario == 8 || scenario == 13) && !preview;
                    if ((pixels != dark_pixels) != inside)
                        throw std::runtime_error("Cursor overlay must appear only inside a live plot");
                }
                if (!preview && (fourier->params[SpectrumAnalyzer::PARAM_FREQUENCY_SCALE].getValue() != 1.f ||
                    spectre->params[Spectrogram::PARAM_FREQUENCY_SCALE].getValue() != 1.f))
                    throw std::runtime_error("Rendering changed frequency scale parameters");
                std::cout << filename << " theme=" << rack::settings::preferDarkPanels
                    << " zoom=" << zoom << " pixelRatio=" << ratio << " GL=OK\n";
            }
            context.event->setHoveredWidget(nullptr);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 3;
    }
    delete context.scene;
    context.scene = nullptr;
    delete context.window;
    context.window = nullptr;
    glfwTerminate();
    rack::contextSet(nullptr);
    return result;
}
