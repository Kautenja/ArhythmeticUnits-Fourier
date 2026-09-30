// Independent canonical references shared by benchmark adapters.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_REFERENCES_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_REFERENCES_HPP_
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace Paper {
namespace Reference {
using Complex = std::complex<long double>;
inline void check(bool valid) {
    if (!valid) throw std::runtime_error("Independent canonical reference differs");
}
template<typename T> double tolerance() { return sizeof(T) == 4 ? 2e-5 : 1e-10; }

/// @brief Reject non-finite errors before a maximum can silently discard NaN.
inline long double absolute_error(const Complex& actual, const Complex& expected) {
    const long double error = std::abs(actual-expected);
    check(std::isfinite(error));
    return error;
}

/// @brief Direct DFT, natural order, negative forward sign and 1/N inverse.
inline Complex coefficient(const std::vector<Complex>& input, size_t bin, bool inverse) {
    Complex result(0, 0);
    for (size_t j = 0; j < input.size(); ++j) {
        const long double angle = (inverse ? 2 : -2)*std::acos(-1.L)*bin*j/input.size();
        result += input[j]*Complex(std::cos(angle), std::sin(angle));
    }
    return inverse ? result/static_cast<long double>(input.size()) : result;
}

template<typename T>
void compare(const std::vector<std::complex<T>>& output, const std::vector<Complex>& expected) {
    check(output.size() == expected.size());
    long double scale = 1;
    for (const auto& value : expected) scale = std::max(scale, std::abs(value));
    for (size_t k = 0; k < output.size(); ++k) {
        const auto error = std::abs(Complex(output[k])-expected[k]);
        check(std::isfinite(error) && error <= tolerance<T>()*scale);
    }
}

/// @brief Adapter receives canonical complex input and returns all natural-order bins.
/// @details Silence, shifted impulse, DC, Nyquist, off-bin tone and dense complex
/// fixtures detect missing stores, packing/permutation, sign and scale errors.
template<typename T, typename Execute>
void transforms(Execute execute, bool real, bool inverse) {
    const size_t n = 128;
    for (size_t fixture = 0; fixture < 6; ++fixture) {
        std::vector<std::complex<T>> input(n);
        for (size_t j = 0; j < n; ++j) {
            const long double angle = 2*std::acos(-1.L)*j/n;
            if (fixture == 1 && j == 3) input[j] = T(1);
            if (fixture == 2) input[j] = T(0.25);
            if (fixture == 3) input[j] = T(j%2 ? -0.5 : 0.5);
            if (fixture == 4) input[j] = {T(std::cos(7.25L*angle)), real ? T(0) : T(std::sin(7.25L*angle))};
            if (fixture == 5) input[j] = {T(std::sin(j*1.73L)), real ? T(0) : T(std::cos(j*0.37L))};
        }
        std::vector<Complex> canonical(input.begin(), input.end()), expected(n);
        for (size_t k = 0; k < n; ++k) expected[k] = coefficient(canonical, k, inverse);
        compare<T>(execute(input), expected);
    }
}

/// @brief Independent recursive radix-2 DFT; no production transform, packing or twiddle table.
/// @details Binary64 complex arithmetic; each twiddle is evaluated directly,
/// avoiding recursive twiddle drift. The small direct DFT fixtures validate this
/// scalable oracle before it is used for every bin of large analysis frames.
inline void independent_fft_recursive(const std::complex<double>* input, size_t stride,
        std::complex<double>* output, size_t n) {
    if (n == 1) { output[0] = *input; return; }
    const size_t half = n/2;
    independent_fft_recursive(input, stride*2, output, half);
    independent_fft_recursive(input+stride, stride*2, output+half, half);
    for (size_t k = 0; k < half; ++k) {
        const double angle = -2*std::acos(-1.)*k/n;
        const std::complex<double> odd = output[k+half]
            *std::complex<double>(std::cos(angle), std::sin(angle));
        const auto even = output[k];
        output[k] = even+odd;
        output[k+half] = even-odd;
    }
}
inline std::vector<std::complex<double>> independent_fft(
        const std::vector<std::complex<double>>& input) {
    check(!input.empty() && !(input.size() & (input.size()-1)));
    std::vector<std::complex<double>> output(input.size());
    independent_fft_recursive(input.data(), 1, output.data(), input.size());
    return output;
}

/// @brief Periodic coherent-gain-corrected windows, independent of production caches.
inline long double window(size_t i, size_t n, bool blackman_harris) {
    const long double angle = 2*std::acos(-1.L)*i/n;
    return blackman_harris ? (0.35875L-0.48829L*std::cos(angle)
        +0.14128L*std::cos(2*angle)-0.01168L*std::cos(3*angle))/0.35875L
        : 1-std::cos(angle);
}

/// @brief Canonical all-bin magnitudes for a zero-padded stream frame ending at endpoint.
inline std::vector<long double> magnitudes(const std::vector<float>& input, int64_t endpoint,
        size_t n, bool blackman_harris) {
    std::vector<Complex> frame(n);
    for (size_t i = 0; i < n; ++i) {
        const int64_t index = endpoint-static_cast<int64_t>(n)+1+static_cast<int64_t>(i);
        if (index >= 0) frame[i] = input[size_t(index)%input.size()]*window(i, n, blackman_harris);
    }
    std::vector<long double> result(n/2+1);
    for (size_t k = 0; k < result.size(); ++k) result[k] = std::abs(coefficient(frame, k, false));
    return result;
}
}  // namespace Reference
}  // namespace Paper
#endif
