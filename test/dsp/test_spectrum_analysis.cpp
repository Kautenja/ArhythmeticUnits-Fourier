// Numerical, scheduling, lifecycle and allocation contracts for spectral analysis.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <cstdlib>
#include <new>
#include <vector>
#include "dsp/spectrum_analysis.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

namespace {
bool tracking = false;
size_t allocations = 0;
using Fourier::SpectrumAnalysis;
using Fourier::SpectrumSettings;

/// Different frame contents expose accidental reads of newer ring samples.
float signal(size_t index, int fixture) {
    switch (fixture) {
    case 0: return 0.f;
    case 1: return index == 0 || index == 71 ? 1.f : 0.f;
    case 2: return 1.f;
    case 3: return index % 2 ? -1.f : 1.f;
    case 4: return std::sin(0.17f * index) + 0.25f * std::cos(0.61f * index);
    default: return float((uint32_t(index) * 1664525u + 1013904223u) >> 8) / 8388608.f - 1.f;
    }
}
}
void* operator new(size_t bytes) {
    if (tracking) ++allocations;
    if (void* result = std::malloc(bytes ? bytes : 1)) return result;
    throw std::bad_alloc();
}
void* operator new[](size_t bytes) { return ::operator new(bytes); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }

TEST_CASE("One-hop spectra match the original RFFT across frames and settings") {
    SpectrumAnalysis<float> analysis(16384, 32768);
    for (size_t n : {4u, 16u, 128u, 2048u, 16384u}) {
        for (size_t hop : {size_t(1), n/4, n/2+1, n+7}) {
            for (float octave : {0.f, 1.f/3.f, 2.5f}) {
                for (int fixture = n > 128 ? 5 : 0; fixture < 6; ++fixture) {
                    SpectrumSettings settings;
                    settings.length = n;
                    settings.hop = hop;
                    settings.window = fixture % 2 ? Fourier::Window::Function::Hann
                                                 : Fourier::Window::Function::Boxcar;
                    settings.octave = octave;
                    settings.alpha = 0.8f;
                    analysis.reset();
                    REQUIRE(analysis.configure(settings));
                    Fourier::OnTheFlyRFFT<float> reference(n);
                    Fourier::Window::CachedWindow<float> window(settings.window, n, false, true);
                    std::vector<float> input(n), previous(n/2+1, 0.f), result(n/2+1);
                    const size_t w = analysis.work_per_frame(), k = n/2+1;
                    for (size_t t = 0; t < 6*hop; ++t) {
                        size_t emitted = 0;
                        const bool ready = analysis.process(signal(t, fixture), [&](size_t bin, float value) {
                            result[bin] = value;
                            ++emitted;
                        });
                        const size_t phase = t%hop;
                        const auto outputs = [=](size_t completed) {
                            return completed > w-k ? completed-(w-k) : 0;
                        };
                        REQUIRE(emitted == outputs((phase+1)*w/hop)-outputs(phase*w/hop));
                        REQUIRE(ready == ((t+1)%hop == 0));
                        if (!ready) continue;
                        const size_t frame_end = t+1-hop;
                        for (size_t i = 0; i < n; ++i) {
                            const int64_t index = int64_t(frame_end)-int64_t(n)+1+i;
                            input[i] = index < 0 ? 0.f : signal(size_t(index), fixture);
                        }
                        reference.buffer(input.data(), window.get_samples());
                        reference.compute();
                        if (octave > 0) reference.smooth(settings.sample_rate, octave);
                        for (size_t bin = 0; bin <= n/2; ++bin) {
                            previous[bin] = settings.alpha * previous[bin]
                                + (1.f-settings.alpha)*std::abs(reference.coefficients[bin]);
                            CAPTURE(n, hop, octave, fixture, frame_end, bin);
                            REQUIRE(result[bin] == Approx(previous[bin]).margin(1e-5f));
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Scheduled magnitudes agree with an independent direct DFT") {
    SpectrumAnalysis<double> analysis(128, 64);
    SpectrumSettings settings;
    settings.length = 32;
    settings.hop = 7;
    REQUIRE(analysis.configure(settings));
    std::vector<double> result(17);
    for (size_t t = 0; t < 160; ++t) {
        if (!analysis.process(signal(t, 4), [&](size_t k, double value) { result[k] = value; })) continue;
        for (size_t k = 0; k < result.size(); ++k) {
            std::complex<double> expected(0, 0);
            for (size_t i = 0; i < settings.length; ++i) {
                const int64_t index = int64_t(t+1-settings.hop)-int64_t(settings.length)+1+i;
                const double angle = -2*std::acos(-1.)*k*i/settings.length;
                const double value = index < 0 ? 0. : signal(size_t(index), 4);
                expected += value * std::complex<double>(std::cos(angle), std::sin(angle));
            }
            REQUIRE(result[k] == Approx(std::abs(expected)).margin(1e-10));
        }
    }
}

TEST_CASE("Control changes and reset cannot retain partial caches or old input") {
    SpectrumAnalysis<float> reused(2048, 4096);
    for (int phase : {0, 1, 31, 97}) {
        SpectrumSettings before;
        before.length = 128;
        before.hop = 101;
        before.window = Fourier::Window::Function::Flattop;
        before.octave = 2.5f;
        reused.reset();
        REQUIRE(reused.configure(before));
        for (int i = 0; i < phase; ++i) reused.process(1.f, [](size_t, float) {});
        if (phase) CHECK_FALSE(reused.configure(before));
        reused.reset();
        SpectrumSettings after;
        after.length = 2048;
        after.hop = 307;
        after.window = Fourier::Window::Function::BlackmanHarris;
        after.alpha = 0.3f;
        after.octave = 1.f/6.f;
        after.sample_rate = 96000.f;
        SpectrumAnalysis<float> fresh(2048, 4096);
        REQUIRE(reused.configure(after));
        REQUIRE(fresh.configure(after));
        std::vector<float> a(1025), b(1025);
        for (size_t i = 0; i < 10*after.hop; ++i) {
            const bool ready = reused.process(signal(i, 5), [&](size_t k, float v) { a[k] = v; });
            REQUIRE(fresh.process(signal(i, 5), [&](size_t k, float v) { b[k] = v; }) == ready);
            if (ready) REQUIRE(a == b);
        }
    }
}

TEST_CASE("All windows and controls can change without allocation in processing") {
    SpectrumAnalysis<float> analysis(16384, 65536);
    allocations = 0;
    bool valid = true;
    tracking = true;
    for (size_t n : {128u, 16384u, 2048u, 4u}) {
        for (int w = 0; w <= int(Fourier::Window::Function::Flattop); ++w) {
            SpectrumSettings settings;
            settings.length = n;
            settings.hop = n+7;
            settings.window = static_cast<Fourier::Window::Function>(w);
            settings.octave = w/6.f;
            valid = analysis.configure(settings) && valid;
            for (size_t i = 0; i < 2*settings.hop; ++i)
                analysis.process(1.f, [](size_t, float) {});
        }
    }
    tracking = false;
    REQUIRE(valid);
    CHECK(allocations == 0);
}

TEST_CASE("Freeze retains input while analysis continues at the same cadence") {
    SpectrumAnalysis<float> analysis(128, 37);
    SpectrumSettings settings;
    settings.length = 128;
    settings.hop = 37;
    REQUIRE(analysis.configure(settings));
    for (size_t i = 0; i < 10*37; ++i) analysis.process(1.f, [](size_t, float) {});
    std::vector<float> values(65);
    for (size_t i = 0; i < 3*37; ++i) {
        const bool complete = analysis.process(99.f, [&](size_t k, float v) { values[k] = v; }, false);
        if (complete) {
            CHECK(values[0] == Approx(128.f));
            for (size_t k = 1; k < values.size(); ++k) CHECK(values[k] == 0.f);
        }
    }
}

TEST_CASE("Invalid configuration is rejected before changing active settings") {
    CHECK_THROWS_AS(SpectrumAnalysis<float>(3, 1), std::invalid_argument);
    CHECK_THROWS_AS(SpectrumAnalysis<float>(8, 0), std::invalid_argument);
    SpectrumAnalysis<float> analysis(128, 37);
    SpectrumSettings good;
    good.length = 128;
    good.hop = 37;
    REQUIRE(analysis.configure(good));
    for (int field = 0; field < 7; ++field) {
        auto bad = good;
        switch (field) {
        case 0: bad.length = 127; break;
        case 1: bad.hop = 0; break;
        case 2: bad.hop = 38; break;
        case 3: bad.alpha = NAN; break;
        case 4: bad.octave = 3.f; break;
        case 5: bad.sample_rate = 0.f; break;
        case 6: bad.window = static_cast<Fourier::Window::Function>(99); break;
        }
        CHECK_FALSE(analysis.configure(bad));
        CHECK(analysis.size() == 128);
        CHECK(analysis.hop_length() == 37);
    }
    analysis.reserve_hop(1000);
    good.hop = 1000;
    CHECK(analysis.configure(good));
}

TEST_CASE("Live window and band caches preserve coherent gain and original smoothing") {
    SpectrumAnalysis<float> analysis(128, 37);
    SpectrumSettings settings;
    settings.length = 128;
    settings.hop = 37;
    REQUIRE(analysis.configure(settings));
    for (size_t i = 0; i < 10*37; ++i) analysis.process(1.f, [](size_t, float) {});
    std::vector<float> input(128, 1.f), result(65);
    Fourier::OnTheFlyRFFT<float> reference(128);
    for (int w = 0; w <= int(Fourier::Window::Function::Flattop); ++w) {
        settings.window = static_cast<Fourier::Window::Function>(w);
        settings.octave = w % 2 ? 0.5f : 0.f;
        settings.sample_rate = w % 2 ? 44100.f : 96000.f;
        REQUIRE(analysis.configure(settings));
        Fourier::Window::CachedWindow<float> window(settings.window, 128, false, true);
        reference.buffer(input.data(), window.get_samples());
        reference.compute();
        if (settings.octave) reference.smooth(settings.sample_rate, settings.octave);
        for (size_t i = 0; i < 37; ++i)
            analysis.process(1.f, [&](size_t k, float v) { result[k] = v; }, false);
        for (size_t k = 0; k < result.size(); ++k) {
            CAPTURE(w, k);
            REQUIRE(result[k] == Approx(std::abs(reference.coefficients[k])).margin(1e-5f));
        }
    }
}
