// Supplemental exact module workload: display geometry controls and log scales.
// Copyright 2026 Arhythmetic Units; SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
#include "src/SpectrumAnalyzer.cpp"
Plugin* plugin_instance = nullptr;
int main() {
    std::fprintf(stderr, "SpectrumAnalyzer bytes: %zu\n", sizeof(SpectrumAnalyzer));
    rack::Context context;
    rack::contextSet(&context);
    context.engine = new rack::engine::Engine;
    context.engine->setSampleRate(48000.f);
    std::vector<float> signal(65536);
    for (size_t i = 0; i < signal.size(); ++i)
        signal[i] = std::sin(.17f*i) + .3f*std::cos(.317f*i);
    std::puts("n,scale,live,repeat,ns_per_sample,p99_ns,max_ns");
    for (size_t n : {2048u, 16384u}) for (size_t scale = 0; scale < 3; ++scale)
    for (int live : {0, 1}) for (size_t repeat = 0; repeat < 5; ++repeat) {
        SpectrumAnalyzer module;
        module.set_window_length(n);
        module.set_hop_length(1440);
        module.set_window_function(Fourier::Window::Function::Flattop);
        module.set_magnitude_scale(static_cast<MagnitudeScale>(scale));
        module.set_time_smoothing(0.f);
        module.set_frequency_smoothing(FrequencySmoothing::None);
        for (auto& input : module.inputs) input.channels = 1;
        rack::engine::Module::ProcessArgs args = {};
        args.sampleRate = 48000.f; args.sampleTime = 1.f/48000.f;
        size_t tick = 0;
        const auto process = [&]() {
            if (tick % 1440 == 0) {
                const bool alternate = live && (tick/1440)%2;
                module.set_slope(alternate ? -4.5f : 4.5f);
                module.set_low_frequency(alternate ? 100.f : 0.f);
                module.set_high_frequency(alternate ? 10000.f : 20000.f);
                module.set_frequency_scale(alternate ? FrequencyScale::Logarithmic : FrequencyScale::Linear);
            }
            for (size_t lane = 0; lane < 4; ++lane)
                module.inputs[lane].setVoltage(signal[(tick+113*lane)%signal.size()]);
            module.process(args);
            ++tick; ++args.frame;
        };
        for (size_t i = 0; i < 2*n+64*1440; ++i) process();
        const size_t block = 256;
        std::vector<double> observations(512*1440/block);
        for (auto& duration : observations) {
            const auto start = std::chrono::steady_clock::now();
            for (size_t i = 0; i < block; ++i) process();
            const auto stop = std::chrono::steady_clock::now();
            duration = std::chrono::duration<double,std::nano>(stop-start).count();
        }
        double sum = 0;
        for (double value : observations) sum += value;
        std::sort(observations.begin(), observations.end());
        const auto& output = module.consume_display_spectrum();
        if (output.count != n/2+1 || !std::isfinite(output.points[0][10].x)) return 2;
        std::printf("%zu,%zu,%d,%zu,%.9f,%.9f,%.9f\n",n,scale,live,repeat,
            sum/(block*observations.size()),observations[size_t(.99*(observations.size()-1))],observations.back());
    }
}
