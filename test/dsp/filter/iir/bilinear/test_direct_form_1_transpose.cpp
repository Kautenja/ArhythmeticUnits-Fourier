// Test cases for Filter::IIR::Bilinear::DirectForm1Transpose structure.
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
#include "dsp/filter/iir/bilinear/direct_form_1_transpose.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../_common.hpp"

TEST_CASE("Filter::IIR::Bilinear::DirectForm1Transpose<int32_t>() should initialize") {
    auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<int32_t>();
    // coefficients should be 0
    REQUIRE(0 == filter.a[0]);
    REQUIRE(0 == filter.b[0]);
    REQUIRE(0 == filter.b[1]);
    // delay lines should be empty
    REQUIRE(0 == filter.h_a);
    REQUIRE(0 == filter.h_b);
}

TEST_CASE("Filter::IIR::Bilinear::DirectForm1Transpose<float>() should initialize") {
    auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
    // coefficients should be 0
    REQUIRE(0.f == filter.a[0]);
    REQUIRE(0.f == filter.b[0]);
    REQUIRE(0.f == filter.b[1]);
    // delay lines should be empty
    REQUIRE(0.f == filter.h_a);
    REQUIRE(0.f == filter.h_b);
}

SCENARIO("(Filter::IIR::Bilinear::DirectForm1Transpose).reset() should reset state") {
    GIVEN("an initialized filter with set coefficients and empty delay lines") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.a[0] = 1.f;
        filter.b[0] = 2.f;
        filter.b[1] = 3.f;
        WHEN("the filter is reset") {
            filter.reset();
            THEN("the delay lines should be empty") {
                REQUIRE(0.f == filter.h_a);
                REQUIRE(0.f == filter.h_b);
            }
            THEN("the coefficients should not have changed") {
                REQUIRE(1.f == filter.a[0]);
                REQUIRE(2.f == filter.b[0]);
                REQUIRE(3.f == filter.b[1]);
            }
        }
    }
    GIVEN("an initialized filter with set coefficients and data in the delay line") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.a[0] = 1.f;
        filter.b[0] = 2.f;
        filter.b[1] = 3.f;
        filter.process(1.f);
        filter.process(1.f);
        WHEN("the filter is reset") {
            filter.reset();
            THEN("the delay lines should be cleared") {
                REQUIRE(0.f == filter.h_a);
                REQUIRE(0.f == filter.h_b);
            }
            THEN("the coefficients should not have changed") {
                REQUIRE(1.f == filter.a[0]);
                REQUIRE(2.f == filter.b[0]);
                REQUIRE(3.f == filter.b[1]);
            }
        }
    }
}

SCENARIO("(Filter::IIR::Bilinear::DirectForm1Transpose).process() should compute a sinusoidal input") {
    // samples of a sinusoid at 0.4pi
    static const float sinusoid[30] = {
         0.        ,  0.40208519,  0.73630021,  0.94623109,  0.99644245,
         0.87845883,  0.61219533,  0.24259603, -0.16795238, -0.55015119,
        -0.83948697, -0.98712114, -0.96813374, -0.78572974, -0.47069811,
        -0.07621478,  0.33113327,  0.68258752,  0.91882408,  0.9999673 ,
         0.91232056,  0.67067824,  0.31582847, -0.09233172, -0.48490673,
        -0.79563168, -0.9720576 , -0.9844046 , -0.83058855, -0.53657292
    };
    GIVEN("an initialized filter with b=[0 0] and a=0") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.b[0] = 0;
        filter.b[1] = 0;
        filter.a[0] = 0;
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
    GIVEN("an initialized filter with b=[1 0] and a=0") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.a[0] = 0;
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, sinusoid);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 1] and a=1") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.a[0] = 0;
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, sinusoid);
            }
        }
    }
    GIVEN("an initialized filter with b=[0 1] and a=0") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.b[0] = 0;
        filter.b[1] = 1;
        filter.a[0] = 0;
        static const float target[30] = {
             0.        ,  0.        ,  0.40208519,  0.73630017,  0.94623113,
             0.99644244,  0.87845874,  0.61219519,  0.24259591, -0.16795225,
            -0.55015129, -0.83948714, -0.98712116, -0.96813369, -0.78572989,
            -0.47069788, -0.07621454,  0.33113301,  0.68258733,  0.91882414,
             0.99996728,  0.91232032,  0.67067814,  0.31582791, -0.09233182,
            -0.48490721, -0.79563171, -0.97205752, -0.98440462, -0.83058828
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 1] and a=0") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 1;
        filter.a[0] = 0;
        static const float target[30] = {
             0.        ,  0.40208519,  1.13838536,  1.6825313 ,  1.94267356,
             1.87490118,  1.49065393,  0.8547911 ,  0.07464366, -0.71810354,
            -1.38963842, -1.8266083 , -1.95525485, -1.75386357, -1.25642776,
            -0.54691242,  0.25491846,  1.01372033,  1.60141146,  1.91879141,
             1.91228759,  1.58299845,  0.98650604,  0.22349609, -0.57723903,
            -1.28053892, -1.76768923, -1.95646214, -1.8149929 , -1.36716121
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 0] and a=1") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = 0;
        filter.a[0] = 1;
        static const float target[30] = {
             0.        ,  0.40208519,  0.33421499,  0.61201614,  0.3844263 ,
             0.49403244,  0.11816275,  0.12443316, -0.29238541, -0.25776587,
            -0.58172126, -0.4053999 , -0.56273378, -0.2229961 , -0.24770178,
             0.17148723,  0.15964577,  0.52294155,  0.39588258,  0.60408469,
             0.30823562,  0.36244252, -0.04661461, -0.04571721, -0.43919   ,
            -0.35644171, -0.61561581, -0.36878881, -0.46179947, -0.07477346
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[1 -0.75] and a=-0.25") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.b[0] = 1;
        filter.b[1] = -0.75;
        filter.a[0] = -0.25;
        static const float target[30] = {
             0.        ,  0.40208519,  0.53525758,  0.52782039,  0.41872419,
             0.23580796,  0.01230313, -0.2134747 , -0.40326786, -0.52500406,
            -0.55812468, -0.49703698, -0.35205206, -0.14764263,  0.08168888,
             0.29723108,  0.46260169,  0.54988799,  0.54435564,  0.44693808,
             0.27407938,  0.05495775, -0.17344126, -0.37256306, -0.50879911,
            -0.55915108, -0.51512151, -0.38414186, -0.18832028,  0.03928821
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
    GIVEN("an initialized filter with b=[-1 -1.75] and a=1.25") {
        auto filter = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
        filter.b[0] = -1;
        filter.b[1] = -1.75;
        filter.a[0] = 1.25;
        static const float target[30] = {
             0.        ,  -0.40208519,  -0.93734276,  -1.06307797,
            -1.32349944,  -0.9678587 ,  -0.93967461,  -0.13934423,
            -0.0824103 ,   0.94708061,   0.61840113,   1.68322224,
             0.59156793,   1.74050393,  -0.32990473,   1.31231675,
            -1.83815349,   1.03562178,  -3.40787918,   1.65193946,
            -4.72718737,   3.64174552,  -6.04169655,   7.09175367,
            -8.2182042 ,  11.91697457, -12.53180521,  18.35026179,
            -20.38453087,  27.47076601
        };
        WHEN("the filter processes a sinusoid") {
            THEN("the output should be correct") {
                process_compare(filter, sinusoid, target);
            }
        }
    }
}
