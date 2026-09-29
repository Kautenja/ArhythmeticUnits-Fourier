// CPU-side curve tessellation, axis layout, and spectrogram image preparation.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>
#include "../../src/SpectrumAnalyzer.cpp"
#include "../../src/Spectrogram.cpp"
#include "catch.hpp"
#include "../fixtures.hpp"
#include "../../test/rack/display_test_support.hpp"

Plugin* plugin_instance = nullptr;

namespace {

/// @brief Count real NanoVG tessellation while retaining the test texture sink.
/// @details No GL context or font/window is required. Frame boundaries reset
/// NanoVG state; pixel ratio is one. Texture uploads copy bytes into host RAM.
struct Renderer : DisplayTest::TestRenderer {
    size_t strokes = 0, fills = 0, vertices = 0;

    Renderer() {
        auto* params = nvgInternalParams(vg);
        params->renderViewport = [](void*, float, float, float) {};
        params->renderFlush = [](void*) {};
        params->renderStroke = [](void* pointer, NVGpaint*, NVGcompositeOperationState,
                NVGscissor*, float, float, const NVGpath* paths, int count) {
            auto& self = *static_cast<Renderer*>(static_cast<DisplayTest::TestRenderer*>(pointer));
            ++self.strokes;
            for (int i = 0; i < count; ++i) self.vertices += paths[i].nstroke;
        };
        params->renderFill = [](void* pointer, NVGpaint*, NVGcompositeOperationState,
                NVGscissor*, float, const float*, const NVGpath* paths, int count) {
            auto& self = *static_cast<Renderer*>(static_cast<DisplayTest::TestRenderer*>(pointer));
            ++self.fills;
            for (int i = 0; i < count; ++i) self.vertices += paths[i].nfill + paths[i].nstroke;
        };
    }

    /// @brief Begin one headless UI frame at logical size 660 by 350 pixels.
    rack::widget::Widget::DrawArgs begin() {
        strokes = fills = vertices = 0;
        nvgBeginFrame(vg, 660.f, 350.f, 1.f);
        rack::widget::Widget::DrawArgs args = {};
        args.vg = vg;
        return args;
    }

    void end() { nvgEndFrame(vg); }

    /// @brief Run the real Spectre image update and image/scan-line paths.
    void image(SpectralImageDisplay& display) {
        const auto args = begin();
        display.draw_spectrogram(args);
        end();
    }
};

/// @brief Fill every history column through the real engine before UI timing.
void fill_history(Spectrogram& module) {
    const auto input = BenchmarkFixtures::signal<float>(Spectrogram::N_FFT);
    module.inputs[Spectrogram::INPUT_SIGNAL].channels = 1;
    module.set_window_function(Fourier::Window::Function::Hann);
    module.set_slope(0.f);
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = 48000.f;
    args.sampleTime = 1.f/args.sampleRate;
    const size_t samples = Spectrogram::N_FFT
        + (Spectrogram::N_STFT + 1) * module.get_hop_length();
    for (size_t i = 0; i < samples; ++i) {
        module.inputs[Spectrogram::INPUT_SIGNAL].setVoltage(5.f * input[i%input.size()]);
        module.process(args);
    }
    const auto* column = module.consume_display_column(0, true);
    REQUIRE(column != nullptr);
    REQUIRE(column->revision > 0);
    REQUIRE(column->values[7] > 1.f);
}

/// @brief Rebuild the real cached artwork without allocating a GL framebuffer.
/// @details The artwork callback clears labels before each draw, preserving
/// normal string/layout work without accumulating labels across iterations.
template<typename Display>
void benchmark_axes(Display& display, Renderer& renderer, const std::string& name) {
    auto* cache = dynamic_cast<Fourier::CachedDisplay*>(display.children.front());
    REQUIRE(cache != nullptr);
    display.prepare_axes_cache();
    auto draw = [&]() {
        const auto args = renderer.begin();
        cache->artwork->draw(args);
        renderer.end();
        Catch::Benchmark::keep_memory(cache->labels.data());
    };
    draw();
    const size_t labels = cache->labels.size();
    REQUIRE(labels > 0);
    BENCHMARK(name + " / one axis artwork rebuild") { draw(); };
    REQUIRE(cache->labels.size() == labels);
    REQUIRE(renderer.vertices > 0);
}

}  // namespace

TEST_CASE("Fourier curves: four traces per frame", "[curves]") {
    DisplayTest::RackContext context;
    SpectrumAnalyzer module;
    SpectrumAnalyzerDisplay display(&module);
    display.setSize(Vec(660, 350));
    Renderer renderer;
    for (const size_t n : {128u, 2048u, 16384u}) {
        const size_t bins = n/2+1;
        const auto signal = BenchmarkFixtures::signal<float>(bins);
        std::array<std::vector<Vec>, 4> points;
        for (const bool cropped : {false, true}) {
            for (size_t lane = 0; lane < points.size(); ++lane) {
                points[lane].resize(bins);
                for (size_t k = 0; k < bins; ++k) {
                    const float x = static_cast<float>(k) / (bins-1);
                    // Include clipped peaks and out-of-range frequencies.
                    points[lane][k] = Vec(cropped ? 2.f*x-0.5f : x,
                        0.2f + 0.2f*lane + signal[(k+13*lane)%bins]);
                }
            }
            for (const bool bezier : {false, true}) {
                for (const bool fill : {false, true}) {
                    module.is_bezier_enabled = bezier;
                    module.is_fill_enabled = fill;
                    auto draw = [&]() {
                        const auto args = renderer.begin();
                        for (const auto& lane : points) {
                            Catch::Benchmark::keep_memory(lane.data());
                            display.draw_coefficients(args, lane.data(), bins, 1.5f);
                        }
                        renderer.end();
                    };
                    draw(); // Prepare NanoVG's path storage before measurement.
                    BENCHMARK("N=" + std::to_string(n)
                            + (cropped ? " cropped" : " full")
                            + (bezier ? " Bezier" : " lines")
                            + (fill ? " filled" : " stroke") + " / four traces") { draw(); };
                    REQUIRE(renderer.strokes == 4);
                    REQUIRE(renderer.fills == (fill ? 4 : 0));
                    REQUIRE(renderer.vertices > 0);
                }
            }
        }
    }
}

TEST_CASE("Axis artwork and label layout", "[axes]") {
    DisplayTest::RackContext context;
    SpectrumAnalyzer fourier;
    Spectrogram spectre;
    SpectrumAnalyzerDisplay spectrum(&fourier);
    SpectralImageDisplay image(&spectre);
    spectrum.setSize(Vec(660, 350));
    image.setSize(Vec(465, 350));
    Renderer renderer;
    for (const auto frequency : {FrequencyScale::Linear, FrequencyScale::Logarithmic}) {
        fourier.set_frequency_scale(frequency);
        spectre.set_frequency_scale(frequency);
        const auto& scale = frequency_scale_names()[static_cast<size_t>(frequency)];
        benchmark_axes(image, renderer, "Spectre frequency=" + scale);
        for (const auto magnitude : {MagnitudeScale::Linear,
                MagnitudeScale::Logarithmic60dB, MagnitudeScale::Logarithmic120dB}) {
            fourier.set_magnitude_scale(magnitude);
            benchmark_axes(spectrum, renderer, "Fourier frequency=" + scale
                + " magnitude=" + magnitude_scale_names()[static_cast<size_t>(magnitude)]);
        }
    }
}

TEST_CASE("Spectre retained image and full recoloring", "[image]") {
    DisplayTest::RackContext context;
    Spectrogram module;
    fill_history(module);
    Renderer renderer;
    SpectralImageDisplay display(&module);
    display.setSize(Vec(465, 350));
    renderer.image(display);
    REQUIRE(renderer.created == 1);
    REQUIRE(renderer.last_pixels.size() == 4u * Spectrogram::N_STFT * (Spectrogram::N_FFT/2));
    const auto original_pixels = renderer.last_pixels;
    const int uploads = renderer.updated;
    BENCHMARK("Spectre unchanged / one frame, zero uploads") { renderer.image(display); };
    REQUIRE(renderer.updated == uploads);

    bool cropped = false;
    BENCHMARK("Spectre crop+resize / one frame, zero uploads") {
        cropped = !cropped;
        module.set_low_frequency(cropped ? 100.f : 0.f);
        module.set_high_frequency(cropped ? 10000.f : 24000.f);
        display.setSize(cropped ? Vec(600, 400) : Vec(465, 350));
        renderer.image(display);
    };
    REQUIRE(renderer.updated == uploads);
    REQUIRE(renderer.last_pixels == original_pixels);
    module.set_low_frequency(0.f);
    module.set_high_frequency(24000.f);
    display.setSize(Vec(465, 350));

    for (const auto scale : {FrequencyScale::Linear, FrequencyScale::Logarithmic}) {
        module.set_frequency_scale(scale);
        for (size_t map = 0; map < Fourier::ColorMap::names().size(); ++map) {
            module.color_map = static_cast<Fourier::ColorMap::Function>(map);
            renderer.image(display);
            const int before = renderer.updated;
            size_t iterations = 0;
            BENCHMARK("Spectre slope rebuild " + Fourier::ColorMap::names()[map]
                    + " " + frequency_scale_names()[static_cast<size_t>(scale)] + " / full image") {
                // Invalidate every timed iteration, including calibration.
                module.set_slope(module.get_slope() == 0.f ? 4.5f : 0.f);
                renderer.image(display);
                ++iterations;
            };
            REQUIRE(renderer.updated - before == static_cast<int>(iterations));
        }
    }
    REQUIRE(renderer.invalid_accesses == 0);
}

TEST_CASE("Spectre reopening imports retained history", "[history]") {
    DisplayTest::RackContext context;
    Spectrogram module;
    fill_history(module);
    Renderer renderer;
    std::vector<unsigned char> expected;
    {
        SpectralImageDisplay display(&module);
        display.setSize(Vec(465, 350));
        renderer.image(display);
        expected = renderer.last_pixels;
    }
    const int before = renderer.created;
    size_t iterations = 0;
    BENCHMARK("Spectre construct+history import+first image+destroy / one widget") {
        SpectralImageDisplay display(&module);
        display.setSize(Vec(465, 350));
        renderer.image(display);
        ++iterations;
    };
    REQUIRE(renderer.created - before == static_cast<int>(iterations));
    REQUIRE(renderer.created == renderer.deleted);
    REQUIRE(renderer.last_pixels == expected);
    REQUIRE(renderer.invalid_accesses == 0);
}
