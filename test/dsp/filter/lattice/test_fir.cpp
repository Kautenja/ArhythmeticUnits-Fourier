// Test cases for Filter::Lattice::StageFIR structure.
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

#include "dsp/filter/lattice/fir.hpp"
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

TEST_CASE("Filter::Lattice::StageFIR<int32_t>() should initialize") {
    auto filter = Filter::Lattice::StageFIR<int32_t>();
    REQUIRE(0 == filter.b);
    REQUIRE(0 == filter.h);
}

TEST_CASE("Filter::Lattice::StageFIR<float>() should initialize") {
    auto filter = Filter::Lattice::StageFIR<float>();
    REQUIRE(0.f == filter.b);
    REQUIRE(0.f == filter.h);
}

SCENARIO("Filter::Lattice::StageFIR::process() should filter a signal") {
    typedef Filter::Lattice::StageFIR<float> Filters;
    GIVEN("a filter cascade with k = [1], x = [1 1 1 1]") {
        Filters filters[1];
        float K[1] = {1};
        Filters::setCoefficients(filters, sizeof(K) / sizeof(float), K);
        float X[4] = {1, 1, 1, 1};
        float F[4] = {1, 2, 2, 2};
        float G[4] = {1, 2, 2, 2};
        WHEN("the signal x is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(X) / sizeof(float); i++) {
                    float f = X[i];
                    float g = f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(F[i] == f);
                    REQUIRE(G[i] == g);
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [-1], x = [1 1 1 1]") {
        Filters filters[1];
        float K[1] = {-1};
        Filters::setCoefficients(filters, sizeof(K) / sizeof(float), K);
        float X[4] = {1, 1, 1, 1};
        float F[4] = {1,  0, 0, 0};
        float G[4] = {-1, 0, 0, 0};
        WHEN("the signal x is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(X) / sizeof(float); i++) {
                    float f = X[i];
                    float g = f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(F[i] == f);
                    REQUIRE(G[i] == g);
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [1 1], x = [1 1 1 1]") {
        Filters filters[2];
        float K[2] = {1, 1};
        Filters::setCoefficients(filters, sizeof(K) / sizeof(float), K);
        float X[4] = {1, 1, 1, 1};
        float F[4] = {1, 3, 4, 4};
        float G[4] = {1, 3, 4, 4};
        WHEN("the signal x is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(X) / sizeof(float); i++) {
                    float f = X[i];
                    float g = f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(F[i] == f);
                    REQUIRE(G[i] == g);
                }
            }
        }
    }
    // sinusoid inputs
    static const float sinusoid[16] = {
         0.        ,  0.40208519,  0.73630021,  0.94623109,
         0.99644245,  0.87845883,  0.61219533,  0.24259603,
        -0.16795238, -0.55015119, -0.83948697, -0.98712114,
        -0.96813374, -0.78572974, -0.47069811, -0.07621478
    };
    GIVEN("a filter cascade with k = [1]") {
        Filters filters[1];
        float K[1] = {1};
        Filters::setCoefficients(filters, sizeof(K) / sizeof(float), K);
        float F[16] = {
            0.000000,0.402085,1.138385,1.682531,1.942674,1.874901,1.490654,0.854791,0.074644,-0.718104,-1.389638,-1.826608,-1.955255,-1.753863,-1.256428,-0.546913
        };
        float G[16] = {
            0.000000,0.402085,1.138385,1.682531,1.942674,1.874901,1.490654,0.854791,0.074644,-0.718104,-1.389638,-1.826608,-1.955255,-1.753863,-1.256428,-0.546913
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [-1]") {
        Filters filters[1];
        float K[1] = {-1};
        Filters::setCoefficients(filters, sizeof(K) / sizeof(float), K);
        float F[16] = {
            0.000000,0.402085,0.334215,0.209931,0.050211,-0.117984,-0.266263,-0.369599,-0.410548,-0.382199,-0.289336,-0.147634,0.018987,0.182404,0.315032,0.394483
        };
        float G[16] = {
            0.000000,-0.402085,-0.334215,-0.209931,-0.050211,0.117984,0.266263,0.369599,0.410548,0.382199,0.289336,0.147634,-0.018987,-0.182404,-0.315032,-0.394483
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [1 1]") {
        Filters filters[2];
        float K[2] = {1, 1};
        Filters::setCoefficients(filters, sizeof(K) / sizeof(float), K);
        float F[16] = {
            0.000000,0.402085,1.540471,2.820917,3.625205,3.817575,3.365555,2.345446,0.929435,-0.643460,-2.107742,-3.216246,-3.781863,-3.709118,-3.010291,-1.803341
        };
        float G[16] = {
            0.000000,0.402085,1.540471,2.820917,3.625205,3.817575,3.365555,2.345446,0.929435,-0.643460,-2.107742,-3.216246,-3.781863,-3.709118,-3.010291,-1.803341
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [0.5 0.25 -0.3 0.15 0.6]") {
        Filters filters[5];
        float K[5] = {0.5, 0.25, -0.3, 0.15, 0.6};
        Filters::setCoefficients(filters, sizeof(K) / sizeof(float), K);
        float F[16] = {
            0.000000,0.402085,0.975541,1.360757,1.446171,1.469622,1.486258,1.252021,0.806448,0.224751,-0.394884,-0.947863,-1.340848,-1.507504,-1.419700,-1.092256
        };
        float G[16] = {
            0.000000,0.241251,0.623925,0.831169,0.874544,1.009540,1.376215,1.510592,1.389988,1.034760,0.504869,-0.110241,-0.706744,-1.183951,-1.461312,-1.492011
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
}
