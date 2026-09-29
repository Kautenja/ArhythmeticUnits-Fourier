// Reproducible numerical and scheduling experiments for the Fourier report.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include "dsp/fft.hpp"

namespace {
using Clock = std::chrono::steady_clock;
volatile double checksum = 0;

/// @brief Return the elapsed wall time in nanoseconds.
double elapsed(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::nano>(end - start).count();
}

/// @brief Consume every output bin outside the timed region.
void consume(const std::vector<std::complex<float>>& values) {
    double sum = 0;
    for (const auto& value : values) sum += std::abs(value);
    checksum += sum;
}

/// @brief Deterministic fixtures, with the random sequence reset per frame.
template<typename T>
std::vector<T> signal(size_t n, int fixture) {
    std::vector<T> x(n);
    uint32_t state = 0x12345678;
    for (size_t i = 0; i < n; ++i) {
        state = 1664525u * state + 1013904223u;
        const long double angle = 2 * std::acos(-1.L) * i / n;
        switch (fixture) {
        case 0: x[i] = T(0); break;
        case 1: x[i] = T(i == 0); break;
        case 2: x[i] = T(1); break;
        case 3: x[i] = T(i % 2 ? -1 : 1); break;
        case 4: x[i] = T(std::sin(angle) + 0.25L * std::cos(3 * angle)); break;
        default: x[i] = T(double(state >> 8) / 8388608.0 - 1.0); break;
        }
    }
    return x;
}

/// @brief Check full spectra and every tested incremental completion count.
template<typename T>
void verify(const char* precision) {
    for (size_t n = 4; n <= 4096; n *= 2) {
        double max_scaled_error = 0;
        size_t reference_bins = 0;
        size_t schedules = 0;
        for (int fixture = 0; fixture < 6; ++fixture) {
            const auto x = signal<T>(n, fixture);
            for (int window = 0; window < 2; ++window) {
                std::vector<float> w(n, 1.f);
                if (window)
                    for (size_t i = 0; i < n; ++i)
                        w[i] = float(0.5L - 0.5L * std::cos(2 * std::acos(-1.L) * i / n));
                Fourier::OnTheFlyRFFT<T> full(n);
                full.buffer(x.data(), w);
                full.compute();
                if (n <= 128) {
                    long double scale = 0;
                    for (size_t i = 0; i < n; ++i)
                        scale += std::abs(static_cast<long double>(x[i] * w[i]));
                    scale = std::max(1.L, scale);
                    for (size_t k = 0; k < n; ++k) {
                        std::complex<long double> reference(0, 0);
                        for (size_t i = 0; i < n; ++i) {
                            const long double angle = -2 * std::acos(-1.L) * k * i / n;
                            reference += static_cast<long double>(x[i] * w[i])
                                * std::complex<long double>(std::cos(angle), std::sin(angle));
                        }
                        const auto actual = std::complex<long double>(full.coefficients[k]);
                        const double error = double(std::abs(actual - reference) / scale);
                        max_scaled_error = std::max(max_scaled_error, error);
                        if (error > 64 * std::numeric_limits<T>::epsilon())
                            throw std::runtime_error("Independent DFT comparison failed");
                        ++reference_bins;
                    }
                }
                for (const auto hop : {size_t(1), n / 4, n / 2, n, n + 7}) {
                    Fourier::OnTheFlyRFFT<T> incremental(n);
                    incremental.buffer(x.data(), w);
                    const size_t b = incremental.get_total_steps();
                    const size_t q = b / hop + (b % hop != 0);
                    const size_t expected = b / q + (b % q != 0);
                    size_t calls = 0;
                    while (!incremental.is_done_computing() && calls <= hop) {
                        incremental.step(hop);
                        ++calls;
                    }
                    if (calls != expected || incremental.coefficients != full.coefficients)
                        throw std::runtime_error("Incremental scheduling/equivalence failed");
                    ++schedules;
                }
            }
        }
        std::cout << precision << ',' << n << ',' << schedules << ','
                  << reference_bins << ',' << max_scaled_error << '\n';
    }
}

/// @brief Measure one mode; frame cadence stays fixed at H in both modes.
void measure(size_t n, size_t hop, size_t repetition, bool incremental) {
    const size_t frames = 64;
    const auto x = signal<float>(n, 5);
    std::vector<float> w(n);
    for (size_t i = 0; i < n; ++i)
        w[i] = float(0.5 - 0.5 * std::cos(2 * std::acos(-1.0) * i / n));
    Fourier::OnTheFlyRFFT<float> fft(n);
    for (size_t frame = 0; frame < 16; ++frame) {
        fft.buffer(x.data(), w);
        fft.compute();
        consume(fft.coefficients);
    }
    // Aggregate pass: no clock reads inside the H-call frame loop.
    double aggregate_ns = 0;
    for (size_t frame = 0; frame < frames; ++frame) {
        const auto start = Clock::now();
        fft.buffer(x.data(), w);
        if (incremental) {
            for (size_t i = 0; i < hop; ++i) fft.step(hop);
        } else {
            fft.compute();
            // The baseline has no FFT work in the remaining H-1 calls.
        }
        const auto end = Clock::now();
        aggregate_ns += elapsed(start, end);
        consume(fft.coefficients);
    }
    // Instrumented pass: measure every simulated sample call, including
    // buffer preparation in call zero and reconstruction on completion.
    std::vector<double> calls;
    std::vector<double> starts;
    std::vector<double> finishes;
    calls.reserve(frames * hop);
    starts.reserve(frames);
    finishes.reserve(frames);
    for (size_t frame = 0; frame < frames; ++frame) {
        bool done = false;
        for (size_t i = 0; i < hop; ++i) {
            const auto start = Clock::now();
            if (i == 0) fft.buffer(x.data(), w);
            if (incremental) fft.step(hop);
            else if (i == 0) fft.compute();
            const auto end = Clock::now();
            const double ns = elapsed(start, end);
            calls.push_back(ns);
            if (i == 0) starts.push_back(ns);
            if (!done && fft.is_done_computing()) {
                finishes.push_back(ns);
                done = true;
            }
        }
        if (!done) throw std::runtime_error("Missed operation-count deadline");
        consume(fft.coefficients);
    }
    const auto quantile = [](std::vector<double>& data, double fraction) {
        std::sort(data.begin(), data.end());
        return data[static_cast<size_t>(std::ceil(fraction * data.size())) - 1];
    };
    const double p99 = quantile(calls, 0.99);
    const double start_median = quantile(starts, 0.5);
    const double finish_median = quantile(finishes, 0.5);
    std::cout << n << ',' << hop << ',' << repetition << ','
              << (incremental ? "incremental" : "complete") << ',' << frames << ','
              << aggregate_ns / frames << ',' << p99 << ',' << calls.back() << ','
              << start_median << ',' << finish_median << '\n';
}
}  // namespace

int main(int argc, char** argv) {
    std::cout << std::setprecision(12);
    if (argc != 2) return 2;
    const std::string mode(argv[1]);
    if (mode == "clock") {
        std::vector<double> samples;
        samples.reserve(10000);
        for (size_t i = 0; i < 10000; ++i) {
            const auto start = Clock::now();
            const auto end = Clock::now();
            samples.push_back(elapsed(start, end));
        }
        std::sort(samples.begin(), samples.end());
        std::cout << "{\"empty_timer_median_ns\":" << samples[4999]
                  << ",\"empty_timer_p99_ns\":" << samples[9899]
                  << ",\"long_double_digits\":" << std::numeric_limits<long double>::digits
                  << "}\n";
    } else if (mode == "verify") {
        std::cout << "precision,n,schedules,reference_bins,max_scaled_error\n";
        verify<float>("float");
        verify<double>("double");
    } else if (mode == "measure") {
        std::cout << "n,hop,repetition,mode,frames,frame_ns,call_p99_ns,call_max_ns,start_median_ns,finish_median_ns\n";
        for (size_t repetition = 0; repetition < 9; ++repetition)
            for (const size_t n : {1024, 2048, 4096, 16384})
                for (const size_t hop : {n / 4, n / 2})
                    for (size_t order = 0; order < 2; ++order)
                        measure(n, hop, repetition, (order + repetition) % 2 == 1);
    } else {
        return 2;
    }
}
