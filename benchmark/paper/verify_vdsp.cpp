// Independent all-output validation for the benchmark-only Apple adapter.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "providers/vdsp.hpp"
#include "references.hpp"
#include "resources.hpp"
#include <iostream>

template<typename T>
void small_fixtures() {
    using namespace Paper;
    for (const std::string operation : {"rfft", "fft", "ifft"}) {
        VdspBackend<T> adapter(128, operation);
        Reference::transforms<T>([&](const std::vector<std::complex<T>>& input) {
            std::vector<std::complex<T>> output(input.size());
            if (operation == "rfft") {
                std::vector<T> real(input.size());
                for (size_t j = 0; j < real.size(); ++j) real[j] = input[j].real();
                adapter.forward_real(real.data(), output.data());
            } else if (operation == "ifft") adapter.inverse_complex(input.data(), output.data());
            else adapter.forward_complex(input.data(), output.data());
            return output;
        }, operation == "rfft", operation == "ifft");
    }
}

/// @brief Analytic, all-bin fixtures at every supported length; no round-trip oracle.
template<typename T>
void all_lengths() {
    using namespace Paper;
    for (size_t n = 128; n <= 16384; n *= 2) {
        VdspBackend<T> real(n, "analysis"), complex(n, "identity");
        std::vector<T> input(n);
        std::vector<std::complex<T>> complex_input(n), output(n), positive(n/2+2);
        std::vector<Reference::Complex> expected(n);
        const auto sentinel = std::complex<T>(T(713), T(-91));
        input[3] = T(1);
        complex_input[3] = {T(1), T(-0.25)};
        real.forward_real(input.data(), output.data());
        for (size_t k = 0; k < n; ++k) {
            const long double phase = -2*std::acos(-1.L)*3*k/n;
            expected[k] = {std::cos(phase), std::sin(phase)};
        }
        Reference::compare<T>(output, expected);
        positive.back() = sentinel;
        real.forward_real_positive(input.data(), positive.data());
        Reference::check(positive.back() == sentinel);
        for (size_t k = 0; k <= n/2; ++k) Reference::check(positive[k] == output[k]);
        complex.forward_complex(complex_input.data(), output.data());
        for (auto& value : expected) value *= Reference::Complex(1, -0.25L);
        Reference::compare<T>(output, expected);

        // A non-Hermitian sparse spectrum validates normalized complex inverse.
        std::fill(complex_input.begin(), complex_input.end(), std::complex<T>(0));
        complex_input[7] = {T(n)*T(0.5), T(n)*T(-0.25)};
        complex_input[n-3] = {T(n)*T(-0.125), T(n)*T(0.0625)};
        complex.inverse_complex(complex_input.data(), output.data());
        for (size_t j = 0; j < n; ++j) {
            const long double phase = 2*std::acos(-1.L)*j/n;
            expected[j] = Reference::Complex(0.5L, -0.25L)
                    *Reference::Complex(std::cos(7*phase), std::sin(7*phase))
                +Reference::Complex(-0.125L, 0.0625L)
                    *Reference::Complex(std::cos(-3*phase), std::sin(-3*phase));
        }
        Reference::compare<T>(output, expected);

        // Repeat calls after preparation under the C++ allocation interposer.
        // This intentionally makes no assertion about native malloc or stack.
        PaperResources::reset_phase();
        PaperResources::active = true;
        for (size_t repetition = 0; repetition < 8; ++repetition) {
            real.forward_real(input.data(), output.data());
            real.forward_real_positive(input.data(), positive.data());
            complex.forward_complex(complex_input.data(), output.data());
            complex.inverse_complex(complex_input.data(), output.data());
        }
        PaperResources::active = false;
        Reference::check(PaperResources::counts.allocations == 0);
    }
}

int main() {
    try {
        small_fixtures<float>();
        small_fixtures<double>();
        all_lengths<float>();
        all_lengths<double>();
        for (size_t n : {0u, 1u, 127u, 129u, 32768u}) {
            bool rejected = false;
            try { Paper::VdspBackend<float> invalid(n, "fft"); }
            catch (const std::invalid_argument&) { rejected = true; }
            Paper::Reference::check(rejected);
        }
        bool rejected = false;
        try { Paper::VdspBackend<float> invalid(128, "unsupported"); }
        catch (const std::invalid_argument&) { rejected = true; }
        Paper::Reference::check(rejected);
        Paper::VdspBackend<float> info(128, "rfft");
        std::cout << info.info_json() << '\n';
        std::cout << "vDSP references passed: both precisions, all lengths, "
            << "real/complex/inverse, packing, normalization, positive boundary, "
            << "and no observed C++ execution allocations\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
