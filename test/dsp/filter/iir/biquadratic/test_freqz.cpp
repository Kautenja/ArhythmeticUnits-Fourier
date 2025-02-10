// Test cases for Filter::IIR::Biquadratic::freqz function.
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

#include "dsp/filter/iir/biquadratic/freqz.hpp"
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

TEST_CASE("freqz2 should calculate response of Sallen Key low-pass filter") {
    const float b[3] = { 0.32980819, 0.65961638, 0.32980819};
    const float a[2] = {-0.58997883, 0.90921159};
    const float phase[20] = {
        0.  , 0.05, 0.1 , 0.15, 0.2 , 0.25, 0.3 , 0.35, 0.4 , 0.45,
        0.5 , 0.55, 0.6 , 0.65, 0.7 , 0.75, 0.8 , 0.85, 0.9 , 0.95
    };
    const std::complex<float> target[20] = {
        {1,                      -0                     },
        {1.0117517704383878,     -0.011089787663364289  },
        {1.0493443269758038,     -0.024016789423198508  },
        {1.1210321072176475,     -0.0415839724486669    },
        {1.2461059190031154,     -0.06965943855139536   },
        {1.4710572341102084,     -0.12425385349324675   },
        {1.9310452234230566,     -0.2664913384537999    },
        {3.1923775213782233,     -0.9330025228593356    },
        {-0,                     -10.00000000000002     },
        {-2.3918740945516017,    -0.7362575516075778    },
        {-1.0921710080636748,    -0.1680678365648833    },
        {-0.6198435253428294,    -0.06254663211788063   },
        {-0.3842138671699112,    -0.028115339440747126  },
        {-0.24647429900293527,   -0.013686741048314426  },
        {-0.15851331064569107,   -0.006799907789179349  },
        {-0.0994774777367446,    -0.003291845804657064  },
        {-0.0589801317925771,    -0.0014745032948144182 },
        {-0.03136949892943213,   -0.0005643401104226434 },
        {-0.013417676253081531,  -0.00015647325641743702},
        {-0.003280185260608099,  -1.8817669922830012e-05}
    };
    for (unsigned i = 1; i < 20; i++) {
        auto output = Filter::IIR::Biquadratic::freqz(b, a, phase[i] * Math::pi<float>());
        REQUIRE(approx_equal(output.real(), target[i].real()));
        REQUIRE(approx_equal(output.imag(), target[i].imag()));
    }
}
