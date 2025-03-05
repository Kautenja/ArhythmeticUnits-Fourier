// Test cases for the Discrete Fourier Transform (DFT.)
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

#include "dsp/math/functions.hpp"
#include "dsp/math/dft.hpp"
#include "dsp/math/ieee754.hpp"
#include "../../functions.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: `dft`
// ---------------------------------------------------------------------------

SCENARIO("the DFT needs to be calculated") {
    GIVEN("an empty sequence") {
        std::vector<float> sequence;
        WHEN("the DFT is calculated") {
            auto output = Math::dft(sequence);
            THEN("the output is empty") {
                REQUIRE(output.size() == 0);
            }
        }
    }
    GIVEN("a sequence with no signal") {
        std::vector<float> sequence = {0};
        WHEN("the DFT is calculated") {
            auto output = Math::dft(sequence);
            THEN("the output has a unit DC coefficient") {
                REQUIRE(output.size() == 1);
                REQUIRE(Math::IEEE754::epsilon_equal(std::complex<float>(0, -0), output[0]));
            }
        }
    }
    GIVEN("a sequence with no signal (length 2)") {
        std::vector<float> sequence = {0, 0};
        WHEN("the DFT is calculated") {
            auto output = Math::dft(sequence);
            THEN("the output has a unit DC coefficient") {
                REQUIRE(output.size() == 2);
                REQUIRE(Math::IEEE754::epsilon_equal(std::complex<float>(0, -0), output[0]));
                REQUIRE(Math::IEEE754::epsilon_equal(std::complex<float>(0, -0), output[1]));
            }
        }
    }
    GIVEN("a sequence with the unit impulse") {
        std::vector<float> sequence = {1};
        WHEN("the DFT is calculated") {
            auto output = Math::dft(sequence);
            THEN("the output has a unit DC coefficient") {
                REQUIRE(output.size() == 1);
                REQUIRE(Math::IEEE754::epsilon_equal(std::complex<float>(1, -0), output[0]));
            }
        }
    }
    GIVEN("a sequence with the unit impulse (length 2)") {
        std::vector<float> sequence = {1, 0};
        WHEN("the DFT is calculated") {
            auto output = Math::dft(sequence);
            THEN("the output has a unit DC coefficient") {
                REQUIRE(output.size() == 2);
                REQUIRE(Math::IEEE754::approx_equal(output[0], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(Math::IEEE754::approx_equal(output[1], std::complex<float>(1, -0), 1e-6f));
            }
        }
    }
    GIVEN("a sequence with the unit impulse (length 3)") {
        std::vector<float> sequence = {1, 0, 0};
        WHEN("the DFT is calculated") {
            auto output = Math::dft(sequence);
            THEN("the output has a unit DC coefficient") {
                REQUIRE(output.size() == 3);
                REQUIRE(Math::IEEE754::approx_equal(output[0], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(Math::IEEE754::approx_equal(output[1], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(Math::IEEE754::approx_equal(output[2], std::complex<float>(1, -0), 1e-6f));
            }
        }
    }
    GIVEN("a sequence with a shifted unit impulse (length 3)") {
        std::vector<float> sequence = {0, 1, 0};
        WHEN("the DFT is calculated") {
            auto output = Math::dft(sequence);
            THEN("the output has a unit DC coefficient") {
                REQUIRE(output.size() == 3);
                REQUIRE(Math::IEEE754::approx_equal(output[0], std::complex<float>(1, -0), 1e-6f));
                REQUIRE(Math::IEEE754::approx_equal(output[1], std::complex<float>(-0.5, -0.8660254), 1e-6f));
                REQUIRE(Math::IEEE754::approx_equal(output[2], std::complex<float>(-0.5, 0.8660254), 1e-6f));
            }
        }
    }
    GIVEN("a sequence with a sinusoid at 441Hz, a sample rate of 44100Hz, and 4096 frequency bins") {
        const float FUNDAMENTAL = 441;
        const float SAMPLE_RATE = 44100;
        const float DFT_BINS = 4096;
        const auto sequence = generate_sinusoid<float>(FUNDAMENTAL, SAMPLE_RATE, DFT_BINS);
        WHEN("the DFT is calculated.") {
            auto output = Math::dft(sequence);
            THEN("the output has the same size as the input") {
                REQUIRE(sequence.size() == output.size());
            }
            THEN("the output has a spike at the fundamental frequency") {
                // Remove the symmetric copy of the DFT coefficients above the
                // Nyquist rate (i.e., sample rate / 2)
                output.erase(output.begin() + output.size() / 2, output.end());
                // Transform the coefficients to decibels
                auto output_dB = amplitude2decibels(output);
                // Locate the coefficient with the greatest magnitude in decibels
                auto highest_bin = argmax(output_dB.data(), output_dB.size());
                // Convert the coefficient to Hz.
                auto frequency = float(highest_bin) * SAMPLE_RATE / DFT_BINS;
                // Frequency should be accurate for integral values
                REQUIRE(Math::IEEE754::approx_equal<float>(FUNDAMENTAL, frequency, 1));
            }
        }
    }
}

SCENARIO("the IDFT needs to be calculated") {
    GIVEN("an empty coefficient sequence") {
        std::vector<std::complex<float>> sequence;
        WHEN("the IDFT is calculated") {
            auto output = Math::idft(sequence);
            THEN("the output is empty") {
                REQUIRE(output.size() == 0);
            }
        }
    }
    GIVEN("a coefficient sequence {0}") {
        std::vector<std::complex<float>> sequence = {{0, 0}};
        WHEN("the IDFT is calculated") {
            auto output = Math::idft(sequence);
            THEN("the output has no signal content") {
                REQUIRE(output.size() == 1);
                REQUIRE(Math::IEEE754::epsilon_equal(0.f, output[0]));
            }
        }
    }
    GIVEN("a coefficient sequence {0, 0}") {
        std::vector<std::complex<float>> sequence = {{0, 0}, {0, 0}};
        WHEN("the IDFT is calculated") {
            auto output = Math::idft(sequence);
            THEN("the output has no signal content") {
                REQUIRE(output.size() == 2);
                REQUIRE(Math::IEEE754::epsilon_equal(0.f, output[0]));
                REQUIRE(Math::IEEE754::epsilon_equal(0.f, output[1]));
            }
        }
    }
    GIVEN("a coefficient sequence {1}") {
        std::vector<std::complex<float>> sequence = {{1, 0}};
        WHEN("the IDFT is calculated") {
            auto output = Math::idft(sequence);
            THEN("the output is correct") {
                REQUIRE(output.size() == 1);
                REQUIRE(Math::IEEE754::epsilon_equal(1.f, output[0]));
            }
        }
    }
    GIVEN("a coefficient sequence {1, 0}") {
        std::vector<std::complex<float>> sequence = {{1, 0}, {0, 0}};
        WHEN("the IDFT is calculated") {
            auto output = Math::idft(sequence);
            THEN("the output is correct") {
                REQUIRE(output.size() == 2);
                REQUIRE(Math::IEEE754::epsilon_equal(1.f/2.f, output[0]));
                REQUIRE(Math::IEEE754::epsilon_equal(1.f/2.f, output[1]));
            }
        }
    }
    GIVEN("a coefficient sequence {1, 0, 0}") {
        std::vector<std::complex<float>> sequence = {{1, 0}, {0, 0}, {0, 0}};
        WHEN("the IDFT is calculated") {
            auto output = Math::idft(sequence);
            THEN("the output is correct") {
                REQUIRE(output.size() == 3);
                REQUIRE(Math::IEEE754::epsilon_equal(1.f/3.f, output[0]));
                REQUIRE(Math::IEEE754::epsilon_equal(1.f/3.f, output[1]));
                REQUIRE(Math::IEEE754::epsilon_equal(1.f/3.f, output[2]));
            }
        }
    }
    GIVEN("a coefficient sequence {0, 1, 0}") {
        std::vector<std::complex<float>> sequence = {{0, 0}, {1, 0}, {0, 0}};
        WHEN("the IDFT is calculated") {
            auto output = Math::idft(sequence);
            THEN("the output is correct") {
                REQUIRE(output.size() == 3);
                REQUIRE(Math::IEEE754::epsilon_equal(1.f/3.f, output[0]));
                REQUIRE(Math::IEEE754::epsilon_equal(-1.f/6.f, output[1]));
                REQUIRE(Math::IEEE754::epsilon_equal(-1.f/6.f, output[2]));
            }
        }
    }
    GIVEN("coefficients with a sinusoid at 441Hz, a sample rate of 44100Hz, and 100 frequency bins") {
        const float DFT_BINS = 100;
        std::vector<std::complex<float>> coefficients(DFT_BINS);
        std::fill(coefficients.begin(), coefficients.end(), 0);
        coefficients[1] = 1;
        const float FUNDAMENTAL = 441;
        const float SAMPLE_RATE = 44100;
        WHEN("the inverse DFT is calculated.") {
            auto output = Math::idft(coefficients);
            THEN("the output has the same size as the input") {
                REQUIRE(output.size() == coefficients.size());
            }
            THEN("the output is a cosine wave in the time domain") {
                for (size_t i = 0; i < output.size(); i++) {
                    const auto expected = std::cos(2.f * Math::pi<float>() * FUNDAMENTAL * i / SAMPLE_RATE);
                    REQUIRE(Math::IEEE754::approx_equal<float>(DFT_BINS * output[i], expected, 1e-6));
                }
            }
        }
    }
}
