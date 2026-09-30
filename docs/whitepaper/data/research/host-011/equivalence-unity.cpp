// Differential checks against the frozen bb748ec production header.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <rack.hpp>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include "unity-analysis.hpp"
#undef ARHYTHMETIC_UNITS_FOURIER_DSP_SPECTRUM_ANALYSIS_HPP_
#define SpectrumAnalysis BaselineSpectrumAnalysis
#define SpectrumSettings BaselineSpectrumSettings
#include "baseline-analysis.hpp"
#undef SpectrumAnalysis
#undef SpectrumSettings

bool same(rack::simd::float_4 a, rack::simd::float_4 b) {
    for (size_t lane = 0; lane < 4; ++lane)
        if (!std::isfinite(a.s[lane]) || a.s[lane] != b.s[lane]) {
            std::cerr << std::setprecision(18) << "lane=" << lane << " actual=" << a.s[lane]
                << " expected=" << b.s[lane] << "\n";
            return false;
        }
    return true;
}
void require(bool good) { if (!good) throw std::runtime_error("Equivalence failed"); }
template<typename Settings>
Settings configuration(size_t frame) {
    Settings s;
    const size_t lengths[] = {4, 8, 128, 2048, 16384};
    const size_t hops[] = {1, 3, 37, 257, 1024, 4096};
    s.length = lengths[(frame / 6) % 5];
    s.hop = hops[frame % 6];
    s.window = static_cast<Fourier::Window::Function>((frame / 30 + frame % 6) % 15);
    s.alpha = frame % 3 ? .8f : 0.f;
    s.octave = frame % 2 ? 1.f/3.f : 0.f;
    s.sample_rate = frame % 2 ? 44100.f : 96000.f;
    return s;
}
template<typename T>
size_t check(bool exact_schedule) {
    Fourier::SpectrumAnalysis<T> candidate(16384, 4096);
    Fourier::BaselineSpectrumAnalysis<T> reference(16384, 4096);
    std::vector<T> a(8193), b(8193);
    size_t total = 0, sample = 0;
    for (size_t frame = 0; frame < 450; ++frame) {
        const auto config = configuration<Fourier::SpectrumSettings>(frame);
        const auto original = configuration<Fourier::BaselineSpectrumSettings>(frame);
        require(candidate.configure(config) && reference.configure(original));
        size_t next_a = 0, next_b = 0;
        for (size_t phase = 0; phase < config.hop; ++phase, ++sample) {
            if (phase && phase == config.hop / 2) {
                require(!candidate.configure(config) && !reference.configure(original));
            }
            const T input(float(.2 + .5*std::sin(.117*sample)), float(.3*std::cos(.713*sample)),
                float(.00001*std::sin(.817*sample)), 0.f);
            const bool capture = sample % 13 != 0;
            const bool done_a = candidate.process(input, [&](size_t k, T value) {
                require(k == next_a++); a[k] = value;
            }, capture);
            const bool done_b = reference.process(input, [&](size_t k, T value) {
                require(k == next_b++); b[k] = value;
            }, capture);
            require(done_a == done_b && done_a == (phase+1 == config.hop));
            if (exact_schedule) require(next_a == next_b);
        }
        require(next_a == config.length/2+1 && next_b == next_a);
        for (size_t k = 0; k < next_a; ++k) {
            require(same(a[k], b[k])); ++total;
        }
        // Cancel a partially written cache, then resume from reset.
        if (config.hop > 1 && frame % 7 == 0) {
            candidate.reset(); reference.reset();
            require(candidate.configure(config) && reference.configure(original));
            for (size_t p = 0; p < config.hop/2; ++p) {
                candidate.process(T(.5), [](size_t, T) {});
                reference.process(T(.5), [](size_t, T) {});
            }
            candidate.reset(); reference.reset();
        }
    }
    return total;
}
int main(int argc, char**) {
    std::cout << 4*check<rack::simd::float_4>(argc == 1)
        << " published bins exactly matched; publication, rejection, reset and freeze checks passed\n";
}
