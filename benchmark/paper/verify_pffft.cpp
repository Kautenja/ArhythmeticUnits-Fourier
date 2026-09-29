// Independent numerical and layout checks for the benchmark-only Rack adapter.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iostream>
#include <limits>
#include "providers/pffft.hpp"
#include "references.hpp"

using Paper::PffftBackend;
namespace Reference = Paper::Reference;

int main() {
    for (const bool real : {false, true}) {
        for (const bool inverse : {false, true}) {
            if (real && inverse) continue;
            PffftBackend backend(128, real ? "rfft" : inverse ? "ifft" : "fft");
            Reference::transforms<float>([&](const std::vector<std::complex<float>>& input) {
                std::vector<std::complex<float>> output(input.size(),
                    {std::numeric_limits<float>::quiet_NaN(), 0});
                if (real) {
                    std::vector<float> scalar(input.size());
                    for (size_t i = 0; i < input.size(); ++i) scalar[i] = input[i].real();
                    backend.forward_real(scalar.data(), output.data());
                } else if (inverse) backend.inverse_complex(input.data(), output.data());
                else backend.forward_complex(input.data(), output.data());
                return output;
            }, real, inverse);
        }
    }
    for (size_t n = 128; n <= 16384; n *= 2) {
        PffftBackend complex(n, "identity"), real(n, "analysis");
        std::vector<std::complex<float>> input(n), output(n);
        std::vector<Reference::Complex> expected(n);
        input[3] = {0.5f, -0.25f};
        complex.forward_complex(input.data(), output.data());
        for (size_t k = 0; k < n; ++k) {
            const long double angle = -2*std::acos(-1.L)*3*k/n;
            expected[k] = Reference::Complex(0.5L, -0.25L)
                *Reference::Complex(std::cos(angle), std::sin(angle));
        }
        Reference::compare(output, expected);

        // Dense, non-Hermitian frequency input sampled against a direct inverse DFT.
        for (size_t k = 0; k < n; ++k) input[k] = {float(std::sin(k*1.37)), float(std::cos(k*0.49))};
        const std::vector<Reference::Complex> canonical(input.begin(), input.end());
        complex.inverse_complex(input.data(), output.data());
        for (const size_t j : {size_t(0), size_t(1), size_t(7), n/2, n-3, n-1}) {
            const auto exact = Reference::coefficient(canonical, j, true);
            Reference::check(std::abs(Reference::Complex(output[j])-exact) < Reference::tolerance<float>());
        }

        // An independent analytic inverse checks every output store and scaling.
        std::fill(input.begin(), input.end(), std::complex<float>(0, 0));
        input[7] = {float(n)*0.5f, float(n)*-0.25f};
        complex.inverse_complex(input.data(), input.data());  // Canonical buffers may alias.
        for (size_t j = 0; j < n; ++j) {
            const long double angle = 2*std::acos(-1.L)*7*j/n;
            expected[j] = Reference::Complex(0.5L, -0.25L)
                *Reference::Complex(std::cos(angle), std::sin(angle));
        }
        Reference::compare(input, expected);

        std::vector<float> scalar(n);
        for (size_t j = 0; j < n; ++j)
            scalar[j] = 0.125f+(j%2 ? -0.25f : 0.25f)
                +float(0.5L*std::cos(2*std::acos(-1.L)*7*j/n));
        const std::complex<float> guard(123, -456);
        std::fill(output.begin(), output.end(), guard);
        real.forward_real_positive(scalar.data(), output.data());
        for (size_t k = n/2+1; k < n; ++k) Reference::check(output[k] == guard);
        real.forward_real(scalar.data(), output.data());
        std::fill(expected.begin(), expected.end(), Reference::Complex(0, 0));
        expected[0] = n*0.125L;
        expected[n/2] = n*0.25L;
        expected[7] = expected[n-7] = n*0.25L;
        Reference::compare(output, expected);
    }
    for (const size_t n : {size_t(0), size_t(32), size_t(129), size_t(32768)}) {
        bool rejected = false;
        try { PffftBackend invalid(n, "rfft"); }
        catch (const std::invalid_argument&) { rejected = true; }
        Reference::check(rejected);
    }
    bool rejected = false;
    try { PffftBackend invalid(128, "unknown"); }
    catch (const std::invalid_argument&) { rejected = true; }
    Reference::check(rejected);
    PffftBackend real(128, "rfft"), complex(128, "ifft");
    rejected = false;
    try { real.inverse_complex(nullptr, nullptr); }
    catch (const std::logic_error&) { rejected = true; }
    Reference::check(rejected);
    rejected = false;
    try { complex.forward_real(nullptr, nullptr); }
    catch (const std::logic_error&) { rejected = true; }
    Reference::check(rejected);
    std::cout << real.info_json() << '\n' << complex.info_json() << '\n';
    std::cout << "PFFFT independent fixtures passed\n";
}
