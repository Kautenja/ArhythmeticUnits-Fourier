// Independent numerical and native-boundary checks for the optional FFTW adapter.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fftw.hpp"
#include "../references.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

namespace {
using Complex = Paper::Reference::Complex;

/// @brief Separate recursive complex transform; no FFTW or production FFT calls.
std::vector<Complex> recursive(const std::vector<Complex>& input, bool inverse) {
    const size_t n = input.size();
    if (n == 1) return input;
    std::vector<Complex> even(n/2), odd(n/2), output(n);
    for (size_t i = 0; i < n/2; ++i) {
        even[i] = input[2*i];
        odd[i] = input[2*i+1];
    }
    even = recursive(even, inverse);
    odd = recursive(odd, inverse);
    for (size_t k = 0; k < n/2; ++k) {
        const long double angle = (inverse ? 2 : -2)*std::acos(-1.L)*k/n;
        const auto rotated = Complex(std::cos(angle), std::sin(angle))*odd[k];
        output[k] = even[k]+rotated;
        output[k+n/2] = even[k]-rotated;
    }
    return output;
}

template<typename T>
std::vector<std::complex<T>> execute(Paper::FftwBackend<T>& backend,
        const std::vector<std::complex<T>>& input, bool real, bool inverse) {
    const auto original = input;
    std::vector<std::complex<T>> output(input.size(), {T(NAN), T(NAN)});
    if (real) {
        std::vector<T> real_input(input.size());
        for (size_t i = 0; i < input.size(); ++i) real_input[i] = input[i].real();
        backend.forward_real(real_input.data(), output.data());
        const size_t bins = input.size()/2+1;
        std::vector<std::complex<T>> positive(bins+2, {T(17), T(-23)});
        backend.forward_real_positive(real_input.data(), positive.data());
        for (size_t k = 0; k < bins; ++k) Paper::Reference::check(positive[k] == output[k]);
        for (size_t k = bins; k < bins+2; ++k)
            Paper::Reference::check(positive[k] == std::complex<T>(17, -23));
        for (size_t i = 0; i < input.size(); ++i)
            Paper::Reference::check(real_input[i] == input[i].real());
    } else if (inverse) backend.inverse_complex(input.data(), output.data());
    else backend.forward_complex(input.data(), output.data());
    Paper::Reference::check(original == input);
    return output;
}

struct Evidence {
    double max_abs_error = 0;
    double max_scaled_error = 0;
    size_t checked_bins = 0;
};

template<typename T>
Evidence verify() {
    Evidence evidence;
    for (size_t kind = 0; kind < 3; ++kind) {
        const bool real = kind == 0, inverse = kind == 2;
        const std::string operation = real ? "rfft" : inverse ? "ifft" : "fft";
        Paper::FftwBackend<T> small(128, operation);
        Paper::Reference::transforms<T>([&](const std::vector<std::complex<T>>& input) {
            return execute(small, input, real, inverse);
        }, real, inverse);
        evidence.checked_bins += 6*128;
        for (const size_t n : {size_t(2048), size_t(16384)}) {
            Paper::FftwBackend<T> backend(n, operation);
            std::vector<std::complex<T>> input(n);
            uint32_t state = 173;
            for (size_t j = 0; j < n; ++j) {
                state = state*1664525u+1013904223u;
                const T noise = T(double(state)/4294967296.0-0.5);
                const long double angle = 2*std::acos(-1.L)*j/n;
                input[j] = {T(noise+0.2L*std::cos(7.25L*angle)),
                    real ? T(0) : T(0.3L*std::sin(19.5L*angle)-noise*0.25L)};
            }
            const std::vector<Complex> canonical(input.begin(), input.end());
            auto expected = recursive(canonical, inverse);
            if (inverse) for (auto& value : expected) value /= static_cast<long double>(n);
            const auto output = execute(backend, input, real, inverse);
            Paper::Reference::compare<T>(output, expected);
            long double scale = 1;
            for (const auto& value : expected) scale = std::max(scale, std::abs(value));
            for (size_t k = 0; k < n; ++k) {
                const double error = double(std::abs(Complex(output[k])-expected[k]));
                evidence.max_abs_error = std::max(evidence.max_abs_error, error);
                evidence.max_scaled_error = std::max(evidence.max_scaled_error, error/double(scale));
            }
            evidence.checked_bins += n;
            for (const size_t bin : {size_t(0), size_t(1), size_t(7), n/2, n-3, n-1}) {
                const auto direct = Paper::Reference::coefficient(canonical, bin, inverse);
                Paper::Reference::check(std::abs(Complex(output[bin])-direct)
                    <= Paper::Reference::tolerance<T>()*scale);
            }
        }
    }
    // Alias-safe complex transport and independent inverse normalization.
    Paper::FftwBackend<T> chain(128, "identity");
    std::vector<std::complex<T>> impulse(128);
    impulse[3] = {T(0.75), T(-0.25)};
    const auto original = impulse;
    chain.forward_complex(impulse.data(), impulse.data());
    chain.inverse_complex(impulse.data(), impulse.data());
    Paper::Reference::compare<T>(impulse, std::vector<Complex>(original.begin(), original.end()));
    bool rejected = false;
    try { chain.forward_real(nullptr, nullptr); } catch (const std::logic_error&) { rejected = true; }
    Paper::Reference::check(rejected);
    return evidence;
}

template<typename F> void rejects(F make) {
    bool rejected = false;
    try { make(); } catch (const std::invalid_argument&) { rejected = true; }
    Paper::Reference::check(rejected);
}
}  // namespace

int main() {
    try {
        const auto single = verify<float>();
        const auto dual = verify<double>();
        rejects([] { Paper::FftwBackend<float> bad(0, "fft"); });
        rejects([] { Paper::FftwBackend<float> bad(128, "unknown"); });
        rejects([] { Paper::FftwBackend<float> bad(128, "fft", FFTW_PATIENT); });
        Paper::FftwBackend<float> measure(128, "identity");
        Paper::FftwBackend<double> estimate(128, "rfft", FFTW_ESTIMATE);
        std::cout << "{\"checked_bins\":" << single.checked_bins+dual.checked_bins
            << ",\"float_max_abs_error\":" << single.max_abs_error
            << ",\"float_max_scaled_error\":" << single.max_scaled_error
            << ",\"double_max_abs_error\":" << dual.max_abs_error
            << ",\"double_max_scaled_error\":" << dual.max_scaled_error
            << ",\"reference_mantissa_bits\":" << std::numeric_limits<long double>::digits
            << ",\"measure\":" << measure.info_json()
            << ",\"estimate\":" << estimate.info_json() << "}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
