// Regression for non-finite external output being hidden by max(error, NaN).
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "external.hpp"
#include <limits>

struct BrokenProvider {
    size_t n;
    BrokenProvider(size_t size, const std::string&) : n(size) {}
    void forward_real(const float*, std::complex<float>* output) { corrupt(output); }
    void forward_complex(const std::complex<float>*, std::complex<float>* output) { corrupt(output); }
    void inverse_complex(const std::complex<float>*, std::complex<float>* output) { corrupt(output); }
    void corrupt(std::complex<float>* output) {
        for (size_t i = 0; i < n; ++i) output[i] = {std::numeric_limits<float>::quiet_NaN(), 0};
    }
    std::string info_json() const { return "{}"; }
};
int main() {
    Paper::Config c{};
    c.backend = "pffft-fft-float"; c.n = 128; c.callbacks = 1;
    try { Paper::external_transform<float, BrokenProvider>(c); }
    catch (const std::runtime_error& error) {
        return std::string(error.what()) == "Non-finite external transform output" ? 0 : 1;
    }
    return 1;
}
