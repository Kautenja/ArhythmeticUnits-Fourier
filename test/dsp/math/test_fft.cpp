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

#include "dsp/math/dft.hpp"
#include "dsp/math/fft.hpp"
#include "../../ieee754.hpp"
#include "../../functions.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: `OnTheFlyFFT`
// ---------------------------------------------------------------------------

SCENARIO("the FFT needs to be calculated") {
    GIVEN("a sequence with no signal (length 1)") {
        std::vector<std::complex<float>> sequence = {0};
        Math::OnTheFlyFFT<float> fft(sequence.size());
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
        Math::OnTheFlyFFT<float> fft(sequence.size());
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
        Math::OnTheFlyFFT<float> fft(sequence.size());
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
        Math::OnTheFlyFFT<float> fft(sequence.size());
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
        Math::OnTheFlyFFT<float> fft(sequence.size());
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
        Math::OnTheFlyFFT<float> fft(sequence.size());
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
        Math::OnTheFlyRFFT<float> fft(sequence.size());
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
        Math::OnTheFlyRFFT<float> fft(sequence.size());
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

// // ---------------------------------------------------------------------------
// // MARK: `ifft`
// // ---------------------------------------------------------------------------

// SCENARIO("the IFFT needs to be calculated") {
//     GIVEN("a coefficient sequence {0}") {
//         std::vector<std::complex<float>> sequence = {{0, 0}};
//         WHEN("the IFFT is calculated") {
//             auto output = Math::ifft<1>(sequence);
//             THEN("the output has no signal content") {
//                 REQUIRE(output.size() == 1);
//                 REQUIRE(epsilon_equal(0.f, output[0]));
//             }
//         }
//     }
//     GIVEN("a coefficient sequence {0, 0}") {
//         std::vector<std::complex<float>> sequence = {{0, 0}, {0, 0}};
//         WHEN("the IFFT is calculated") {
//             auto output = Math::ifft<2>(sequence);
//             THEN("the output has no signal content") {
//                 REQUIRE(output.size() == 2);
//                 REQUIRE(epsilon_equal(0.f, output[0]));
//                 REQUIRE(epsilon_equal(0.f, output[1]));
//             }
//         }
//     }
//     GIVEN("a coefficient sequence {1}") {
//         std::vector<std::complex<float>> sequence = {{1, 0}};
//         WHEN("the IFFT is calculated") {
//             auto output = Math::ifft<1>(sequence);
//             THEN("the output is correct") {
//                 REQUIRE(output.size() == 1);
//                 REQUIRE(epsilon_equal(1.f, output[0]));
//             }
//         }
//     }
//     GIVEN("a coefficient sequence {1, 0}") {
//         std::vector<std::complex<float>> sequence = {{1, 0}, {0, 0}};
//         WHEN("the IFFT is calculated") {
//             auto output = Math::ifft<2>(sequence);
//             THEN("the output is correct") {
//                 REQUIRE(output.size() == 2);
//                 REQUIRE(epsilon_equal(1.f/2.f, output[0]));
//                 REQUIRE(epsilon_equal(1.f/2.f, output[1]));
//             }
//         }
//     }
//     GIVEN("a coefficient sequence {1, 0, 0, 0}") {
//         std::vector<std::complex<float>> sequence = {{1, 0}, {0, 0}, {0, 0}, {0, 0}};
//         WHEN("the IFFT is calculated") {
//             auto output = Math::ifft<4>(sequence);
//             THEN("the output is correct") {
//                 REQUIRE(output.size() == 4);
//                 REQUIRE(epsilon_equal(1.f/4.f, output[0]));
//                 REQUIRE(epsilon_equal(1.f/4.f, output[1]));
//                 REQUIRE(epsilon_equal(1.f/4.f, output[2]));
//                 REQUIRE(epsilon_equal(1.f/4.f, output[3]));
//             }
//         }
//     }
//     GIVEN("a coefficient sequence {0, 1, 0, 0}") {
//         std::vector<std::complex<float>> sequence = {{0, 0}, {1, 0}, {0, 0}, {0, 0}};
//         auto expected = Math::idft_trig(sequence);
//         WHEN("the IFFT is calculated") {
//             auto output = Math::ifft<4>(sequence);
//             THEN("the output matches a naive DFT") {
//                 REQUIRE(output.size() == 4);
//                 for (std::size_t i = 0; i < sequence.size(); i++) {
//                     REQUIRE(approx_equal<float>(output[i], expected[i], 1e-6));
//                 }
//             }
//         }
//     }
//     GIVEN("a coefficient sequence {0, 1, 0, 1}") {
//         std::vector<std::complex<float>> sequence = {{0, 0}, {1, 0}, {0, 0}, {1, 0}};
//         auto expected = Math::idft_trig(sequence);
//         WHEN("the IFFT is calculated") {
//             auto output = Math::ifft<4>(sequence);
//             THEN("the output matches a naive DFT") {
//                 REQUIRE(output.size() == 4);
//                 for (std::size_t i = 0; i < sequence.size(); i++) {
//                     REQUIRE(approx_equal<float>(output[i], expected[i], 1e-6));
//                 }
//             }
//         }
//     }
// }

// // ---------------------------------------------------------------------------
// // MARK: `fft`/`ifft` integration
// // ---------------------------------------------------------------------------

// SCENARIO("the IFFT/IFFT needs to be calculated") {
//     GIVEN("a sequence with a sinusoid at 441Hz, a sample rate of 44100Hz, and 4096 frequency bins") {
//         const float FUNDAMENTAL = 441;
//         const float SAMPLE_RATE = 44100;
//         constexpr int FFT_BINS = 4096;
//         const auto sequence = Math::generate_sinusoid<float>(FUNDAMENTAL, SAMPLE_RATE, FFT_BINS);
//         WHEN("the FFT/IFFT is calculated.") {
//             auto components = Math::fft<FFT_BINS>(sequence);
//             auto reconstruction = Math::ifft<FFT_BINS>(components);
//             for (std::size_t i = 0; i < sequence.size(); i++) {
//                 REQUIRE(approx_equal<float>(reconstruction[i], sequence[i], 1e-6));
//             }
//         }
//     }
// }

// SCENARIO("the IFFT/IFFT needs to be calculated in-place") {
//     GIVEN("a sequence with a sinusoid at 441Hz, a sample rate of 44100Hz, and 4096 frequency bins") {
//         const float FUNDAMENTAL = 441;
//         const float SAMPLE_RATE = 44100;
//         constexpr int FFT_BINS = 4096;
//         const auto sequence = Math::generate_sinusoid<float>(FUNDAMENTAL, SAMPLE_RATE, FFT_BINS);
//         std::vector<std::complex<float>> coefficients(sequence.size());
//         for (std::size_t i = 0; i < sequence.size(); i++) coefficients[i] = sequence[i];
//         WHEN("the FFT/IFFT is calculated.") {
//             // Transform to the frequency domain and find the highest frequency.
//             Math::fft_<FFT_BINS>(coefficients);
//             auto frequency = float(Math::argmax(coefficients.data(), coefficients.size())) * SAMPLE_RATE / FFT_BINS;
//             REQUIRE(approx_equal<float>(FUNDAMENTAL, frequency, 1));
//             // Transform back to the time-domain and check against the input.
//             Math::ifft_<FFT_BINS>(coefficients);
//             for (std::size_t i = 0; i < sequence.size(); i++)
//                 REQUIRE(approx_equal<float>(coefficients[i], sequence[i], 1e-6));
//         }
//     }
// }
