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

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <limits>
#include <string>
#include <vector>
#include "../../src/Spectrogram.cpp"
#include "../../src/SpectrumAnalyzer.cpp"
#include "catch_amalgamated.hpp"

Plugin* plugin_instance = nullptr;

#include "display_test_support.hpp"
using DisplayTest::TestRenderer;
using DisplayTest::RackContext;

/// Read a coherent published history for the independent image reference.
/// Tests use the same UI thread and call this after draws (or before creating
/// a display, which initially imports each mailbox's current snapshot).
Fourier::STFTCoefficients published_history(Spectrogram& module) {
    Fourier::STFTCoefficients history(Spectrogram::N_STFT,
        Fourier::DFTCoefficients(Spectrogram::N_FFT));
    for (size_t i = 0; i < history.size(); ++i) {
        const auto* column = module.consume_display_column(i, true);
        std::copy(column->values.begin(), column->values.end(), history[i].begin());
    }
    return history;
}

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
    const auto history = published_history(module);
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
        CHECK(renderer.updated == 0);
        renderer.destroy_context(display);
        CHECK(renderer.deleted == 1);
        renderer.destroy_context(display);
        CHECK(renderer.deleted == 1);
        CHECK(renderer.invalid_accesses == 0);
        CHECK(bool(published_history(module) == history));
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

namespace {

/// Original full-image calculation, independent of cache state and dirty columns.
std::vector<unsigned char> reference_pixels(Spectrogram& module) {
    const auto& coefficients = published_history(module);
    const int width = coefficients.size();
    const int height = coefficients[0].size() / 2;
    std::vector<unsigned char> result(width * height * 4);
    for (int y = 0; y < height; ++y) {
        float gain = log2f((y / static_cast<float>(height)) *
            (module.get_sample_rate() / 2.f) / 1000.f + std::numeric_limits<float>::epsilon());
        gain = Fourier::decibels2amplitude(module.get_slope() * gain);
        for (int x = 0; x < width; ++x) {
            float position = y;
            if (module.get_frequency_scale() == FrequencyScale::Logarithmic)
                position = height * Fourier::squared(position / height);
            auto coefficient = gain * Fourier::interpolate_coefficients(coefficients[x], position);
            auto color = Fourier::ColorMap::color_map(module.color_map, abs(coefficient) / height);
            const int index = 4 * (width * (height - 1 - y) + x);
            result[index] = color.r * 255;
            result[index + 1] = color.g * 255;
            result[index + 2] = color.b * 255;
            result[index + 3] = 255;
        }
    }
    return result;
}

void advance_signal(Spectrogram& module, int samples) {
    module.inputs[Spectrogram::INPUT_SIGNAL].channels = 1;
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = module.get_sample_rate();
    args.sampleTime = 1.f / args.sampleRate;
    for (int i = 0; i < samples; ++i) {
        module.inputs[Spectrogram::INPUT_SIGNAL].setVoltage(
            5.f * std::sin(2.f * M_PI * 1000.f * i * args.sampleTime));
        module.process(args);
    }
}

}  // namespace

TEST_CASE("Spectre caches pixels and invalidates every pixel-affecting setting") {
    RackContext context;
    Spectrogram module;
    TestRenderer renderer;
    SpectralImageDisplay display(&module);
    display.setSize(Vec(465, 350));
    advance_signal(module, 8192);
    renderer.draw(display);
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));
    renderer.draw(display);
    CHECK(renderer.updated == 0);

    // Pan/crop and resize only transform the existing texture.
    module.set_low_frequency(200.f);
    module.set_high_frequency(6000.f);
    display.setSize(Vec(800, 400));
    renderer.draw(display);
    CHECK(renderer.updated == 0);

    module.params[Spectrogram::PARAM_SLOPE].setValue(-3.f);
    renderer.draw(display);
    CHECK(renderer.updated == 1);
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));
    module.params[Spectrogram::PARAM_FREQUENCY_SCALE].setValue(static_cast<float>(FrequencyScale::Linear));
    renderer.draw(display);
    CHECK(renderer.updated == 2);
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));
    module.color_map = Fourier::ColorMap::Function::Gray;
    renderer.draw(display);
    CHECK(renderer.updated == 3);
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));
    context.context.engine->setSampleRate(96000.f);
    module.onSampleRateChange();
    renderer.draw(display);
    CHECK(renderer.updated == 4);
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));

    // Several new columns, and then more than a full history wrap without a draw.
    advance_signal(module, 4096);
    renderer.draw(display);
    CHECK(renderer.updated == 5);
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));
    advance_signal(module, (Spectrogram::N_STFT + 4) * 1024);
    renderer.draw(display);
    CHECK(renderer.updated == 6);
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));

    module.onReset();
    renderer.draw(display);
    CHECK(renderer.updated == 7);
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));
    renderer.draw(display);
    CHECK(renderer.updated == 7);
}

template<typename Display, typename Module>
void check_axes_cache(Display& display, Module& module, int low, int high, int scale) {
    auto cache = dynamic_cast<Fourier::CachedDisplay*>(display.children.front());
    REQUIRE(cache);
    display.setSize(Vec(660, 350));
    display.prepare_axes_cache();
    REQUIRE(cache->dirty);
    cache->setDirty(false);
    display.prepare_axes_cache();
    CHECK_FALSE(cache->dirty);
    module.params[0].setValue(0.5f);
    display.prepare_axes_cache();
    CHECK_FALSE(cache->dirty);
    // Layout is generated on cache rebuild, with no font/window dependency.
    TestRenderer renderer;
    rack::widget::Widget::DrawArgs args = {};
    args.vg = renderer.vg;
    cache->artwork->draw(args);
    REQUIRE_FALSE(cache->labels.empty());
    const auto label_count = cache->labels.size();
    cache->artwork->draw(args);
    CHECK(cache->labels.size() == label_count);
    for (auto param : {low, high, scale}) {
        module.params[param].setValue(module.params[param].getValue() == 0.f ? 1.f : 0.f);
        display.prepare_axes_cache();
        CHECK(cache->dirty);
        cache->setDirty(false);
    }
    display.setSize(Vec(700, 400));
    display.prepare_axes_cache();
    CHECK(cache->dirty);
    cache->setDirty(false);
    APP->engine->setSampleRate(96000.f);
    module.onSampleRateChange();
    display.prepare_axes_cache();
    CHECK(cache->dirty);
    cache->setDirty(false);
    rack::widget::Widget::ContextDestroyEvent destroy;
    destroy.vg = nullptr;
    display.onContextDestroy(destroy);
    CHECK(cache->dirty);
}

TEST_CASE("Both axis caches invalidate their rendering inputs but ignore signal changes") {
    RackContext context;
    SpectrumAnalyzer fourier;
    Spectrogram spectre;
    SpectrumAnalyzerDisplay fourier_display(&fourier);
    SpectralImageDisplay spectre_display(&spectre);
    check_axes_cache(fourier_display, fourier, SpectrumAnalyzer::PARAM_LOW_FREQUENCY,
        SpectrumAnalyzer::PARAM_HIGH_FREQUENCY, SpectrumAnalyzer::PARAM_FREQUENCY_SCALE);
    check_axes_cache(spectre_display, spectre, Spectrogram::PARAM_LOW_FREQUENCY,
        Spectrogram::PARAM_HIGH_FREQUENCY, Spectrogram::PARAM_FREQUENCY_SCALE);
    auto cache = dynamic_cast<Fourier::CachedDisplay*>(fourier_display.children.front());
    cache->setDirty(false);
    fourier.params[SpectrumAnalyzer::PARAM_MAGNITUDE_SCALE].setValue(0.f);
    fourier_display.prepare_axes_cache();
    CHECK(cache->dirty);
}

TEST_CASE("Recreating a frozen Spectre widget retains the last published history") {
    RackContext context;
    Spectrogram module;
    advance_signal(module, 8192);
    TestRenderer renderer;
    std::vector<unsigned char> original;
    {
        SpectralImageDisplay display(&module);
        display.setSize(Vec(465, 350));
        renderer.draw(display);
        original = renderer.last_pixels;
    }
    // No processing/publication between widgets: the consumer must retain its snapshot.
    SpectralImageDisplay recreated(&module);
    recreated.setSize(Vec(465, 350));
    renderer.draw(recreated);
    CHECK(bool(renderer.last_pixels == original));
    CHECK(bool(renderer.last_pixels == reference_pixels(module)));
}

TEST_CASE("Spectre publishes on exact hops and resumes an unfinished frozen frame") {
    RackContext context;
    Spectrogram module;
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = module.get_sample_rate();
    args.sampleTime = 1.f/args.sampleRate;
    const size_t hop = module.get_hop_length();
    for (size_t i = 0; i < hop-1; ++i) module.process(args);
    CHECK(module.get_hop_index() == 0);
    json_t* state = module.dataToJson();
    json_object_set_new(state, "is_running", json_false());
    module.dataFromJson(state);
    for (size_t i = 0; i < 2*hop; ++i) module.process(args);
    CHECK(module.get_hop_index() == 0);
    json_object_set_new(state, "is_running", json_true());
    module.dataFromJson(state);
    json_decref(state);
    module.process(args);
    CHECK(module.get_hop_index() == 1);
    for (size_t i = 0; i < hop; ++i) module.process(args);
    CHECK(module.get_hop_index() == 2);
    // A sample-rate callback cancels a partially computed old-rate frame.
    module.process(args);
    module.onSampleRateChange();
    for (size_t i = 0; i < hop-1; ++i) module.process(args);
    CHECK(module.get_hop_index() == 2);
    module.process(args);
    CHECK(module.get_hop_index() == 3);
}

namespace {

void configure_spectre(Spectrogram& module) {
    module.set_window_function(Fourier::Window::Function::Boxcar);
    module.set_time_smoothing(0.f);
    module.set_frequency_smoothing(FrequencySmoothing::None);
    module.is_ac_coupled = false;
    module.inputs[Spectrogram::INPUT_SIGNAL].channels = 1;
    // Discard reset publications before checking the next engine publication.
    for (size_t column = 0; column < Spectrogram::N_STFT; ++column)
        module.consume_display_column(column);
}

void drive_spectre(Spectrogram& module, size_t sample,
        const rack::engine::Module::ProcessArgs& args) {
    module.inputs[Spectrogram::INPUT_SIGNAL].setVoltage(
        0.3f + std::sin(0.071f * sample) + 0.2f * std::cos(0.19f * sample));
    module.process(args);
}

}  // namespace

TEST_CASE("Spectre latches smoothing and window changes at the next frame boundary") {
    const auto control = GENERATE("time smoothing", "frequency smoothing", "window");
    const size_t phase = GENERATE(1u, 512u, 1023u);
    CAPTURE(control, phase);
    RackContext context;
    Spectrogram changed, boundary, unchanged;
    for (auto* module : {&changed, &boundary, &unchanged}) configure_spectre(*module);
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = changed.get_sample_rate();
    args.sampleTime = 1.f / args.sampleRate;
    const size_t hop = changed.get_hop_length();
    const auto change = [control](Spectrogram& module) {
        const std::string name(control);
        if (name == "time smoothing") module.set_time_smoothing(0.7f);
        else if (name == "frequency smoothing")
            module.set_frequency_smoothing(FrequencySmoothing::_1_1);
        else module.set_window_function(Fourier::Window::Function::Hann);
    };
    size_t sample = 0;
    for (; sample < 3 * hop + phase; ++sample)
        for (auto* module : {&changed, &boundary, &unchanged}) drive_spectre(*module, sample, args);
    change(changed);
    for (size_t i = phase; i < hop; ++i, ++sample) {
        for (auto* module : {&changed, &boundary, &unchanged}) drive_spectre(*module, sample, args);
        REQUIRE(changed.get_hop_index() == (i + 1 == hop ? 4 : 3));
        const auto* column = changed.consume_display_column(3);
        REQUIRE((column != nullptr) == (i + 1 == hop));
        if (column) {
            const auto* expected = boundary.consume_display_column(3);
            REQUIRE(expected != nullptr);
            REQUIRE(column->values == expected->values);
            REQUIRE(*std::max_element(column->values.begin(), column->values.end()) > 0.f);
        }
    }
    change(boundary);
    for (size_t frame = 4; frame < 6; ++frame) {
        for (size_t i = 0; i < hop; ++i, ++sample) {
            for (auto* module : {&changed, &boundary, &unchanged}) drive_spectre(*module, sample, args);
            REQUIRE(changed.get_hop_index() == frame + (i + 1 == hop));
            const auto* column = changed.consume_display_column(frame);
            REQUIRE((column != nullptr) == (i + 1 == hop));
            if (column) {
                const auto* expected = boundary.consume_display_column(frame);
                const auto* old = unchanged.consume_display_column(frame);
                REQUIRE(expected != nullptr);
                REQUIRE(old != nullptr);
                REQUIRE(column->values == expected->values);
                REQUIRE(column->values != old->values);
            }
        }
    }
}

TEST_CASE("Spectre sample-rate changes discard pending nonzero input and averaging") {
    const size_t phase = GENERATE(1u, 512u, 1023u);
    const float rate = GENERATE(44100.f, 96000.f);
    CAPTURE(phase, rate);
    RackContext context;
    context.context.engine->setSampleRate(48000.f);
    Spectrogram reused;
    configure_spectre(reused);
    reused.is_ac_coupled = true;
    reused.set_time_smoothing(0.7f);
    reused.set_frequency_smoothing(FrequencySmoothing::_1_3);
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = 48000.f;
    args.sampleTime = 1.f / args.sampleRate;
    const size_t hop = reused.get_hop_length();
    for (size_t i = 0; i < 3 * hop + phase; ++i) drive_spectre(reused, i, args);
    const auto* held = reused.consume_display_column(2);
    REQUIRE(held != nullptr);
    const auto saved = *held;
    REQUIRE(*std::max_element(saved.values.begin(), saved.values.end()) > 0.f);
    context.context.engine->setSampleRate(rate);
    reused.onSampleRateChange();
    Spectrogram fresh;
    configure_spectre(fresh);
    fresh.is_ac_coupled = true;
    for (size_t i = 0; i < Spectrogram::NUM_PARAMS; ++i)
        fresh.params[i].setValue(reused.params[i].getValue());
    args.sampleRate = rate;
    args.sampleTime = 1.f / rate;
    REQUIRE(reused.get_sample_rate() == rate);
    // Existing display history survives; only pending analysis is cancelled.
    REQUIRE(reused.get_hop_index() == 3);
    REQUIRE(reused.consume_display_column(2) == nullptr);
    REQUIRE(held->values == saved.values);
    REQUIRE(held->revision == saved.revision);
    for (size_t frame = 0; frame < 3; ++frame) {
        for (size_t i = 0; i < hop; ++i) {
            for (auto* module : {&reused, &fresh}) {
                module->inputs[Spectrogram::INPUT_SIGNAL].setVoltage(
                    frame == 0 ? 0.f : std::sin(0.09f * i));
                module->process(args);
            }
            REQUIRE(reused.get_hop_index() == 3 + frame + (i + 1 == hop));
            const auto* column = reused.consume_display_column(3 + frame);
            REQUIRE((column != nullptr) == (i + 1 == hop));
            if (column) {
                const auto* expected = fresh.consume_display_column(frame);
                REQUIRE(expected != nullptr);
                REQUIRE(column->values == expected->values);
                if (frame == 0)
                    for (float value : column->values) REQUIRE(value == 0.f);
            }
        }
    }
}

TEST_CASE("Spectre display scale changes during a frame preserve spectral columns") {
    RackContext context;
    Spectrogram module, reference;
    for (auto* item : {&module, &reference}) configure_spectre(*item);
    TestRenderer renderer;
    SpectralImageDisplay display(&module);
    display.setSize(Vec(465, 350));
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = module.get_sample_rate();
    args.sampleTime = 1.f / args.sampleRate;
    const size_t hop = module.get_hop_length();
    size_t sample = 0;
    for (; sample < 4 * hop - 1; ++sample) {
        drive_spectre(module, sample, args);
        drive_spectre(reference, sample, args);
    }
    renderer.draw(display);
    const auto old_pixels = renderer.last_pixels;
    module.set_frequency_scale(FrequencyScale::Linear);
    // Spectre maps raw columns in the UI: a scale change immediately redraws
    // completed history, without publishing the engine's partially written column.
    renderer.draw(display);
    REQUIRE(renderer.updated == 1);
    REQUIRE(bool(renderer.last_pixels != old_pixels));
    REQUIRE(bool(renderer.last_pixels == reference_pixels(module)));
    REQUIRE(module.get_hop_index() == 3);
    REQUIRE(module.consume_display_column(3) == nullptr);
    for (; sample < 5 * hop; ++sample) {
        drive_spectre(module, sample, args);
        drive_spectre(reference, sample, args);
        if ((sample + 1) % hop == 0) {
            const size_t index = module.get_hop_index() - 1;
            // Let the display consume first, then inspect its held snapshot.
            renderer.draw(display);
            const auto* column = module.consume_display_column(index, true);
            const auto* expected = reference.consume_display_column(index);
            REQUIRE(column != nullptr);
            REQUIRE(expected != nullptr);
            REQUIRE(column->values == expected->values);
            REQUIRE(bool(renderer.last_pixels == reference_pixels(module)));
        }
    }
}

/// Exercise the same hover/enter/leave ordering used by Rack's event dispatcher.
template<typename Display>
void check_plot_hover(Display& display, float left, float width, bool live) {
    rack::widget::EventState events;
    display.setSize(Vec(width, 350.f));
    const auto hover = [&](Vec position, bool inside) {
        CAPTURE(position.x, position.y, width, live);
        rack::widget::EventContext context;
        rack::widget::Widget::HoverEvent event;
        event.context = &context;
        event.pos = position;
        display.onHover(event);
        CHECK((context.target == &display) == (inside && live));
        CHECK(context.consumed == (inside && live));
        events.setHoveredWidget(context.target);
    };
    const float right = width - 15.f;
    const float bottom = 300.f;
    // Enter from every gutter, then leave the plot without leaving the widget.
    for (Vec outside : {Vec(left - 0.01f, 150.f), Vec(100.f, 19.99f),
            Vec(right, 150.f), Vec(100.f, bottom), Vec(100.f, 330.f)}) {
        hover(outside, false);
        hover(Vec(100.f, 150.f), true);
        hover(outside, false);
    }
    // Rack rectangles include the top/left edges and exclude bottom/right.
    hover(Vec(left, 20.f), true);
    hover(Vec(right - 0.01f, bottom - 0.01f), true);
    events.setHoveredWidget(nullptr);
}

TEST_CASE("Display cursor hover is confined to the plot rectangle") {
    RackContext context;
    SpectrumAnalyzer fourier;
    Spectrogram spectre;
    SpectrumAnalyzerDisplay fourier_display(&fourier), fourier_preview(nullptr);
    SpectralImageDisplay spectre_display(&spectre), spectre_preview(nullptr);
    for (float width : {660.f, 700.f}) {
        check_plot_hover(fourier_display, 35.f, width, true);
        check_plot_hover(fourier_preview, 35.f, width, false);
    }
    for (float width : {465.f, 500.f}) {
        check_plot_hover(spectre_display, 40.f, width, true);
        check_plot_hover(spectre_preview, 40.f, width, false);
    }
}
