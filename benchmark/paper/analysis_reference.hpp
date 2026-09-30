// Independent, untimed analysis reference with explicit interval arithmetic.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_ANALYSIS_REFERENCE_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_ANALYSIS_REFERENCE_HPP_
#include <utility>
#include "protocol.hpp"
#include "references.hpp"
#include "../../src/dsp/window.hpp"

namespace Paper {
/// @brief Preserve the binary32 production/legacy or binary64 native interval contract.
/// @details Window/input multiplication remains in the tested scalar precision;
/// the FFT, direct band sums and EMA are independently evaluated in wider
/// arithmetic. Reusing declared float window coefficients matches workload
/// bytes; it does not use the tested FFT, reconstruction or smoothing as oracle.
template<typename T>
struct AnalysisReference {
    Config config;
    struct Transform {
        std::vector<std::complex<double>> coefficients;
    } forward;
    Fourier::Window::CachedWindow<float> window;
    std::vector<std::complex<double>> frame;
    std::vector<T> expected;
    size_t next_endpoint = 0;
    bool float_intervals;
    explicit AnalysisReference(const Config& c) : config(c),
        window(Fourier::Window::Function::Hann, c.n, false, true),
        frame(c.n), expected(c.n/2+1),
        float_intervals(c.backend.find("core-") == 0 || c.backend.find("legacy-") == 0) {}

    template<typename Arithmetic>
    std::pair<size_t, size_t> band(size_t k) const {
        const float half = std::pow(2.f, (1.f/3.f)/2.f), ratio = std::pow(2.f, 1.f/3.f);
        const Arithmetic width = Arithmetic(config.rate)/Arithmetic(config.n);
        const Arithmetic maximum = Arithmetic(config.rate)/Arithmetic(2);
        Arithmetic low = Arithmetic(k)*width/half, high = Arithmetic(k)*width*half;
        if (high > maximum) { high = maximum; low = high/ratio; }
        return {size_t(std::floor(low/width)), std::min(config.n/2, size_t(std::floor(high/width)))};
    }
    void advance(const std::vector<float>& input, size_t endpoint) {
        require(!input.empty() && endpoint%config.hop == 0, "Invalid independent frame endpoint");
        for (; next_endpoint <= endpoint; next_endpoint += config.hop) {
            const size_t index = next_endpoint/config.hop;
            const bool live = config.state == "live";
            window.set_window(live && index%2 == 0 ? Fourier::Window::Function::BlackmanHarris
                : Fourier::Window::Function::Hann, config.n, false, true);
            for (size_t i = 0; i < config.n; ++i) {
                const int64_t source = int64_t(next_endpoint)-int64_t(config.n)+1+int64_t(i);
                frame[i] = source < 0 ? T(0) : T(input[size_t(source)%input.size()])*window.get_samples()[i];
            }
            std::vector<long double> magnitudes(expected.size());
            if (config.n <= 256) {
                const std::vector<Reference::Complex> precise(frame.begin(), frame.end());
                for (size_t k = 0; k < expected.size(); ++k)
                    magnitudes[k] = std::abs(Reference::coefficient(precise, k, false));
            } else {
                forward.coefficients = Reference::independent_fft(frame);
                for (size_t k = 0; k < expected.size(); ++k)
                    magnitudes[k] = std::abs(forward.coefficients[k]);
            }
            const bool bands = live ? index%2 == 1 : config.smooth;
            const float alpha = config.smooth ? 0.8f : 0.f;
            for (size_t k = 0; k < expected.size(); ++k) {
                long double value = magnitudes[k];
                if (bands) {
                    const auto interval = float_intervals ? band<float>(k) : band<double>(k);
                    value = 0;
                    for (size_t j = interval.first; j <= interval.second; ++j) value += magnitudes[j];
                    value /= interval.second-interval.first+1;
                }
                expected[k] = T(alpha*expected[k]+(1.f-alpha)*value);
            }
        }
    }
};
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_ANALYSIS_REFERENCE_HPP_
