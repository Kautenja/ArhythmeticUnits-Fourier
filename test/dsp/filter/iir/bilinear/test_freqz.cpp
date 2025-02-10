// Test cases for Filter::IIR::Bilinear::freqz function.
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

#include "dsp/filter/iir/bilinear/freqz.hpp"
#include "dsp/math/constants.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

/// @brief Return true if the given floats are approximately equal.
///
/// @param a the first operand
/// @param b the second operand
/// @param p the number of digits of precision
/// @returns true if the floating point numbers are approximately equal
/// @details
/// This is best suited for floats in the range [-1, 1]
///
inline bool approx_equal(float a, float b, int p = 7) {
    return (roundf(a * p) / p) == (roundf(b * p) / p);
}

TEST_CASE("freqz should calculate response of 1-pole low-pass filter") {
    const float b[2] = { 0.42080778, 0.42080778};
    const float a[1] = {-0.15838444};
    const float phase[20] = {
        0.  , 0.05, 0.1 , 0.15, 0.2 , 0.25, 0.3 , 0.35, 0.4 , 0.45,
        0.5 , 0.55, 0.6 , 0.65, 0.7 , 0.75, 0.8 , 0.85, 0.9 , 0.95
    };
    const std::complex<float> target[20] = {
        {1.0       ,  0.0       },
        {0.98840209, -0.10706728},
        {0.95463307, -0.2081076 },
        {0.90155828, -0.29791097},
        {0.83333333, -0.372678  },
        {0.75469857, -0.43026578},
        {0.67032038, -0.47009677},
        {0.58431543, -0.49283964},
        {0.5       , -0.5       },
        {0.41983341, -0.49353148},
        {0.3454915 , -0.47552826},
        {0.27800568, -0.44801621},
        {0.21791931, -0.41283227},
        {0.16543293, -0.37157082},
        {0.12052505, -0.32557451},
        {0.08304592, -0.27595162},
        {0.0527864 , -0.2236068 },
        {0.02952658, -0.16927718},
        {0.01306875, -0.11356917},
        {0.00325891, -0.05699379}
    };
    for (unsigned i = 1; i < 20; i++) {
        auto output = Filter::IIR::Bilinear::freqz(b, a, phase[i] * Math::pi<float>());
        REQUIRE(approx_equal(output.real(), target[i].real()));
        REQUIRE(approx_equal(output.imag(), target[i].imag()));
    }
}

TEST_CASE("freqz should calculate response of 1-pole high-pass filter") {
    const float b[2] = { 0.57919222, -0.57919222};
    const float a[1] = {-0.15838444};
    const float phase[20] = {
        0.  , 0.05, 0.1 , 0.15, 0.2 , 0.25, 0.3 , 0.35, 0.4 , 0.45,
        0.5 , 0.55, 0.6 , 0.65, 0.7 , 0.75, 0.8 , 0.85, 0.9 , 0.95
    };
    const std::complex<float> target[20] = {
        {0.0       , 0.0       },
        {0.01159791, 0.10706728},
        {0.04536693, 0.2081076 },
        {0.09844172, 0.29791097},
        {0.16666667, 0.372678  },
        {0.24530143, 0.43026578},
        {0.32967962, 0.47009677},
        {0.41568457, 0.49283964},
        {0.5       , 0.5       },
        {0.58016659, 0.49353148},
        {0.6545085 , 0.47552826},
        {0.72199432, 0.44801621},
        {0.78208069, 0.41283227},
        {0.83456707, 0.37157082},
        {0.87947495, 0.32557451},
        {0.91695408, 0.27595162},
        {0.9472136 , 0.2236068 },
        {0.97047342, 0.16927718},
        {0.98693125, 0.11356917},
        {0.99674109, 0.05699379}
    };
    for (unsigned i = 1; i < 20; i++) {
        auto output = Filter::IIR::Bilinear::freqz(b, a, phase[i] * Math::pi<float>());
        REQUIRE(approx_equal(output.real(), target[i].real()));
        REQUIRE(approx_equal(output.imag(), target[i].imag()));
    }
}

TEST_CASE("freqz should calculate response of 1-pole all-pass filter") {
    const float b[2] = { 0.15838444, -1.0};
    const float a[1] = {-0.15838444};
    const float phase[20] = {
        0.  , 0.05, 0.1 , 0.15, 0.2 , 0.25, 0.3 , 0.35, 0.4 , 0.45,
        0.5 , 0.55, 0.6 , 0.65, 0.7 , 0.75, 0.8 , 0.85, 0.9 , 0.95
    };
    const std::complex<float> target[20] = {
        {-1.00000000e+00, 0.0       },
        {-9.76804173e-01, 0.21413456},
        {-9.09266134e-01, 0.41621521},
        {-8.03116557e-01, 0.59582195},
        {-6.66666667e-01, 0.74535599},
        {-5.09397144e-01, 0.86053155},
        {-3.40640757e-01, 0.94019353},
        {-1.68630860e-01, 0.98567927},
        {-8.54092723e-17, 1.0       },
        {1.60333181e-01 , 0.98706295 },
        {3.09016994e-01 , 0.95105652 },
        {4.43988645e-01 , 0.89603241 },
        {5.64161372e-01 , 0.82566455 },
        {6.69134143e-01 , 0.74314164 },
        {7.58949902e-01 , 0.65114902 },
        {8.33908161e-01 , 0.55190323 },
        {8.94427191e-01 , 0.4472136  },
        {9.40946833e-01 , 0.33855436 },
        {9.73862502e-01 , 0.22713834 },
        {9.93482174e-01 , 0.11398758 },
    };
    for (unsigned i = 1; i < 20; i++) {
        auto output = Filter::IIR::Bilinear::freqz(b, a, phase[i] * Math::pi<float>());
        REQUIRE(approx_equal(output.real(), target[i].real()));
        REQUIRE(approx_equal(output.imag(), target[i].imag()));
    }
}
