// Native Rack framebuffer inspection; requires a graphical desktop session.
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
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "../../src/Spectrogram.cpp"
#include "../../src/SpectrumAnalyzer.cpp"
Plugin* plugin_instance = nullptr;

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: inspect_displays RACK_DIR PLUGIN_DIR OUTPUT_PREFIX\n";
        return 1;
    }
    rack::Context context;
    rack::contextSet(&context);
    context.engine = new rack::engine::Engine;
    context.engine->setSampleRate(48000.f);
    context.event = new rack::widget::EventState;
    rack::asset::systemDir = argv[1];
    rack::plugin::Plugin plugin;
    plugin.path = argv[2];
    plugin_instance = &plugin;
    if (!glfwInit()) return 2;
    // Rack briefly creates a native window; subsequent rendering is hidden.
    context.window = new rack::window::Window;
    glfwHideWindow(context.window->win);
    glfwSetWindowSize(context.window->win, 1200, 400);
    {
        SpectrumAnalyzer fourier;
        Spectrogram spectre;
        SpectrumAnalyzerDisplay spectrum(&fourier);
        SpectralImageDisplay spectrogram(&spectre);
        spectrum.setSize(Vec(660, 350));
        spectrogram.setSize(Vec(465, 350));
        fourier.inputs[SpectrumAnalyzer::INPUT_SIGNAL].channels = 1;
        spectre.inputs[Spectrogram::INPUT_SIGNAL].channels = 1;
        rack::engine::Module::ProcessArgs process = {};
        process.sampleRate = 48000.f;
        process.sampleTime = 1.f / process.sampleRate;
        for (int i = 0; i < 100000; ++i) {
            float v = 5.f * std::sin(2.f * M_PI * 1000.f * i * process.sampleTime);
            fourier.inputs[SpectrumAnalyzer::INPUT_SIGNAL].setVoltage(v);
            spectre.inputs[Spectrogram::INPUT_SIGNAL].setVoltage(v);
            fourier.process(process);
            spectre.process(process);
        }
        auto fc = dynamic_cast<Fourier::CachedDisplay*>(spectrum.children.front());
        auto sc = dynamic_cast<Fourier::CachedDisplay*>(spectrogram.children.front());
        for (int frame = 0; frame < 5; ++frame) {
            if (frame == 2) {
                rack::widget::Widget::ContextDestroyEvent destroy;
                destroy.vg = context.window->vg;
                spectrum.onContextDestroy(destroy);
                spectrogram.onContextDestroy(destroy);
                delete context.window;
                context.window = new rack::window::Window;
                glfwHideWindow(context.window->win);
                glfwSetWindowSize(context.window->win, 1200, 400);
                rack::widget::Widget::ContextCreateEvent create;
                create.vg = context.window->vg;
                spectrum.onContextCreate(create);
                spectrogram.onContextCreate(create);
            }
            if (frame == 3) {
                fourier.params[SpectrumAnalyzer::PARAM_FREQUENCY_SCALE].setValue(0);
                spectre.params[Spectrogram::PARAM_FREQUENCY_SCALE].setValue(0);
                // Fourier reprojects its trace at the next completed spectrum.
                for (int i = 0; i < 4096; ++i) {
                    const float voltage = 5.f * std::sin(2.f * M_PI * 1000.f *
                        (100000 + i) * process.sampleTime);
                    fourier.inputs[SpectrumAnalyzer::INPUT_SIGNAL].setVoltage(voltage);
                    spectre.inputs[Spectrogram::INPUT_SIGNAL].setVoltage(voltage);
                    fourier.process(process);
                    spectre.process(process);
                }
            }
            // Exercise zoom invalidation independently of display settings.
            const float zoom = frame == 4 ? 0.8f : 1.f;
            int width, height;
            glfwGetFramebufferSize(context.window->win, &width, &height);
            const float ratio = width / 1200.f;
            context.window->pixelRatio = ratio;
            auto vg = context.window->vg;
            nvgBeginFrame(vg, 1200, 400, ratio);
            rack::widget::Widget::DrawArgs args = {};
            args.vg = vg;
            args.clipBox = Rect(Vec(0, 0), Vec(1200, 400));
            context.window->fbCount() = 0;
            nvgSave(vg);
            nvgTranslate(vg, 10, 15);
            nvgScale(vg, zoom, zoom);
            spectrum.drawLayer(args, 1);
            nvgRestore(vg);
            context.window->fbCount() = 0;
            nvgSave(vg);
            nvgTranslate(vg, 700, 15);
            nvgScale(vg, zoom, zoom);
            spectrogram.drawLayer(args, 1);
            nvgRestore(vg);
            glViewport(0, 0, width, height);
            glClearColor(0.2f, 0.2f, 0.2f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            nvgEndFrame(vg);
            glFinish();
            const auto error = glGetError();
            if (fc->getImageHandle() <= 0 || sc->getImageHandle() <= 0 ||
                fc->dirty || sc->dirty || error != GL_NO_ERROR) return 3;
            std::cout << frame << " handles=" << fc->getImageHandle() << ',' << sc->getImageHandle()
                << " dirty=" << fc->dirty << ',' << sc->dirty << " glError=" << error << std::endl;
            if (frame == 1 || frame == 2 || frame == 3 || frame == 4) {
                std::vector<unsigned char> pixels(width * height * 3);
                glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
                std::ofstream output(std::string(argv[3]) + "-" + std::to_string(frame) + ".ppm", std::ios::binary);
                output << "P6\n" << width << ' ' << height << "\n255\n";
                for (int y = height - 1; y >= 0; --y)
                    output.write(reinterpret_cast<const char*>(pixels.data() + y * width * 3), width * 3);
            }
        }
    }
    delete context.window;
    context.window = nullptr;
    glfwTerminate();
    rack::contextSet(nullptr);
}
