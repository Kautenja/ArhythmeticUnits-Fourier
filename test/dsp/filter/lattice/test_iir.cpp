// Test cases for Filter::Lattice::StageIIR structure.
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

#include "dsp/filter/lattice/iir.hpp"
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

TEST_CASE("Filter::Lattice::StageIIR<int32_t>() should initialize") {
    auto filter = Filter::Lattice::StageIIR<int32_t>();
    REQUIRE(0 == filter.b);
    REQUIRE(0 == filter.g);
}

TEST_CASE("Filter::Lattice::StageIIR<float>() should initialize") {
    auto filter = Filter::Lattice::StageIIR<float>();
    REQUIRE(0.f == filter.b);
    REQUIRE(0.f == filter.g);
}

SCENARIO("Filter::Lattice::StageIIR::process() should filter a signal") {
    typedef Filter::Lattice::StageIIR<float> Filters;
    GIVEN("a filter cascade with k = [1], v = [1 0], x = [1 1 1 1]") {
        Filters filters[1];
        float K[1] = {1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[1] = {0};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float X[4] = {1, 1, 1, 1};
        float F[4] = {1, 0, 1, 0};
        float G[4] = {1, 1, 1, 1};
        WHEN("the signal x is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(X) / sizeof(float); i++) {
                    float f = X[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(F[i] == f);
                    REQUIRE(G[i] == g);
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [-1], v = [1 0], x = [1 1 1 1]") {
        Filters filters[1];
        float K[1] = {-1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[1] = {0};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float X[4] = { 1,  1,  1,  1};
        float F[4] = { 1,  2,  3,  4};
        float G[4] = {-1, -1, -1, -1};
        WHEN("the signal x is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(X) / sizeof(float); i++) {
                    float f = X[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(F[i] == f);
                    REQUIRE(G[i] == g);
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [1 1], v = [1 0 0], x = [1 1 1 1]") {
        Filters filters[2];
        float K[2] = {1, 1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[2] = {0, 0};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float X[4] = {1,  1, 1,  1};
        float F[4] = {1, -1, 2, -2};
        float G[4] = {1,  1, 1,  1};
        WHEN("the signal x is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(X) / sizeof(float); i++) {
                    float f = X[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(F[i] == f);
                    REQUIRE(G[i] == g);
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [1 1], v = [1 1 0], x = [1 1 1 1]") {
        Filters filters[2];
        float K[2] = {1, 1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[2] = {0, 1};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float X[4] = {1,  1, 1,  1};
        float F[4] = {2, -1, 3, -2};
        float G[4] = {1,  1, 1,  1};
        WHEN("the signal x is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(X) / sizeof(float); i++) {
                    float f = X[i];
                    float g = 1.f;
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
    GIVEN("a filter cascade with k = [1], v = [1 0]") {
        Filters filters[1];
        float K[1] = {1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[1] = {0};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.402085,0.334215,0.612016,0.384426,0.494032,0.118163,0.124433,-0.292386,-0.257766,-0.581721,-0.405400,-0.562734,-0.222996,-0.247702,0.171488
        };
        float G[16] = {
            0.000000,0.402085,0.736300,0.946231,0.996442,0.878459,0.612195,0.242596,-0.167952,-0.550151,-0.839487,-0.987121,-0.968134,-0.785730,-0.470698,-0.076215
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [1], v = [1 1]") {
        Filters filters[1];
        float K[1] = {1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[1] = {1};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.804170,1.070515,1.558247,1.380869,1.372491,0.730358,0.367029,-0.460338,-0.807917,-1.421208,-1.392521,-1.530868,-1.008726,-0.718400,0.095273
        };
        float G[16] = {
            0.000000,0.402085,0.736300,0.946231,0.996442,0.878459,0.612195,0.242596,-0.167952,-0.550151,-0.839487,-0.987121,-0.968134,-0.785730,-0.470698,-0.076215
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [-1], v = [1 0]") {
        Filters filters[1];
        float K[1] = {-1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[1] = {0};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.402085,1.138385,2.084616,3.081059,3.959518,4.571713,4.814309,4.646357,4.096206,3.256719,2.269597,1.301464,0.515734,0.045036,-0.031179
        };
        float G[16] = {
            0.000000,-0.402085,-0.736300,-0.946231,-0.996442,-0.878459,-0.612195,-0.242596,0.167952,0.550151,0.839487,0.987121,0.968134,0.785730,0.470698,0.076215
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [-1], v = [1 1]") {
        Filters filters[1];
        float K[1] = {-1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[1] = {1};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.000000,0.402085,1.138385,2.084616,3.081059,3.959518,4.571713,4.814309,4.646357,4.096206,3.256719,2.269597,1.301464,0.515734,0.045036
        };
        float G[16] = {
            0.000000,-0.402085,-0.736300,-0.946231,-0.996442,-0.878459,-0.612195,-0.242596,0.167952,0.550151,0.839487,0.987121,0.968134,0.785730,0.470698,0.076215
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [1 1], v = [1 0 0]") {
        Filters filters[2];
        float K[2] = {1, 1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[2] = {0, 0};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.402085,-0.067870,0.679886,-0.295460,0.789492,-0.671329,0.795763,-1.088148,0.830382,-1.412104,1.006704,-1.569438,1.346442,-1.594144,1.765632
        };
        float G[16] = {
            0.000000,0.402085,0.736300,0.946231,0.996442,0.878459,0.612195,0.242596,-0.167952,-0.550151,-0.839487,-0.987121,-0.968134,-0.785730,-0.470698,-0.076215
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [1 1], v = [1 1 0]") {
        Filters filters[2];
        float K[2] = {1, 1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[2] = {0, 1};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.804170,0.266345,1.291902,0.088967,1.283525,-0.553167,0.920196,-1.380534,0.572617,-1.993825,0.601304,-2.132172,1.123446,-1.841847,1.937119
        };
        float G[16] = {
            0.000000,0.402085,0.736300,0.946231,0.996442,0.878459,0.612195,0.242596,-0.167952,-0.550151,-0.839487,-0.987121,-0.968134,-0.785730,-0.470698,-0.076215
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [1 1], v = [1 1 1]") {
        Filters filters[2];
        float K[2] = {1, 1};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[2] = {1, 1};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,1.206256,1.002645,2.238133,1.085409,2.161984,0.059029,1.162792,-1.548486,0.022466,-2.833312,-0.385817,-3.100305,0.337716,-2.312545,1.860905
        };
        float G[16] = {
            0.000000,0.402085,0.736300,0.946231,0.996442,0.878459,0.612195,0.242596,-0.167952,-0.550151,-0.839487,-0.987121,-0.968134,-0.785730,-0.470698,-0.076215
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [0.5 0.25 -0.3 0.15 0.6], v = [1 0 0 0 0 0]") {
        Filters filters[5];
        // NOTE: reversed from the MATLAB notebook!
        float K[5] = {0.6, 0.15, -0.3, 0.25, 0.5};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[5] = {0,   0,     0,   0,    0};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.402085,0.497060,0.674053,0.694635,0.409198,0.060565,-0.251905,-0.662265,-0.762458,-0.741530,-0.628318,-0.319561,-0.018978,0.205684,0.474113
        };
        float G[16] = {
            0.000000,0.241251,0.480380,0.559486,0.611880,0.652751,0.758898,0.662337,0.541338,0.221002,-0.214827,-0.630444,-0.948314,-1.119040,-0.996296,-0.715693
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [0.5 0.25 -0.3 0.15 0.6], v = [1 1 0 0 0 0]") {
        Filters filters[5];
        // NOTE: reversed from the MATLAB notebook!
        float K[5] = {0.6, 0.15, -0.3, 0.25, 0.5};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[5] = {0,   0,     0,   0,    1};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.603128,1.147674,1.508139,1.716005,1.308431,0.500045,-0.317294,-1.245304,-1.805952,-1.874753,-1.684007,-1.107659,-0.348028,0.289549,0.916854
        };
        float G[16] = {
            0.000000,0.241251,0.480380,0.559486,0.611880,0.652751,0.758898,0.662337,0.541338,0.221002,-0.214827,-0.630444,-0.948314,-1.119040,-0.996296,-0.715693
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 1.f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
    GIVEN("a filter cascade with k = [0.5 0.25 -0.3 0.15 0.6], v = [0.5 0.3 0.6 0.4 0.1 0.9]") {
        Filters filters[5];
        // NOTE: reversed from the MATLAB notebook!
        float K[5] = {0.6, 0.15, -0.3, 0.25, 0.5};
        Filters::setCoefficientsPoles(filters, sizeof(K) / sizeof(float), K);
        float V[5] = {0.9, 0.1,   0.4, 0.6,  0.3};
        Filters::setCoefficientsZeros(filters, sizeof(V) / sizeof(float), V);
        float F[16] = {
            0.000000,0.496575,1.050513,1.641735,2.082940,2.164706,1.930598,1.180928,0.159980,-0.920357,-1.881090,-2.461260,-2.524833,-2.167110,-1.388134,-0.397602
        };
        float G[16] = {
            0.000000,0.241251,0.480380,0.559486,0.611880,0.652751,0.758898,0.662337,0.541338,0.221002,-0.214827,-0.630444,-0.948314,-1.119040,-0.996296,-0.715693
        };
        WHEN("the sinusoid is processed") {
            THEN("the output f,g are correct") {
                for (int i = 0; i < sizeof(sinusoid) / sizeof(float); i++) {
                    float f = sinusoid[i];
                    float g = 0.5f;
                    Filters::process(filters, sizeof(K) / sizeof(float), f, g);
                    REQUIRE(approx_equal(F[i], f));
                    REQUIRE(approx_equal(G[i], g));
                }
            }
        }
    }
}
