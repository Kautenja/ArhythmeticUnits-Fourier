// Test cases for basic mathematical functions.
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
#include "dsp/math/ieee754.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: `sgn`
// ---------------------------------------------------------------------------

// float

TEST_CASE("sign should return positive for positive float") {
    const float input = 5.f;
    const float sign = 1.f;
    REQUIRE(sign == Math::sgn(input));
}

TEST_CASE("sign should return positive for 0 float") {
    const float input = 0.f;
    const float sign = 1.f;
    REQUIRE(sign == Math::sgn(input));
}

TEST_CASE("sign should return negative for negative float") {
    const float input = -5.f;
    const float sign = -1.f;
    REQUIRE(sign == Math::sgn(input));
}

// double

TEST_CASE("sign should return positive for positive double") {
    const double input = 5.0;
    const double sign = 1.0;
    REQUIRE(sign == Math::sgn(input));
}

TEST_CASE("sign should return positive for 0 double") {
    const double input = 0.0;
    const double sign = 1.0;
    REQUIRE(sign == Math::sgn(input));
}

TEST_CASE("sign should return negative for negative double") {
    const double input = -5.0;
    const double sign = -1.0;
    REQUIRE(sign == Math::sgn(input));
}

// int

TEST_CASE("sign should return positive for positive integer") {
    const int input = 5;
    const int sign = 1;
    REQUIRE(sign == Math::sgn(input));
}

TEST_CASE("sign should return positive for 0 integer") {
    const int input = 0;
    const int sign = 1;
    REQUIRE(sign == Math::sgn(input));
}

TEST_CASE("sign should return negative for negative integer") {
    const int input = -5;
    const int sign = -1;
    REQUIRE(sign == Math::sgn(input));
}

// ---------------------------------------------------------------------------
// MARK: `mod`
// ---------------------------------------------------------------------------

// a mod 2

TEST_CASE("1 % 2 is correct") {
    REQUIRE(1 == Math::mod(1, 2));
}

TEST_CASE("1 % -2 is correct") {
    REQUIRE(-1 == Math::mod(1, -2));
}

TEST_CASE("-1 % 2 is correct") {
    REQUIRE(1 == Math::mod(1, 2));
}

TEST_CASE("-1 % -2 is correct") {
    REQUIRE(-1 == Math::mod(-1, -2));
}

TEST_CASE("2 % 2 is correct") {
    REQUIRE(0 == Math::mod(2, 2));
}

TEST_CASE("3 % 2 is correct") {
    REQUIRE(1 == Math::mod(3, 2));
}

// a mod 3

TEST_CASE("1 % 3 is correct") {
    REQUIRE(1 == Math::mod(1, 3));
}

TEST_CASE("1 % -3 is correct") {
    REQUIRE(-2 == Math::mod(1, -3));
}

TEST_CASE("-1 % 3 is correct") {
    REQUIRE(2 == Math::mod(-1, 3));
}

TEST_CASE("-1 % -3 is correct") {
    REQUIRE(-1 == Math::mod(-1, -3));
}

// ---------------------------------------------------------------------------
// MARK: `squared`
// ---------------------------------------------------------------------------

TEST_CASE("squared(0) = 0") {
    REQUIRE(0 == Math::squared(0));
}

TEST_CASE("squared(1) = 1") {
    REQUIRE(1 == Math::squared(1));
}

TEST_CASE("squared(-1) = 1") {
    REQUIRE(1 == Math::squared(-1));
}

TEST_CASE("squared(2) = 4") {
    REQUIRE(4 == Math::squared(2));
}

TEST_CASE("squared(-2) = 4") {
    REQUIRE(4 == Math::squared(-2));
}

TEST_CASE("squared(0.5) = 0.25") {
    REQUIRE(0.25 == Math::squared(0.5));
}

TEST_CASE("squared(-0.5) = 0.25") {
    REQUIRE(0.25 == Math::squared(-0.5));
}

// ---------------------------------------------------------------------------
// MARK: `cubed`
// ---------------------------------------------------------------------------

TEST_CASE("cubed(0) = 0") {
    REQUIRE(0 == Math::cubed(0));
}

TEST_CASE("cubed(1) = 1") {
    REQUIRE(1 == Math::cubed(1));
}

TEST_CASE("cubed(-1) = -1") {
    REQUIRE(-1 == Math::cubed(-1));
}

TEST_CASE("cubed(2) = 8") {
    REQUIRE(8 == Math::cubed(2));
}

TEST_CASE("cubed(-2) = -8") {
    REQUIRE(-8 == Math::cubed(-2));
}

TEST_CASE("cubed(0.5) = 0.125") {
    REQUIRE(0.125 == Math::cubed(0.5));
}

TEST_CASE("cubed(-0.5) = -0.125") {
    REQUIRE(-0.125 == Math::cubed(-0.5));
}

// ---------------------------------------------------------------------------
// MARK: `powT2i`
// ---------------------------------------------------------------------------

// 2**x

TEST_CASE("powT2i should calculate 2**0") {
    REQUIRE(1.f == Math::powT2i(2.f, 0));
}

TEST_CASE("powT2i should calculate 2**1") {
    REQUIRE(2.f == Math::powT2i(2.f, 1));
}

TEST_CASE("powT2i should calculate 2**2") {
    REQUIRE(4.f == Math::powT2i(2.f, 2));
}

TEST_CASE("powT2i should calculate 2**3") {
    REQUIRE(8.f == Math::powT2i(2.f, 3));
}

TEST_CASE("powT2i should calculate 2**-1") {
    REQUIRE(1.f / 2.f == Approx(Math::powT2i(2.f, -1)));
}

TEST_CASE("powT2i should calculate 2**-2") {
    REQUIRE(1.f / 4.f == Approx(Math::powT2i(2.f, -2)));
}

TEST_CASE("powT2i should calculate 2**-3") {
    REQUIRE(1.f / 8.f == Approx(Math::powT2i(2.f, -3)));
}

TEST_CASE("powT2i should calculate 2**127") {
    REQUIRE(170141183460469231731687303715884105728.f == Math::powT2i(2.f, 127));
}

TEST_CASE("powT2i should calculate 2**-32") {
    REQUIRE(2.3283064365386963e-10 == Approx(Math::powT2i(2.f, -32)));
}

TEST_CASE("powT2i should calculate 2**-64") {
    REQUIRE(5.421010862427522e-20 == Approx(Math::powT2i(2.f, -64)));
}

// 3**x

TEST_CASE("powT2i should calculate 3**0") {
    REQUIRE(1.f == Math::powT2i(3.f, 0));
}

TEST_CASE("powT2i should calculate 3**1") {
    REQUIRE(3.f == Math::powT2i(3.f, 1));
}

TEST_CASE("powT2i should calculate 3**2") {
    REQUIRE(9.f == Math::powT2i(3.f, 2));
}

TEST_CASE("powT2i should calculate 3**3") {
    REQUIRE(27.f == Math::powT2i(3.f, 3));
}

TEST_CASE("powT2i should calculate 3**-1") {
    REQUIRE(1.f / 3.f == Approx(Math::powT2i(3.f, -1)));
}

TEST_CASE("powT2i should calculate 3**-2") {
    REQUIRE(1.f / 9.f == Approx(Math::powT2i(3.f, -2)));
}

TEST_CASE("powT2i should calculate 3**-3") {
    REQUIRE(1.f / 27.f == Approx(Math::powT2i(3.f, -3)));
}

// ---------------------------------------------------------------------------
// MARK: `exp2shift`
// ---------------------------------------------------------------------------

TEST_CASE("exp2shift should calculate 2**0") {
    REQUIRE(1.f == Math::exp2shift(0));
}

TEST_CASE("exp2shift should calculate 2**1") {
    REQUIRE(2.f == Math::exp2shift(1));
}

TEST_CASE("exp2shift should calculate 2**2") {
    REQUIRE(4.f == Math::exp2shift(2));
}

TEST_CASE("exp2shift should calculate 2**3") {
    REQUIRE(8.f == Math::exp2shift(3));
}

TEST_CASE("exp2shift should calculate 2**31") {
    REQUIRE(2147483648.f == Math::exp2shift(31));
}

TEST_CASE("exp2shift should not calculate 2**32") {
    REQUIRE_THROWS(Math::exp2shift(32));
}

TEST_CASE("exp2shift should calculate 2**-1") {
    REQUIRE(1.f / 2.f == Approx(Math::exp2shift(-1)));
}

TEST_CASE("exp2shift should calculate 2**-2") {
    REQUIRE(1.f / 4.f == Approx(Math::exp2shift(-2)));
}

TEST_CASE("exp2shift should calculate 2**-3") {
    REQUIRE(1.f / 8.f == Approx(Math::exp2shift(-3)));
}

TEST_CASE("exp2shift should calculate 2**-32") {
    REQUIRE(4.656612873077393e-10 == Approx(Math::exp2shift(-31)));
}

TEST_CASE("exp2shift should not calculate 2**-32") {
    REQUIRE_THROWS(Math::exp2shift(-32));
}

// ---------------------------------------------------------------------------
// MARK: `stirlings_formula`
// ---------------------------------------------------------------------------

SCENARIO("n! needs to be approximated using Stirling's Formula") {
    WHEN("1! is computed") {
        THEN("~1 is returned") {
            REQUIRE(1.f == round(Math::stirlings_formula(1.f)));
        }
    }
    WHEN("2! is computed") {
        THEN("~2 is returned") {
            REQUIRE(2.f == round(Math::stirlings_formula(2.f)));
        }
    }
    WHEN("3! is computed") {
        THEN("~6 is returned") {
            REQUIRE(6.f == round(Math::stirlings_formula(3.f)));
        }
    }
    WHEN("4! is computed") {
        THEN("~24 is returned") {
            REQUIRE(24.f == round(Math::stirlings_formula(4.f)));
        }
    }
    WHEN("5! is computed") {
        THEN("~120 is returned") {
            REQUIRE(118.f == round(Math::stirlings_formula(5.f)));
        }
    }
    WHEN("6! is computed") {
        THEN("~720 is returned") {
            REQUIRE(710.f == round(Math::stirlings_formula(6.f)));
        }
    }
    WHEN("7! is computed") {
        THEN("~5040 is returned") {
            REQUIRE(4980.f == round(Math::stirlings_formula(7.f)));
        }
    }
    WHEN("8! is computed") {
        THEN("~40320 is returned") {
            REQUIRE(39902.f == round(Math::stirlings_formula(8.f)));
        }
    }
    WHEN("9! is computed") {
        THEN("~362880 is returned") {
            REQUIRE(359537.f == round(Math::stirlings_formula(9.f)));
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: `amplitude2decibels`
// ---------------------------------------------------------------------------

// Float

TEST_CASE("amplitude2decibels<float> should convert 1 to 0dB") {
    REQUIRE(0.f == Math::amplitude2decibels<float>(1.f));
}

TEST_CASE("amplitude2decibels<double> should convert 1 to 0dB") {
    REQUIRE(0.0 == Math::amplitude2decibels<double>(1.0));
}

TEST_CASE("amplitude2decibels<float> should convert 0.5 to -6dB") {
    REQUIRE(Math::IEEE754::approx_equal<float>(-6.f, Math::amplitude2decibels<float>(0.5f), 1e-1f));
}

TEST_CASE("amplitude2decibels<double> should convert 0.5 to -6dB") {
    REQUIRE(Math::IEEE754::approx_equal<float>(-6.0, Math::amplitude2decibels<double>(0.5), 1e-1));
}

TEST_CASE("amplitude2decibels<float> should convert 1e-3 to -60dB") {
    REQUIRE(-60.f == Math::amplitude2decibels<float>(1e-3f));
}

TEST_CASE("amplitude2decibels<double> should convert 1e-3 to -60dB") {
    REQUIRE(-60.0 == Math::amplitude2decibels<double>(1e-3));
}

TEST_CASE("amplitude2decibels<float> should convert 1e-6 to -120dB") {
    REQUIRE(-120.f == Math::amplitude2decibels<float>(1e-6f));
}

TEST_CASE("amplitude2decibels<double> should convert 1e-6 to -120dB") {
    REQUIRE(Math::IEEE754::approx_equal<double>(-120.0, Math::amplitude2decibels<double>(1e-6f), 1e-6));
}

TEST_CASE("amplitude2decibels<float> should convert 2 to 6dB") {
    REQUIRE(Math::IEEE754::approx_equal<float>(6.f, Math::amplitude2decibels<float>(2.f), 1e-1f));
}

TEST_CASE("amplitude2decibels<double> should convert 2 to 6dB") {
    REQUIRE(Math::IEEE754::approx_equal<float>(6.0, Math::amplitude2decibels<double>(2.0), 1e-1));
}

TEST_CASE("amplitude2decibels<float> should convert 0 to -infinity dB") {
    REQUIRE(-std::numeric_limits<float>::infinity() == Math::amplitude2decibels<float>(0.f));
}

TEST_CASE("amplitude2decibels<double> should convert 0 to -infinity dB") {
    REQUIRE(-std::numeric_limits<double>::infinity() == Math::amplitude2decibels<double>(0.0));
}

// ---------------------------------------------------------------------------
// MARK: `decibels2amplitude`
// ---------------------------------------------------------------------------

TEST_CASE("decibels2amplitude<float> should convert 0dB to 1") {
    REQUIRE(1.f == Math::decibels2amplitude<float>(0.f));
}

TEST_CASE("decibels2amplitude<double> should convert 0dB to 1") {
    REQUIRE(1.0 == Math::decibels2amplitude<float>(0.0));
}

TEST_CASE("decibels2amplitude<float> should convert -6dB to 0.5") {
    REQUIRE(Math::IEEE754::approx_equal<float>(0.5f, Math::decibels2amplitude<float>(-6.f), 1e-2f));
}

TEST_CASE("decibels2amplitude<double> should convert -6dB to 0.5") {
    REQUIRE(Math::IEEE754::approx_equal<float>(0.5, Math::decibels2amplitude<float>(-6.0), 1e-2f));
}

TEST_CASE("decibels2amplitude<float> should convert -60dB to 1e-3") {
    REQUIRE(1e-3f == Math::decibels2amplitude<float>(-60.f));
}

TEST_CASE("decibels2amplitude<double> should convert -60dB to 1e-3") {
    REQUIRE(1e-3 == Math::decibels2amplitude<double>(-60.0));
}

TEST_CASE("decibels2amplitude<float> should convert -120dB to 1e-6") {
    REQUIRE(1e-6f == Math::decibels2amplitude<float>(-120.f));
}

TEST_CASE("decibels2amplitude<double> should convert -120dB to 1e-6") {
    REQUIRE(1e-6 == Math::decibels2amplitude<double>(-120.0));
}

TEST_CASE("decibels2amplitude<float> should convert 6dB to 2") {
    REQUIRE(Math::IEEE754::approx_equal<float>(2.f, Math::decibels2amplitude<float>(6.f), 1e-2f));
}

TEST_CASE("decibels2amplitude<double> should convert 6dB to 2") {
    REQUIRE(Math::IEEE754::approx_equal<float>(2.0, Math::decibels2amplitude<float>(6.0), 1e-2f));
}

TEST_CASE("decibels2amplitude<float> should convert -infinity dB to 0") {
    REQUIRE(0.f == Math::decibels2amplitude<float>(-std::numeric_limits<float>::infinity()));
}

TEST_CASE("decibels2amplitude<double> should convert -infinity dB to 0") {
    REQUIRE(0.f == Math::decibels2amplitude<double>(-std::numeric_limits<double>::infinity()));
}

// ---------------------------------------------------------------------------
// MARK: `quantize`
// ---------------------------------------------------------------------------

TEST_CASE("should quantize positive values to 1-bit") {
    REQUIRE(0.f / 1.f == Math::quantize(0.0f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.1f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.2f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.3f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.4f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.5f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.6f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.7f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.8f, 1));
    REQUIRE(0.f / 1.f == Math::quantize(0.9f, 1));
    REQUIRE(1.f / 1.f == Math::quantize(1.0f, 1));
}

TEST_CASE("should quantize negative values to 1-bit") {
    REQUIRE(0.f / -1.f == Math::quantize(-0.0f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.1f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.2f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.3f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.4f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.5f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.6f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.7f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.8f, 1));
    REQUIRE(0.f / -1.f == Math::quantize(-0.9f, 1));
    REQUIRE(1.f / -1.f == Math::quantize(-1.0f, 1));
}

TEST_CASE("should quantize positive values to 2-bit") {
    REQUIRE(0.f / 3.f == Math::quantize(0.0f, 2));
    REQUIRE(0.f / 3.f == Math::quantize(0.1f, 2));
    REQUIRE(0.f / 3.f == Math::quantize(0.2f, 2));
    REQUIRE(0.f / 3.f == Math::quantize(0.3f, 2));
    REQUIRE(1.f / 3.f == Math::quantize(0.4f, 2));
    REQUIRE(1.f / 3.f == Math::quantize(0.5f, 2));
    REQUIRE(1.f / 3.f == Math::quantize(0.6f, 2));
    REQUIRE(2.f / 3.f == Math::quantize(0.7f, 2));
    REQUIRE(2.f / 3.f == Math::quantize(0.8f, 2));
    REQUIRE(2.f / 3.f == Math::quantize(0.9f, 2));
    REQUIRE(3.f / 3.f == Math::quantize(1.0f, 2));
}

TEST_CASE("should quantize negative values to 2-bit") {
    REQUIRE(0.f / -3.f == Math::quantize(-0.0f, 2));
    REQUIRE(0.f / -3.f == Math::quantize(-0.1f, 2));
    REQUIRE(0.f / -3.f == Math::quantize(-0.2f, 2));
    REQUIRE(0.f / -3.f == Math::quantize(-0.3f, 2));
    REQUIRE(1.f / -3.f == Math::quantize(-0.4f, 2));
    REQUIRE(1.f / -3.f == Math::quantize(-0.5f, 2));
    REQUIRE(1.f / -3.f == Math::quantize(-0.6f, 2));
    REQUIRE(2.f / -3.f == Math::quantize(-0.7f, 2));
    REQUIRE(2.f / -3.f == Math::quantize(-0.8f, 2));
    REQUIRE(2.f / -3.f == Math::quantize(-0.9f, 2));
    REQUIRE(3.f / -3.f == Math::quantize(-1.0f, 2));
}

TEST_CASE("should quantize positive values to 3-bit") {
    REQUIRE(0.f / 7.f == Math::quantize(0.0f, 3));
    REQUIRE(0.f / 7.f == Math::quantize(0.1f, 3));
    REQUIRE(1.f / 7.f == Math::quantize(0.2f, 3));
    REQUIRE(2.f / 7.f == Math::quantize(0.3f, 3));
    REQUIRE(2.f / 7.f == Math::quantize(0.4f, 3));
    REQUIRE(3.f / 7.f == Math::quantize(0.5f, 3));
    REQUIRE(4.f / 7.f == Math::quantize(0.6f, 3));
    REQUIRE(4.f / 7.f == Math::quantize(0.7f, 3));
    REQUIRE(5.f / 7.f == Math::quantize(0.8f, 3));
    REQUIRE(6.f / 7.f == Math::quantize(0.9f, 3));
    REQUIRE(7.f / 7.f == Math::quantize(1.0f, 3));
}

TEST_CASE("should quantize negative values to 3-bit") {
    REQUIRE(0.f / -7.f == Math::quantize(-0.0f, 3));
    REQUIRE(0.f / -7.f == Math::quantize(-0.1f, 3));
    REQUIRE(1.f / -7.f == Math::quantize(-0.2f, 3));
    REQUIRE(2.f / -7.f == Math::quantize(-0.3f, 3));
    REQUIRE(2.f / -7.f == Math::quantize(-0.4f, 3));
    REQUIRE(3.f / -7.f == Math::quantize(-0.5f, 3));
    REQUIRE(4.f / -7.f == Math::quantize(-0.6f, 3));
    REQUIRE(4.f / -7.f == Math::quantize(-0.7f, 3));
    REQUIRE(5.f / -7.f == Math::quantize(-0.8f, 3));
    REQUIRE(6.f / -7.f == Math::quantize(-0.9f, 3));
    REQUIRE(7.f / -7.f == Math::quantize(-1.0f, 3));
}

TEST_CASE("should quantize positive values to 4-bit") {
    REQUIRE(0.f  / 15.f == Math::quantize(0.0f, 4));
    REQUIRE(1.f  / 15.f == Math::quantize(0.1f, 4));
    REQUIRE(3.f  / 15.f == Math::quantize(0.2f, 4));
    REQUIRE(4.f  / 15.f == Math::quantize(0.3f, 4));
    REQUIRE(6.f  / 15.f == Math::quantize(0.4f, 4));
    REQUIRE(7.f  / 15.f == Math::quantize(0.5f, 4));
    REQUIRE(9.f  / 15.f == Math::quantize(0.6f, 4));
    REQUIRE(10.f / 15.f == Math::quantize(0.7f, 4));
    REQUIRE(12.f / 15.f == Math::quantize(0.8f, 4));
    REQUIRE(13.f / 15.f == Math::quantize(0.9f, 4));
    REQUIRE(15.f / 15.f == Math::quantize(1.0f, 4));
}

TEST_CASE("should quantize negative values to 4-bit") {
    REQUIRE(0.f  / -15.f == Math::quantize(-0.0f, 4));
    REQUIRE(1.f  / -15.f == Math::quantize(-0.1f, 4));
    REQUIRE(3.f  / -15.f == Math::quantize(-0.2f, 4));
    REQUIRE(4.f  / -15.f == Math::quantize(-0.3f, 4));
    REQUIRE(6.f  / -15.f == Math::quantize(-0.4f, 4));
    REQUIRE(7.f  / -15.f == Math::quantize(-0.5f, 4));
    REQUIRE(9.f  / -15.f == Math::quantize(-0.6f, 4));
    REQUIRE(10.f / -15.f == Math::quantize(-0.7f, 4));
    REQUIRE(12.f / -15.f == Math::quantize(-0.8f, 4));
    REQUIRE(13.f / -15.f == Math::quantize(-0.9f, 4));
    REQUIRE(15.f / -15.f == Math::quantize(-1.0f, 4));
}

// ---------------------------------------------------------------------------
// MARK: `interpolate`
// ---------------------------------------------------------------------------

TEST_CASE("should interpolate between 0 and 1") {
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, Math::interpolate(0.f, 1.f, 0.f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.1f, Math::interpolate(0.f, 1.f, 0.1f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.5f, Math::interpolate(0.f, 1.f, 0.5f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.9f, Math::interpolate(0.f, 1.f, 0.9f)));
    REQUIRE(Math::IEEE754::epsilon_equal(1.f, Math::interpolate(0.f, 1.f, 1.f)));
}

TEST_CASE("should interpolate between 1 and 0") {
    REQUIRE(Math::IEEE754::epsilon_equal(1.f, Math::interpolate(1.f, 0.f, 0.f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.9f, Math::interpolate(1.f, 0.f, 0.1f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.5f, Math::interpolate(1.f, 0.f, 0.5f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.1f, Math::interpolate(1.f, 0.f, 0.9f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, Math::interpolate(1.f, 0.f, 1.f)));
}

TEST_CASE("should interpolate between 0 and -1") {
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, Math::interpolate(0.f, -1.f, 0.f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.1f, Math::interpolate(0.f, -1.f, 0.1f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.5f, Math::interpolate(0.f, -1.f, 0.5f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.9f, Math::interpolate(0.f, -1.f, 0.9f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-1.f, Math::interpolate(0.f, -1.f, 1.f)));
}

TEST_CASE("should interpolate between -1 and 0") {
    REQUIRE(Math::IEEE754::epsilon_equal(-1.f, Math::interpolate(-1.f, 0.f, 0.f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.9f, Math::interpolate(-1.f, 0.f, 0.1f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.5f, Math::interpolate(-1.f, 0.f, 0.5f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.1f, Math::interpolate(-1.f, 0.f, 0.9f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, Math::interpolate(-1.f, 0.f, 1.f)));
}

// ---------------------------------------------------------------------------
// MARK: `argmax`
// ---------------------------------------------------------------------------

SCENARIO("the maximum argument needs to be found in an array") {
    // int
    GIVEN("an array of integers of equal value (0)") {
        std::vector<int> sequence{0, 0, 0, 0, 0};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of integers of equal value (-1)") {
        std::vector<int> sequence{-1, -1, -1, -1, -1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of integers of equal value (1)") {
        std::vector<int> sequence{1, 1, 1, 1, 1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of integers in ascending order") {
        std::vector<int> sequence{1, 2, 3, 4, 5};
        WHEN("the argmax is calculated") {
            THEN("4 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(4 == idx);
            }
        }
    }
    GIVEN("an array of integers in descending order") {
        std::vector<int> sequence{5, 4, 3, 2, 1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of integers in triangular order") {
        std::vector<int> sequence{1, 2, 3, 2, 1};
        WHEN("the argmax is calculated") {
            THEN("2 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(2 == idx);
            }
        }
    }
    // float
    GIVEN("an array of floats of equal value (0)") {
        std::vector<float> sequence{0, 0, 0, 0, 0};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats of equal value (-1)") {
        std::vector<float> sequence{-1, -1, -1, -1, -1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats of equal value (1)") {
        std::vector<float> sequence{1, 1, 1, 1, 1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats in ascending order") {
        std::vector<float> sequence{1, 2, 3, 4, 5};
        WHEN("the argmax is calculated") {
            THEN("4 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(4 == idx);
            }
        }
    }
    GIVEN("an array of floats in descending order") {
        std::vector<float> sequence{5, 4, 3, 2, 1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats in triangular order") {
        std::vector<float> sequence{1, 2, 3, 2, 1};
        WHEN("the argmax is calculated") {
            THEN("2 is returned") {
                const auto idx = Math::argmax(sequence.data(), sequence.size());
                REQUIRE(2 == idx);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: `argmin`
// ---------------------------------------------------------------------------

SCENARIO("the minimum argument needs to be found in an array") {
    // int
    GIVEN("an array of integers of equal value (0)") {
        std::vector<int> sequence{0, 0, 0, 0, 0};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of integers of equal value (-1)") {
        std::vector<int> sequence{-1, -1, -1, -1, -1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of integers of equal value (1)") {
        std::vector<int> sequence{1, 1, 1, 1, 1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of integers in ascending order") {
        std::vector<int> sequence{1, 2, 3, 4, 5};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of integers in descending order") {
        std::vector<int> sequence{5, 4, 3, 2, 1};
        WHEN("the argmin is calculated") {
            THEN("4 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(4 == idx);
            }
        }
    }
    GIVEN("an array of integers in triangular order") {
        std::vector<int> sequence{1, 2, 3, 2, 1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    // float
    GIVEN("an array of floats of equal value (0)") {
        std::vector<float> sequence{0, 0, 0, 0, 0};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats of equal value (-1)") {
        std::vector<float> sequence{-1, -1, -1, -1, -1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats of equal value (1)") {
        std::vector<float> sequence{1, 1, 1, 1, 1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats in ascending order") {
        std::vector<float> sequence{1, 2, 3, 4, 5};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats in descending order") {
        std::vector<float> sequence{5, 4, 3, 2, 1};
        WHEN("the argmin is calculated") {
            THEN("4 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(4 == idx);
            }
        }
    }
    GIVEN("an array of floats in triangular order") {
        std::vector<float> sequence{1, 2, 3, 2, 1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = Math::argmin(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
}
