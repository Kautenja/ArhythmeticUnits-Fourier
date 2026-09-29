// Benchmark code for FFT computations.
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

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <cmath>
#include <complex>
#include <string>
#include <vector>
#include "catch.hpp"
#include "../fixtures.hpp"
#include "dsp/fft.hpp"
#include "dsp/window.hpp"

TEMPLATE_TEST_CASE("Legacy transforms: one complete frame", "[fft]", float, double) {
    using T = TestType;
    for (const size_t n : {128u, 2048u, 16384u}) {
        const std::string size = " N=" + std::to_string(n);
        const auto real = BenchmarkFixtures::signal<T>(n);
        std::vector<std::complex<T>> complex(n);
        for (size_t i = 0; i < n; ++i) complex[i] = {real[i], real[(i+17)%n]};
        Fourier::Window::CachedWindow<float> window(Fourier::Window::Function::Hann,
            n, false, true);
        Fourier::OnTheFlyFFT<T> fft(n);
        Fourier::OnTheFlyRFFT<T> rfft(n);
        Fourier::OnTheFlyIFFT<T> ifft(n);
        fft.buffer(complex.data());
        fft.compute();
        const auto spectrum = fft.coefficients;
        ifft.buffer(spectrum.data());
        ifft.compute();
        REQUIRE(std::abs(ifft.coefficients[17] - complex[17]) < T(1e-4));
        rfft.buffer(real.data(), window.get_samples());
        rfft.compute();
        REQUIRE(std::abs(rfft.coefficients[7]) > T(n) / T(10));

        BENCHMARK("FFT buffer+compute" + size) {
            Catch::Benchmark::keep_memory(complex.data());
            fft.buffer(complex.data());
            fft.compute();
            Catch::Benchmark::keep_memory(fft.coefficients.data());
        };
        BENCHMARK("IFFT buffer+compute" + size) {
            Catch::Benchmark::keep_memory(spectrum.data());
            ifft.buffer(spectrum.data());
            ifft.compute();
            Catch::Benchmark::keep_memory(ifft.coefficients.data());
        };
        BENCHMARK("RFFT Hann buffer+compute" + size) {
            Catch::Benchmark::keep_memory(real.data());
            rfft.buffer(real.data(), window.get_samples());
            rfft.compute();
            Catch::Benchmark::keep_memory(rfft.coefficients.data());
        };
        BENCHMARK("RFFT Hann buffer+compute+1/3 octave" + size) {
            Catch::Benchmark::keep_memory(real.data());
            rfft.buffer(real.data(), window.get_samples());
            rfft.compute();
            rfft.smooth(48000.f, 1.f/3.f);
            Catch::Benchmark::keep_memory(rfft.coefficients.data());
        };
        // Restart each iteration: timing step() on an already-finished FFT
        // would otherwise report the early-return path rather than butterflies.
        BENCHMARK("FFT buffer+step until complete H=256" + size) {
            Catch::Benchmark::keep_memory(complex.data());
            fft.buffer(complex.data());
            while (!fft.is_done_computing()) fft.step(256);
            Catch::Benchmark::keep_memory(fft.coefficients.data());
        };
        BENCHMARK("RFFT Hann buffer+step until complete H=256" + size) {
            Catch::Benchmark::keep_memory(real.data());
            rfft.buffer(real.data(), window.get_samples());
            while (!rfft.is_done_computing()) rfft.step(256);
            Catch::Benchmark::keep_memory(rfft.coefficients.data());
        };
        BENCHMARK("IFFT buffer+step until complete H=256" + size) {
            Catch::Benchmark::keep_memory(spectrum.data());
            ifft.buffer(spectrum.data());
            while (!ifft.is_done_computing()) ifft.step(256);
            Catch::Benchmark::keep_memory(ifft.coefficients.data());
        };
    }
}

TEST_CASE("FFT plan construction includes allocation", "[fft][setup]") {
    for (const size_t n : {128u, 2048u, 16384u}) {
        BENCHMARK("Twiddles+bit reversal float N=" + std::to_string(n)) {
            Fourier::TwiddleFactors<float> twiddles(n);
            Fourier::BitReversalTable reversal(n);
            Catch::Benchmark::deoptimize_value(twiddles);
            Catch::Benchmark::deoptimize_value(reversal);
        };
    }
}
