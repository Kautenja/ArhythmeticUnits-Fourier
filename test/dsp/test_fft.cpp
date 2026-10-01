// Test cases for the Fast Fourier Transform (FFT.)
//
// Copyright (c) 2020 Christian Kauten
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>
#include <vector>
#include "dsp/dft.hpp"
// DSP headers must not depend on the nonstandard M_PI macro.
#undef M_PI
#include "dsp/fft.hpp"
#include "../ieee754.hpp"
#include "../functions.hpp"
#include "catch_amalgamated.hpp"

// ---------------------------------------------------------------------------
// MARK: `OnTheFlyFFT`
// ---------------------------------------------------------------------------

SCENARIO("the FFT needs to be calculated") {
    GIVEN("a sequence with no signal (length 1)") {
        std::vector<std::complex<float>> sequence = {0};
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        WHEN("the FFT is calculated") {
            fft.compute();
            THEN("the output has a unit DC coefficient") {
                REQUIRE(epsilon_equal(std::complex<float>(0, -0), fft.coefficients[0]));
            }
        }
    }
    GIVEN("a sequence with no signal (length 2)") {
        std::vector<std::complex<float>> sequence = {0, 0};
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        WHEN("the FFT is calculated") {
            fft.compute();
            THEN("the output has a unit DC coefficient") {
                REQUIRE(epsilon_equal(std::complex<float>(0, -0), fft.coefficients[0]));
                REQUIRE(epsilon_equal(std::complex<float>(0, -0), fft.coefficients[1]));
            }
        }
    }
    GIVEN("a sequence with the unit impulse") {
        std::vector<std::complex<float>> sequence = {1};
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        WHEN("the FFT is calculated") {
            fft.compute();
            THEN("the output has a unit DC coefficient") {
                REQUIRE(epsilon_equal(std::complex<float>(1, -0), fft.coefficients[0]));
            }
        }
    }
    GIVEN("a sequence with the unit impulse (length 2)") {
        std::vector<std::complex<float>> sequence = {1, 0};
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        WHEN("the FFT is calculated") {
            fft.compute();
            THEN("the output has a unit DC coefficient") {
                REQUIRE(approx_equal(fft.coefficients[0], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(approx_equal(fft.coefficients[1], std::complex<float>(1, -0), 1e-6f));
            }
        }
    }
    GIVEN("a sequence with the unit impulse (length 4)") {
        std::vector<std::complex<float>> sequence = {1, 0, 0, 0};
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        WHEN("the FFT is calculated") {
            fft.compute();
            THEN("the output has a unit DC coefficient") {
                REQUIRE(approx_equal(fft.coefficients[0], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(approx_equal(fft.coefficients[1], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(approx_equal(fft.coefficients[2], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(approx_equal(fft.coefficients[3], std::complex<float>(1, -0), 1e-6f));
            }
        }
    }
    GIVEN("a sequence with a sinusoid at 441Hz, a sample rate of 44100Hz, and 4096 frequency bins") {
        const float FUNDAMENTAL = 441;
        const float SAMPLE_RATE = 44100;
        constexpr int FFT_BINS = 4096;
        const auto sequence = generate_sinusoid<std::complex<float>>(FUNDAMENTAL, SAMPLE_RATE, FFT_BINS);
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        WHEN("the FFT is calculated.") {
            fft.compute();
            THEN("the output has a spike at the fundamental frequency") {
                // Remove the symmetric copy of the FFT coefficients above the
                // Nyquist rate (i.e., sample rate / 2)
                fft.coefficients.erase(fft.coefficients.begin() + fft.coefficients.size() / 2, fft.coefficients.end());
                // Transform the coefficients to decibels
                auto output_dB = amplitude2decibels(fft.coefficients);
                // Locate the coefficient with the greatest magnitude in decibels
                auto highest_bin = argmax(output_dB.data(), output_dB.size());
                // Convert the coefficient to Hz.
                auto frequency = float(highest_bin) * SAMPLE_RATE / FFT_BINS;
                // Frequency should be accurate for integral values
                REQUIRE(approx_equal<float>(FUNDAMENTAL, frequency, 1));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: `OnTheFlyRFFT`
// ---------------------------------------------------------------------------

SCENARIO("the RFFT needs to be calculated") {
    GIVEN("a sequence with the unit impulse (length 4)") {
        std::vector<float> sequence = {1, 0, 0, 0};
        std::vector<float> window(sequence.size(), 1.f);
        Fourier::OnTheFlyRFFT<float> fft(sequence.size());
        fft.buffer(sequence.data(), window);
        WHEN("the FFT is calculated") {
            fft.compute();
            THEN("the output has a unit DC coefficient") {
                REQUIRE(approx_equal(fft.coefficients[0], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(approx_equal(fft.coefficients[1], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(approx_equal(fft.coefficients[2], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(approx_equal(fft.coefficients[3], std::complex<float>(1, -0), 1e-6f));
            }
        }
    }
    GIVEN("a sequence with a sinusoid at 441Hz, a sample rate of 44100Hz, and 4096 frequency bins") {
        const float FUNDAMENTAL = 441;
        const float SAMPLE_RATE = 44100;
        constexpr int FFT_BINS = 4096;
        const auto sequence = generate_sinusoid<float>(FUNDAMENTAL, SAMPLE_RATE, FFT_BINS);
        std::vector<float> window(sequence.size(), 1.f);
        Fourier::OnTheFlyRFFT<float> fft(sequence.size());
        fft.buffer(sequence.data(), window);
        WHEN("the FFT is calculated.") {
            fft.compute();
            THEN("the output has a spike at the fundamental frequency") {
                // Remove the symmetric copy of the FFT coefficients above the
                // Nyquist rate (i.e., sample rate / 2)
                fft.coefficients.erase(fft.coefficients.begin() + fft.coefficients.size() / 2, fft.coefficients.end());
                // Transform the coefficients to decibels
                auto output_dB = amplitude2decibels(fft.coefficients);
                // Locate the coefficient with the greatest magnitude in decibels
                auto highest_bin = argmax(output_dB.data(), output_dB.size());
                // Convert the coefficient to Hz.
                auto frequency = float(highest_bin) * SAMPLE_RATE / FFT_BINS;
                // Frequency should be accurate for integral values
                REQUIRE(approx_equal<float>(FUNDAMENTAL, frequency, 1));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: RFFT storage reuse
// ---------------------------------------------------------------------------

TEMPLATE_TEST_CASE("RFFT reuse preserves windowing and smoothing", "[rfft]", float, double) {
    using T = TestType;
    Fourier::OnTheFlyRFFT<T> fft(4);
    const size_t hop = GENERATE(1, 3, 64);
    const float fraction = GENERATE(1.f, 1.f / 3.f, 1.f / 12.f);
    for (const size_t n : {4, 32, 8, 64, 4}) {
        fft.resize(n);
        std::vector<T> input(n);
        std::vector<float> window(n);
        for (size_t frame = 0; frame < 4; ++frame) {
            CAPTURE(n, hop, fraction, frame);
            for (size_t i = 0; i < n; ++i) {
                // Follow changing signals with silence to detect stale scratch.
                input[i] = frame == 3 ? T(0) : T(int((i + frame) % 7) - 3) / T(4);
                window[i] = frame % 2 == 0 ? 1.f : float(i % 4 + 1) / 4.f;
            }
            fft.buffer(input.data(), window);
            const size_t budget = (fft.get_total_steps() + hop - 1) / hop;
            const size_t calls = (fft.get_total_steps() + budget - 1) / budget;
            for (size_t call = 0; call < calls; ++call) {
                REQUIRE_FALSE(fft.is_done_computing());
                fft.step(hop);
            }
            REQUIRE(fft.is_done_computing());
            // Independent direct DFT checks packing and windowing after reuse.
            const long double tolerance = 32.L * n * std::numeric_limits<T>::epsilon();
            for (size_t k = 0; k < n; ++k) {
                std::complex<long double> expected(0, 0);
                for (size_t i = 0; i < n; ++i) {
                    const long double angle = -2.L * std::acos(-1.L) * k * i / n;
                    expected += static_cast<long double>(input[i]) * window[i]
                        * std::complex<long double>(std::cos(angle), std::sin(angle));
                }
                const std::complex<long double> actual(fft.coefficients[k].real(),
                                                       fft.coefficients[k].imag());
                REQUIRE(std::abs(actual - expected) <= tolerance);
            }
            const auto original = fft.coefficients;
            fft.smooth(48000.f, fraction);
            // Sum each band directly rather than using a prefix-sum buffer.
            const float bin_width = 48000.f / n;
            for (size_t k = 0; k < n; ++k) {
                T expected = 0;
                if (k <= n / 2) {
                    const float center = k * bin_width;
                    float low = center / std::pow(2.f, fraction / 2.f);
                    float high = center * std::pow(2.f, fraction / 2.f);
                    if (high > 24000.f) {
                        high = 24000.f;
                        low = high / std::pow(2.f, fraction);
                    }
                    const size_t first = std::floor(low / bin_width);
                    const size_t last = std::floor(high / bin_width);
                    for (size_t bin = first; bin <= last; ++bin)
                        expected += std::abs(original[bin]);
                    expected /= T(last - first + 1);
                }
                REQUIRE(std::abs(fft.coefficients[k].real() - expected) <= tolerance);
                REQUIRE(fft.coefficients[k].imag() == T(0));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: `OnTheFlyIFFT`
// ---------------------------------------------------------------------------

SCENARIO("the IFFT needs to be calculated") {
    GIVEN("a sequence with no signal (length 2)") {
        std::vector<std::complex<float>> sequence = {0, 0};
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        Fourier::OnTheFlyIFFT<float> ifft(sequence.size());
        fft.buffer(sequence.data());
        fft.compute();
        ifft.buffer(fft.coefficients.data());
        WHEN("the IFFT is calculated") {
            ifft.compute();
            THEN("the output matches the input") {
                for (size_t i = 0; i < sequence.size(); i++)
                    REQUIRE(epsilon_equal(sequence[i], ifft.coefficients[i]));
            }
        }
    }
    GIVEN("a sequence with the unit impulse (length 2)") {
        std::vector<std::complex<float>> sequence = {1, 0};
        // Compute the FFT
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        fft.compute();
        // Compute the IFFT
        Fourier::OnTheFlyIFFT<float> ifft(sequence.size());
        ifft.buffer(fft.coefficients.data());
        WHEN("the IFFT is calculated") {
            ifft.compute();
            THEN("the output matches the input") {
                for (size_t i = 0; i < sequence.size(); i++)
                    REQUIRE(epsilon_equal(sequence[i], ifft.coefficients[i]));
            }
        }
    }
    GIVEN("a sequence with the unit impulse (length 4)") {
        std::vector<std::complex<float>> sequence = {1, 0, 0, 0};
        // Compute the FFT
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        fft.compute();
        // Compute the IFFT
        Fourier::OnTheFlyIFFT<float> ifft(sequence.size());
        ifft.buffer(fft.coefficients.data());
        WHEN("the IFFT is calculated") {
            ifft.compute();
            THEN("the output matches the input") {
                for (size_t i = 0; i < sequence.size(); i++)
                    REQUIRE(epsilon_equal(sequence[i], ifft.coefficients[i]));
            }
        }
    }
    GIVEN("a sequence with a sinusoid at 441Hz, a sample rate of 44100Hz, and 4096 frequency bins") {
        const float FUNDAMENTAL = 441;
        const float SAMPLE_RATE = 44100;
        constexpr int FFT_BINS = 4096;
        const auto sequence = generate_sinusoid<std::complex<float>>(FUNDAMENTAL, SAMPLE_RATE, FFT_BINS);
        // Compute the FFT
        Fourier::OnTheFlyFFT<float> fft(sequence.size());
        fft.buffer(sequence.data());
        fft.compute();
        // Compute the IFFT
        Fourier::OnTheFlyIFFT<float> ifft(sequence.size());
        ifft.buffer(fft.coefficients.data());
        WHEN("the IFFT is calculated.") {
            ifft.compute();
            THEN("the output matches the input") {
                for (size_t i = 0; i < sequence.size(); i++)
                    REQUIRE(approx_equal(sequence[i], ifft.coefficients[i], 1e-6f));
            }
        }
    }
}

TEMPLATE_TEST_CASE("IFFT reconstructs known complex signals", "[ifft]", float, double) {
    using T = TestType;
    using Complex = std::complex<T>;
    const size_t n = GENERATE(1, 2, 4, 8, 32);
    CAPTURE(n);
    std::vector<Complex> spectrum(n, Complex(0, 0));
    std::vector<Complex> expected(n, Complex(0, 0));
    const Complex amplitude(T(0.75), T(-0.5));

    SECTION("silence") {}
    SECTION("DC") {
        spectrum[0] = amplitude * T(n);
        std::fill(expected.begin(), expected.end(), amplitude);
    }
    SECTION("impulse") {
        std::fill(spectrum.begin(), spectrum.end(), amplitude);
        expected[0] = amplitude;
    }
    SECTION("positive frequency complex sinusoid") {
        const size_t bin = n == 1 ? 0 : 1;
        spectrum[bin] = amplitude * T(n);
        for (size_t i = 0; i < n; ++i) {
            const T angle = T(2) * Fourier::pi<T>() * T(bin * i) / T(n);
            expected[i] = amplitude * Complex(std::cos(angle), std::sin(angle));
        }
    }
    SECTION("Nyquist") {
        spectrum[n / 2] = amplitude * T(n);
        for (size_t i = 0; i < n; ++i)
            expected[i] = (i % 2 == 0) ? amplitude : -amplitude;
    }

    Fourier::OnTheFlyIFFT<T> ifft(n);
    const auto original = spectrum;
    ifft.buffer(spectrum.data());
    ifft.compute();
    REQUIRE(ifft.is_done_computing());
    REQUIRE(spectrum == original);
    // At most five radix-2 stages, with unit-scale inputs.
    const T tolerance = T(32) * std::numeric_limits<T>::epsilon();
    for (size_t i = 0; i < n; ++i) {
        CAPTURE(i);
        REQUIRE(std::abs(ifft.coefficients[i] - expected[i]) <= tolerance);
    }
}

TEMPLATE_TEST_CASE("IFFT matches the complex inverse DFT", "[ifft]", float, double) {
    using T = TestType;
    using Complex = std::complex<T>;
    const size_t n = GENERATE(2, 4, 8, 16, 64);
    CAPTURE(n);
    std::vector<Complex> spectrum(n);
    for (size_t k = 0; k < n; ++k)
        spectrum[k] = Complex(T(int(k % 7) - 3) / T(4),
                              T(int(k % 5) - 2) / T(3));
    Fourier::OnTheFlyIFFT<T> ifft(n);
    ifft.buffer(spectrum.data());
    ifft.compute();
    // Independent O(N^2) complex reference using long double arithmetic.
    // Fourier::idft returns only the real component and cannot check phase here.
    const long double pi = std::acos(-1.L);
    const long double tolerance = 64.L * std::numeric_limits<T>::epsilon();
    for (size_t i = 0; i < n; ++i) {
        std::complex<long double> expected(0, 0);
        for (size_t k = 0; k < n; ++k) {
            const long double angle = 2.L * pi * k * i / n;
            expected += std::complex<long double>(spectrum[k].real(), spectrum[k].imag())
                * std::complex<long double>(std::cos(angle), std::sin(angle));
        }
        expected /= n;
        const std::complex<long double> actual(ifft.coefficients[i].real(),
                                               ifft.coefficients[i].imag());
        CAPTURE(i);
        REQUIRE(std::abs(actual - expected) <= tolerance);
    }
}

TEMPLATE_TEST_CASE("FFT and IFFT preserve complex samples", "[ifft]", float, double) {
    using T = TestType;
    using Complex = std::complex<T>;
    const size_t n = GENERATE(1, 2, 16, 1024);
    CAPTURE(n);
    std::vector<Complex> input(n);
    for (size_t i = 0; i < n; ++i)
        input[i] = Complex(T(int(i % 11) - 5) / T(8),
                           T(int(i % 7) - 3) / T(4));
    Fourier::OnTheFlyFFT<T> fft(n);
    Fourier::OnTheFlyIFFT<T> ifft(n);
    fft.buffer(input.data());
    fft.compute();
    ifft.buffer(fft.coefficients.data());
    ifft.compute();
    // Allow accumulated rounding through up to ten stages in each direction.
    const T tolerance = T(64) * std::numeric_limits<T>::epsilon();
    for (size_t i = 0; i < n; ++i) {
        CAPTURE(i);
        REQUIRE(std::abs(ifft.coefficients[i] - input[i]) <= tolerance);
    }
}

TEST_CASE("IFFT steps include incremental output normalization", "[ifft]") {
    const size_t n = GENERATE(1, 2, 8, 32);
    CAPTURE(n);
    Fourier::OnTheFlyIFFT<float> ifft(n);
    const std::complex<float> amplitude(2, -3);
    std::vector<std::complex<float>> spectrum(n, 0.f);
    spectrum[0] = amplitude * float(n);
    size_t butterflies = 0;
    for (size_t stage = n; stage > 1; stage >>= 1)
        butterflies += n / 2;
    const size_t total = butterflies + n;
    REQUIRE(ifft.get_total_steps() == total);
    REQUIRE(ifft.is_done_computing());
    ifft.buffer(spectrum.data());
    REQUIRE_FALSE(ifft.is_done_computing());
    ifft.step(0);  // Zero hop must neither advance nor hang.
    for (size_t step = 1; step <= total; ++step) {
        ifft.step();
        CAPTURE(step);
        REQUIRE(ifft.is_done_computing() == (step == total));
        const size_t normalized = step > butterflies ? step - butterflies : 0;
        for (size_t i = 0; i < n; ++i)
            REQUIRE(ifft.coefficients[i] == (i < normalized ? amplitude : 0.f));
    }
    const auto output = ifft.coefficients;
    ifft.step();
    ifft.step(1);
    ifft.compute();
    REQUIRE(ifft.coefficients == output);
}

TEST_CASE("IFFT hop scheduling matches complete computation", "[ifft]") {
    const size_t n = GENERATE(1, 2, 8, 64);
    const size_t hop = GENERATE(size_t(1), size_t(3), size_t(16), size_t(1024),
                               std::numeric_limits<size_t>::max());
    CAPTURE(n, hop);
    std::vector<std::complex<float>> spectrum(n);
    for (size_t i = 0; i < n; ++i)
        spectrum[i] = {float(int(i % 5) - 2), float(int(i % 3) - 1)};
    Fourier::OnTheFlyIFFT<float> complete(n), incremental(n);
    complete.buffer(spectrum.data());
    complete.compute();
    incremental.buffer(spectrum.data());
    // Bound the loop independently of the readiness flag to catch stuck work.
    const size_t total = incremental.get_total_steps();
    const size_t budget = total / hop + (total % hop != 0);
    const size_t calls = total / budget + (total % budget != 0);
    REQUIRE(calls <= hop);
    for (size_t call = 0; call < calls; ++call) {
        REQUIRE_FALSE(incremental.is_done_computing());
        incremental.step(hop);
    }
    REQUIRE(incremental.is_done_computing());
    REQUIRE(incremental.coefficients == complete.coefficients);
}

TEST_CASE("IFFT can restart and buffer its own output", "[ifft]") {
    // Restart before work, during butterflies, during normalization, or done.
    const size_t steps = GENERATE(0, 1, 13, 20);
    CAPTURE(steps);
    Fourier::OnTheFlyIFFT<float> ifft(8);
    std::vector<std::complex<float>> spectrum(8, 1.f);
    ifft.buffer(spectrum.data());
    for (size_t i = 0; i < steps; ++i) ifft.step();
    std::fill(spectrum.begin(), spectrum.end(), 0.f);
    spectrum[0] = {24.f, -8.f};
    ifft.buffer(spectrum.data());
    // Buffer owns its copy of the input.
    std::fill(spectrum.begin(), spectrum.end(), 0.f);
    ifft.compute();
    const std::complex<float> amplitude(3.f, -1.f);
    for (const auto& sample : ifft.coefficients) REQUIRE(sample == amplitude);
    ifft.buffer(ifft.coefficients.data());
    ifft.compute();
    REQUIRE(ifft.coefficients[0] == amplitude);
    for (size_t i = 1; i < ifft.size(); ++i)
        REQUIRE(ifft.coefficients[i] == std::complex<float>(0, 0));
}

TEST_CASE("IFFT resize clears pending work and permits reuse", "[ifft]") {
    const size_t n = GENERATE(1, 2, 8, 32);
    const size_t steps = GENERATE(0, 1, 13, 20);
    CAPTURE(n, steps);
    Fourier::OnTheFlyIFFT<float> ifft(8);
    std::vector<std::complex<float>> spectrum(8, 1.f);
    ifft.buffer(spectrum.data());
    for (size_t i = 0; i < steps; ++i) ifft.step();
    ifft.resize(n);
    REQUIRE(ifft.size() == n);
    REQUIRE(ifft.is_done_computing());
    ifft.step();
    ifft.compute();
    for (const auto& sample : ifft.coefficients)
        REQUIRE(sample == std::complex<float>(0, 0));
    spectrum.assign(n, std::complex<float>(-2, 1));
    ifft.buffer(spectrum.data());
    ifft.compute();
    REQUIRE(ifft.coefficients[0] == std::complex<float>(-2, 1));
    for (size_t i = 1; i < n; ++i)
        REQUIRE(ifft.coefficients[i] == std::complex<float>(0, 0));
}

TEST_CASE("IFFT rejects invalid lengths without changing pending work", "[ifft]") {
    const size_t invalid = GENERATE(0, 3, 6);
    CAPTURE(invalid);
    REQUIRE_THROWS_AS(Fourier::OnTheFlyIFFT<float>(invalid), std::invalid_argument);
    Fourier::OnTheFlyIFFT<float> ifft(4);
    const std::vector<std::complex<float>> spectrum(4, {2.f, -1.f});
    ifft.buffer(spectrum.data());
    ifft.step();
    REQUIRE_THROWS_AS(ifft.resize(invalid), std::invalid_argument);
    REQUIRE(ifft.size() == 4);
    REQUIRE_FALSE(ifft.is_done_computing());
    ifft.compute();
    REQUIRE(ifft.coefficients[0] == spectrum[0]);
    for (size_t i = 1; i < ifft.size(); ++i)
        REQUIRE(ifft.coefficients[i] == std::complex<float>(0, 0));
}

TEMPLATE_TEST_CASE("Complex FFT applies a window before transforming and can reuse unwindowed input",
        "[fft]", float, double) {
    using T = TestType;
    for (size_t n : {8u, 32u}) {
        Fourier::OnTheFlyFFT<T> fft(n);
        std::vector<std::complex<T>> input(n);
        std::vector<T> weights(n);
        for (size_t i = 0; i < n; ++i) {
            input[i] = {T(int(i % 7) - 3) / T(4), T(int(i % 5) - 2) / T(4)};
            weights[i] = T(i) / T(n - 1);
        }
        const auto original = input;
        for (bool windowed : {true, false}) {
            CAPTURE(n, windowed);
            fft.buffer(input.data(), windowed ? weights : std::vector<T>{});
            fft.compute();
            CHECK(input == original);
            // Independent complex DFT checks both phase and amplitude. The
            // asymmetric ramp exposes windowing after bit reversal by mistake.
            for (size_t k = 0; k < n; ++k) {
                std::complex<long double> expected(0, 0);
                for (size_t i = 0; i < n; ++i) {
                    const long double angle = -2.L * std::acos(-1.L) * k * i / n;
                    expected += std::complex<long double>(input[i].real(), input[i].imag())
                        * (windowed ? static_cast<long double>(weights[i]) : 1.L)
                        * std::complex<long double>(std::cos(angle), std::sin(angle));
                }
                const std::complex<long double> actual(fft.coefficients[k].real(),
                                                       fft.coefficients[k].imag());
                CAPTURE(k);
                CHECK(std::abs(actual - expected) < 32.L * n * std::numeric_limits<T>::epsilon());
            }
        }
    }
}

TEST_CASE("Coefficient interpolation preserves endpoints and interpolates real and imaginary parts") {
    const Fourier::DFTCoefficients coefficients = {{2.f, -4.f}, {6.f, 8.f}, {-2.f, 4.f}};
    CHECK(Fourier::interpolate_coefficients(coefficients, 0.f) == coefficients.front());
    CHECK(Fourier::interpolate_coefficients(coefficients, 2.f) == coefficients.back());
    CHECK(Fourier::interpolate_coefficients(coefficients, 0.5f) == std::complex<float>(4.f, 2.f));
    CHECK(Fourier::interpolate_coefficients(coefficients, 1.25f) == std::complex<float>(4.f, 7.f));
}

TEST_CASE("Bit reversal resizes and restores indices when applied twice") {
    Fourier::BitReversalTable table(8);
    const size_t expected[] = {0, 4, 2, 6, 1, 5, 3, 7};
    REQUIRE(table.size() == 8);
    for (size_t i = 0; i < table.size(); ++i) CHECK(table[i] == expected[i]);
    for (size_t n : {32u, 4u, 1u}) {
        table.resize(n);
        REQUIRE(table.size() == n);
        for (size_t i = 0; i < n; ++i) {
            REQUIRE(table[i] < n);
            CHECK(table[table[i]] == i);
        }
    }
}
