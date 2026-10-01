// Repeated headless display and engine workloads; no GPU timing.
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

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#ifndef FOURIER_SPECTROGRAM_SOURCE
#define FOURIER_SPECTROGRAM_SOURCE "../../src/Spectrogram.cpp"
#endif
#include FOURIER_SPECTROGRAM_SOURCE
#include "../../test/rack/display_test_support.hpp"

Plugin* plugin_instance = nullptr;

int main() {
    DisplayTest::RackContext context;
    std::cout << "workload,repeat,draw_us_per_frame,engine_us_per_sample,uploads\n";
    for (const auto mode : {"frozen", "running", "settings", "engine"}) {
        for (int repeat = 0; repeat < 7; ++repeat) {
            Spectrogram module;
            module.inputs[Spectrogram::INPUT_SIGNAL].channels = 1;
            rack::engine::Module::ProcessArgs args = {};
            args.sampleRate = 48000.f;
            args.sampleTime = 1.f / args.sampleRate;
            int sample = 0;
            auto process = [&]() {
                module.inputs[Spectrogram::INPUT_SIGNAL].setVoltage(
                    5.f * std::sin(2.f * M_PI * 1000.f * sample++ * args.sampleTime));
                module.process(args);
            };
            for (int i = 0; i < 8192; ++i) process();
            if (std::string(mode) != "running" && std::string(mode) != "engine") {
                json_t* state = module.dataToJson();
                json_object_set_new(state, "is_running", json_false());
                module.dataFromJson(state);
                json_decref(state);
            }
            DisplayTest::TestRenderer renderer;
            SpectralImageDisplay display(&module);
            display.setSize(Vec(465, 350));
            renderer.draw(display);
            double draw_seconds = 0;
            double engine_seconds = 0;
            const int frames = 120;
            const bool engine_only = std::string(mode) == "engine";
            const bool running = std::string(mode) == "running" || engine_only;
            const int samples_per_frame = engine_only ? 32768 : 800;
            for (int frame = 0; frame < frames; ++frame) {
                auto start = std::chrono::steady_clock::now();
                if (running)
                    for (int i = 0; i < samples_per_frame; ++i) process();
                auto finish = std::chrono::steady_clock::now();
                engine_seconds += std::chrono::duration<double>(finish - start).count();
                if (std::string(mode) == "settings")
                    module.params[Spectrogram::PARAM_SLOPE].setValue(frame % 2 ? 3.f : 4.5f);
                start = std::chrono::steady_clock::now();
                if (!engine_only) renderer.draw(display);
                finish = std::chrono::steady_clock::now();
                draw_seconds += std::chrono::duration<double>(finish - start).count();
            }
            std::cout << mode << ',' << repeat << ',' << std::setprecision(9)
                << 1e6 * draw_seconds / frames << ','
                << (running ? 1e6 * engine_seconds / (frames * samples_per_frame) : 0)
                << ',' << renderer.created + renderer.updated << std::endl;
        }
    }
}
