// Regressions for analysis reference precision and non-finite external output.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../benchmark/paper/external.hpp"
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
    // A strong tone amplifies binary32 FFT error in a weak, distant bin.
    // The oracle must retain rounded float frame bytes but compute in double.
    Paper::Config reference_config{};
    reference_config.n = 16384; reference_config.hop = 1024;
    reference_config.rate = 48000; reference_config.state = "steady";
    Paper::AnalysisReference<float> reference(reference_config);
    reference.advance(Paper::signal(), 15360);
    const std::vector<Paper::Reference::Complex> frame(reference.frame.begin(), reference.frame.end());
    const double direct = std::abs(Paper::Reference::coefficient(frame, 4040, false));
    Paper::require(std::abs(double(reference.expected[4040])-direct) < 1e-6,
        "Large analysis oracle loses weak-bin precision");
    Paper::Config c{};
    c.backend = "pffft-fft-float"; c.n = 128; c.callbacks = 1;
    try { Paper::external_transform<float, BrokenProvider>(c); }
    catch (const std::runtime_error& error) {
        return std::string(error.what()) == "Non-finite external transform output" ? 0 : 1;
    }
    return 1;
}
