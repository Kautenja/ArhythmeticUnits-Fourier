// Test cases for Filter::IIR::Biquadratic::DirectForm2TransposeTranspose structure.
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

#include <cstdint>
#include "dsp/filter/iir/biquadratic/direct_form_2_transpose.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../_common.hpp"

TEST_CASE("Filter::IIR::Biquadratic::DirectForm2Transpose<int32_t>() should initialize") {
    auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<int32_t>();
    // coefficients should be 0
    REQUIRE(0 == filter.a[0]);
    REQUIRE(0 == filter.a[1]);
    REQUIRE(0 == filter.b[0]);
    REQUIRE(0 == filter.b[1]);
    REQUIRE(0 == filter.b[2]);
    // delay lines should be empty
    REQUIRE(0 == filter.h[0]);
    REQUIRE(0 == filter.h[1]);
}

TEST_CASE("Filter::IIR::Biquadratic::DirectForm2Transpose<float>() should initialize") {
    auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
    // coefficients should be 0
    REQUIRE(0.f == filter.a[0]);
    REQUIRE(0.f == filter.a[1]);
    REQUIRE(0.f == filter.b[0]);
    REQUIRE(0.f == filter.b[1]);
    REQUIRE(0.f == filter.b[2]);
    // delay lines should be empty
    REQUIRE(0.f == filter.h[0]);
    REQUIRE(0.f == filter.h[1]);
}

SCENARIO("(Filter::IIR::Biquadratic::DirectForm2Transpose).reset() should reset state") {
    GIVEN("an initialized filter with set coefficients and empty delay lines") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.a[0] = 1.f;
        filter.a[1] = 2.f;
        filter.b[0] = 3.f;
        filter.b[1] = 4.f;
        filter.b[2] = 5.f;
        WHEN("the filter is reset") {
            filter.reset();
            THEN("the delay line should be empty") {
                REQUIRE(0.f == filter.h[0]);
                REQUIRE(0.f == filter.h[1]);
            }
        }
    }
    GIVEN("an initialized filter with set coefficients and data in the delay line") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.a[0] = 1.f;
        filter.a[1] = 2.f;
        filter.b[0] = 3.f;
        filter.b[1] = 4.f;
        filter.b[2] = 5.f;
        filter.process(1.f);
        filter.process(1.f);
        WHEN("the filter is reset") {
            filter.reset();
            THEN("the delay line should be cleared") {
                REQUIRE(0.f == filter.h[0]);
                REQUIRE(0.f == filter.h[1]);
            }
        }
    }
}

SCENARIO("(Filter::IIR::Biquadratic::DirectForm2Transpose).process() should compute a sinusoidal input") {
    // samples of a sinusoid at 0.4pi
    static const float sinusoid[30] = {
         0.        ,  0.40208519,  0.73630021,  0.94623109,  0.99644245,
         0.87845883,  0.61219533,  0.24259603, -0.16795238, -0.55015119,
        -0.83948697, -0.98712114, -0.96813374, -0.78572974, -0.47069811,
        -0.07621478,  0.33113327,  0.68258752,  0.91882408,  0.9999673 ,
         0.91232056,  0.67067824,  0.31582847, -0.09233172, -0.48490673,
        -0.79563168, -0.9720576 , -0.9844046 , -0.83058855, -0.53657292
    };
    // zero
    GIVEN("an initialized filter with b=[0 0 0] and a=[0 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 0;
        filter.b[1] = 0;
        filter.b[2] = 0;
        filter.a[0] = 0;
        filter.a[1] = 0;
        static const float target[30] = {
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    // identity
    GIVEN("an initialized filter with b=[1 0 0] and a=[0 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.b[2] = 0;
        filter.a[0] = 0;
        filter.a[1] = 0;
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, sinusoid);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 1 0] and a=[1 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 1;
        filter.b[2] = 0;
        filter.a[0] = 1;
        filter.a[1] = 0;
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, sinusoid);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 0 1] and a=[0 1]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.b[2] = 1;
        filter.a[0] = 0;
        filter.a[1] = 1;
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, sinusoid);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 1 1] and a=[1 1]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 1;
        filter.b[2] = 1;
        filter.a[0] = 1;
        filter.a[1] = 1;
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, sinusoid);
            }
        }
    }
    // simple b, no a
    GIVEN("an initialized filter with b=[0 1 0] and a=[0 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 0;
        filter.b[1] = 1;
        filter.b[2] = 0;
        filter.a[0] = 0;
        filter.a[1] = 0;
        static const float target[30] = {
             0.        ,  0.        ,  0.40208519,  0.73630021,  0.94623109,
             0.99644245,  0.87845883,  0.61219533,  0.24259603, -0.16795238,
            -0.55015119, -0.83948697, -0.98712114, -0.96813374, -0.78572974,
            -0.47069811, -0.07621478,  0.33113327,  0.68258752,  0.91882408,
             0.9999673 ,  0.91232056,  0.67067824,  0.31582847, -0.09233172,
            -0.48490673, -0.79563168, -0.9720576 , -0.9844046 , -0.83058855
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[0 0 1] and a=[0 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 0;
        filter.b[1] = 0;
        filter.b[2] = 1;
        filter.a[0] = 0;
        filter.a[1] = 0;
        static const float target[30] = {
             0.        ,  0.        ,  0.        ,  0.40208519,  0.73630021,
             0.94623109,  0.99644245,  0.87845883,  0.61219533,  0.24259603,
            -0.16795238, -0.55015119, -0.83948697, -0.98712114, -0.96813374,
            -0.78572974, -0.47069811, -0.07621478,  0.33113327,  0.68258752,
             0.91882408,  0.9999673 ,  0.91232056,  0.67067824,  0.31582847,
            -0.09233172, -0.48490673, -0.79563168, -0.9720576 , -0.9844046
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    // complex b, no a
    GIVEN("an initialized filter with b=[1 1 0] and a=[0 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 1;
        filter.b[2] = 0;
        filter.a[0] = 0;
        filter.a[1] = 0;
        static const float target[30] = {
             0.        ,  0.40208519,  1.1383854 ,  1.68253131,  1.94267354,
             1.87490128,  1.49065416,  0.85479136,  0.07464365, -0.71810357,
            -1.38963816, -1.82660811, -1.95525488, -1.75386348, -1.25642785,
            -0.54691289,  0.25491849,  1.01372079,  1.6014116 ,  1.91879138,
             1.91228786,  1.5829988 ,  0.9865067 ,  0.22349674, -0.57723846,
            -1.28053842, -1.76768929, -1.9564622 , -1.81499315, -1.36716147
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 0 1] and a=[0 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.b[2] = 1;
        filter.a[0] = 0;
        filter.a[1] = 0;
        static const float target[30] = {
             0.        ,  0.40208519,  0.73630021,  1.34831628,  1.73274266,
             1.82468992,  1.60863778,  1.12105486,  0.44424295, -0.30755516,
            -1.00743935, -1.53727233, -1.8076207 , -1.77285088, -1.43883185,
            -0.86194452, -0.13956484,  0.60637274,  1.24995735,  1.68255481,
             1.83114464,  1.67064554,  1.22814903,  0.57834652, -0.16907827,
            -0.88796341, -1.45696434, -1.78003628, -1.80264616, -1.52097752
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 1 1] and a=[0 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 1;
        filter.b[2] = 1;
        filter.a[0] = 0;
        filter.a[1] = 0;
        static const float target[30] = {
             0.        ,  0.40208519,  1.1383854 ,  2.08461649,  2.67897376,
             2.82113237,  2.48709661,  1.73325019,  0.68683898, -0.47550754,
            -1.55759054, -2.3767593 , -2.79474185, -2.74098462, -2.22456159,
            -1.33264263, -0.21577962,  0.93750601,  1.93254487,  2.6013789 ,
             2.83111194,  2.5829661 ,  1.89882726,  0.89417498, -0.26140999,
            -1.37287014, -2.25259602, -2.75209389, -2.78705076, -2.35156607
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    // simple a, i.e., b_0 = 1, b_n>0 = 0
    GIVEN("an initialized filter with b=[1 0 0] and a=[1 0]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.b[2] = 0;
        filter.a[0] = 1;
        filter.a[1] = 0;
        static const float target[30] = {
             0.        ,  0.40208519,  0.33421503,  0.61201607,  0.38442638,
             0.49403245,  0.11816288,  0.12443315, -0.29238553, -0.25776566,
            -0.58172131, -0.40539984, -0.5627339 , -0.22299584, -0.24770228,
             0.1714875 ,  0.15964577,  0.52294175,  0.39588233,  0.60408496,
             0.3082356 ,  0.36244264, -0.04661418, -0.04571755, -0.43918919,
            -0.3564425 , -0.61561511, -0.36878949, -0.46179906, -0.07477386
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 0 0] and a=[0 1]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.b[2] = 0;
        filter.a[0] = 0;
        filter.a[1] = 1;
        static const float target[30] = {
             0.        ,  0.40208519,  0.73630021,  0.54414591,  0.26014224,
             0.33431292,  0.35205309, -0.09171689, -0.52000547, -0.4584343 ,
            -0.31948149, -0.52868684, -0.64865224, -0.2570429 ,  0.17795413,
             0.18082812,  0.15317914,  0.5017594 ,  0.76564494,  0.4982079 ,
             0.14667562,  0.17247034,  0.16915285, -0.26480206, -0.65405958,
            -0.53082962, -0.31799802, -0.45357498, -0.51259053, -0.08299794
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 0 0] and a=[1 1]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.b[2] = 0;
        filter.a[0] = 1;
        filter.a[1] = 1;
        static const float target[30] = {
             0.        ,  0.40208519,  0.33421503,  0.20993088,  0.45229654,
             0.2162314 , -0.05633262,  0.08269724, -0.19431701, -0.43853142,
            -0.20663854, -0.34195118, -0.41954402, -0.02423454, -0.02691956,
            -0.02506068,  0.38311351,  0.32453469,  0.21117588,  0.46425673,
             0.23688795, -0.03046644,  0.10940695, -0.17127224, -0.42304145,
            -0.20131799, -0.34769816, -0.43538845, -0.04750195, -0.05368252
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    // complex cases
    GIVEN("an initialized filter with b=[1 0.5 0.75] and a=[0.25 0.5]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0.5;
        filter.b[2] = 0.75;
        filter.a[0] = 0.25;
        filter.a[1] = 0.5;
        static const float target[30] = {
             0.        ,  0.40208519,  0.83682151,  1.20569712,  1.30194812,
             1.15801778,  0.85827807,  0.41395941, -0.12013675, -0.62912587,
            -1.023177  , -1.24912083, -1.26744083, -1.06871684, -0.68876366,
            -0.1943118 ,  0.33296208,  0.80490845,  1.14075964,  1.28367584,
             1.21012349,  0.9324452 ,  0.49723496, -0.02194015, -0.53733369,
            -0.96203034, -1.22437907, -1.28004723, -1.11963272, -0.77023885
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 1.5 -0.75] and a=[-0.25 -0.6]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 1.5;
        filter.b[2] = -0.75;
        filter.a[0] = -0.25;
        filter.a[1] = -0.6;
        static const float target[30] = {
             0.        ,  0.40208519,  1.43994929,  2.35035596,  3.31512249,
             3.90244338,  4.14723608,  3.88031995,  3.1952168 ,  2.14296939,
             0.91412296, -0.31942583, -1.35058291, -2.02689071, -2.24026484,
            -1.96916528, -1.26661553, -0.26170455,  0.86895995,  1.92648004,
             2.72614942,  3.12660898,  3.0549473 ,  2.51810451,  1.60221884,
             0.45767443, -0.72607517, -1.75268138, -2.45196769, -2.70875305
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[-1 -0.25 -1.75] and a=[-0.8 1.25]") {
        auto filter = Filter::IIR::Biquadratic::DirectForm2Transpose<double>();
        filter.b[0] = -1;
        filter.b[1] = -0.25;
        filter.b[2] = -1.75;
        filter.a[0] = -0.8;
        filter.a[1] = 1.25;
        // double precision is required for the approximations in the Catch2
        // testing framework to work out correctly
        static const double target[30] = {
             0.        , -0.40208519, -1.15848966, -2.25814047, -2.8799259 ,
            -2.26473899, -0.78746814,  0.26800141,  0.23469785,  0.02035275,
             0.99385132,  2.92939758,  3.78522014,  2.12165431, -0.67283714,
            -1.62142125,  0.05555155,  1.43922282, -0.58701536, -4.69284229,
            -5.79075916, -1.41525561,  4.02618545,  3.82970557, -2.01367751,
            -5.31963509,  0.28094113,  9.49407121, 10.02187107, -1.38316405
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
}
