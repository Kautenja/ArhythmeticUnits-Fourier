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

#include "dsp/math/functions.h"
// #include "dsp/math/ieee754.h"
#include "dsp/math/ieee754.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

SCENARIO("The modulo operator needs to be computed") {
    GIVEN("expression of the form \"a mod 2\"") {
        WHEN("The modulo is computed") {
            THEN("The correct value is returned") {
                REQUIRE(1 == mod(1, 2));
                REQUIRE(-1 == mod(1, -2));
                REQUIRE(1 == mod(1, 2));
                REQUIRE(-1 == mod(-1, -2));
                REQUIRE(0 == mod(2, 2));
                REQUIRE(1 == mod(3, 2));
            }
        }
    }
    GIVEN("expressions of the form \"a mod 3\"") {
        WHEN("The modulo is computed") {
            THEN("The correct value is returned") {
                REQUIRE(1 == mod(1, 3));
                REQUIRE(-2 == mod(1, -3));
                REQUIRE(2 == mod(-1, 3));
                REQUIRE(-1 == mod(-1, -3));
            }
        }
    }
}

SCENARIO("The sign of floating point numbers needs to be determined") {
    WHEN("The sign of positive floating point numbers is computed") {
        THEN("The number 1 is returned") {
            REQUIRE(1.f == sgnf(5.f));
            REQUIRE(1.f == sgnf(1e-6f));
        }
    }
    WHEN("The sign of negative floating point numbers is computed") {
        THEN("The number -1 is returned") {
            REQUIRE(-1.f == sgnf(-5.f));
            REQUIRE(-1.f == sgnf(-1e-6f));
        }
    }
    WHEN("The sign of 0.f is computed") {
        THEN("The number 0 is returned") {
            REQUIRE(0.f == sgnf(0.f));
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: `maxf`
// ---------------------------------------------------------------------------

TEST_CASE("maxf should return the maximum of two positive floats") {
    REQUIRE(5.f == maxf(4.f, 5.f));
    REQUIRE(5.f == maxf(5.f, 4.f));
}

TEST_CASE("maxf should return the maximum of two negative floats") {
    REQUIRE(-4.f == maxf(-4.f, -5.f));
    REQUIRE(-4.f == maxf(-5.f, -4.f));
}

TEST_CASE("maxf should return the maximum of a negative and positive floats") {
    REQUIRE(4.f == maxf(4.f, -5.f));
    REQUIRE(4.f == maxf(-5.f, 4.f));
}

TEST_CASE("maxf should return the maximum of 0 and a positive float") {
    REQUIRE(4.f == maxf(4.f, 0.f));
    REQUIRE(4.f == maxf(0.f, 4.f));
}

TEST_CASE("maxf should return the maximum of 0 and a negative float") {
    REQUIRE(0.f == maxf(-4.f, 0.f));
    REQUIRE(0.f == maxf(0.f, -4.f));
}

// ---------------------------------------------------------------------------
// MARK: `minf`
// ---------------------------------------------------------------------------

TEST_CASE("minf should return the minimum of two positive floats") {
    REQUIRE(4.f == minf(4.f, 5.f));
    REQUIRE(4.f == minf(5.f, 4.f));
}

TEST_CASE("minf should return the minimum of two negative floats") {
    REQUIRE(-5.f == minf(-4.f, -5.f));
    REQUIRE(-5.f == minf(-5.f, -4.f));
}

TEST_CASE("minf should return the minimum of a negative and positive floats") {
    REQUIRE(-5.f == minf(4.f, -5.f));
    REQUIRE(-5.f == minf(-5.f, 4.f));
}

TEST_CASE("minf should return the minimum of 0 and a positive float") {
    REQUIRE(0.f == minf(4.f, 0.f));
    REQUIRE(0.f == minf(0.f, 4.f));
}

TEST_CASE("minf should return the minimum of 0 and a negative float") {
    REQUIRE(-4.f == minf(-4.f, 0.f));
    REQUIRE(-4.f == minf(0.f, -4.f));
}

// ---------------------------------------------------------------------------
// MARK: `clampf`
// ---------------------------------------------------------------------------

TEST_CASE("clampf should not clip values within positive range") {
    REQUIRE(4.5f == clampf(4.5f, 4.f, 5.f));
}

TEST_CASE("clampf should not clip values within negative range") {
    REQUIRE(-4.5f == clampf(-4.5f, -5.f, -4.f));
}

TEST_CASE("clampf should clip to a null positive range") {
    REQUIRE(4.f == clampf(0.f, 4.f, 4.f));
}

TEST_CASE("clampf should clip to a null negative range") {
    REQUIRE(-4.f == clampf(0.f, -4.f, -4.f));
}

TEST_CASE("clampf should clip to a null 0 range") {
    REQUIRE(0.f == clampf(1.f, 0.f, 0.f));
}

TEST_CASE("clampf should clip to the minimum bound of a positive range") {
    REQUIRE(4.f == clampf(0.f, 4.f, 5.f));
}

TEST_CASE("clampf should clip to the maximum bound of a positive range") {
    REQUIRE(5.f == clampf(6.f, 4.f, 5.f));
}

TEST_CASE("clampf should clip to the minimum bound of a negative range") {
    REQUIRE(-5.f == clampf(-6.f, -5.f, -4.f));
}

TEST_CASE("clampf should clip to the maximum bound of a negative range") {
    REQUIRE(-4.f == clampf(0.f, -5.f, -4.f));
}

TEST_CASE("clampf should not panic if lo > hi") {
    clampf(0.f, 5.f, 4.f);
}

// ---------------------------------------------------------------------------
// MARK: `squaref`
// ---------------------------------------------------------------------------

TEST_CASE("squaref(0) = 0") {
    REQUIRE(0 == squaref(0));
}

TEST_CASE("squaref(1) = 1") {
    REQUIRE(1 == squaref(1));
}

TEST_CASE("squaref(-1) = 1") {
    REQUIRE(1 == squaref(-1));
}

TEST_CASE("squaref(2) = 4") {
    REQUIRE(4 == squaref(2));
}

TEST_CASE("squaref(-2) = 4") {
    REQUIRE(4 == squaref(-2));
}

TEST_CASE("squaref(0.5) = 0.25") {
    REQUIRE(0.25 == squaref(0.5));
}

TEST_CASE("squaref(-0.5) = 0.25") {
    REQUIRE(0.25 == squaref(-0.5));
}

// ---------------------------------------------------------------------------
// MARK: `cubef`
// ---------------------------------------------------------------------------

TEST_CASE("cubef(0) = 0") {
    REQUIRE(0 == cubef(0));
}

TEST_CASE("cubef(1) = 1") {
    REQUIRE(1 == cubef(1));
}

TEST_CASE("cubef(-1) = -1") {
    REQUIRE(-1 == cubef(-1));
}

TEST_CASE("cubef(2) = 8") {
    REQUIRE(8 == cubef(2));
}

TEST_CASE("cubef(-2) = -8") {
    REQUIRE(-8 == cubef(-2));
}

TEST_CASE("cubef(0.5) = 0.125") {
    REQUIRE(0.125 == cubef(0.5));
}

TEST_CASE("cubef(-0.5) = -0.125") {
    REQUIRE(-0.125 == cubef(-0.5));
}

// ---------------------------------------------------------------------------
// MARK: `powfi`
// ---------------------------------------------------------------------------

// 2**x

TEST_CASE("powfi should calculate 2**0") {
    REQUIRE(1.f == powfi(2.f, 0));
}

TEST_CASE("powfi should calculate 2**1") {
    REQUIRE(2.f == powfi(2.f, 1));
}

TEST_CASE("powfi should calculate 2**2") {
    REQUIRE(4.f == powfi(2.f, 2));
}

TEST_CASE("powfi should calculate 2**3") {
    REQUIRE(8.f == powfi(2.f, 3));
}

TEST_CASE("powfi should calculate 2**-1") {
    REQUIRE(1.f / 2.f == Approx(powfi(2.f, -1)));
}

TEST_CASE("powfi should calculate 2**-2") {
    REQUIRE(1.f / 4.f == Approx(powfi(2.f, -2)));
}

TEST_CASE("powfi should calculate 2**-3") {
    REQUIRE(1.f / 8.f == Approx(powfi(2.f, -3)));
}

TEST_CASE("powfi should calculate 2**127") {
    REQUIRE(170141183460469231731687303715884105728.f == powfi(2.f, 127));
}

TEST_CASE("powfi should calculate 2**-32") {
    REQUIRE(2.3283064365386963e-10 == Approx(powfi(2.f, -32)));
}

TEST_CASE("powfi should calculate 2**-64") {
    REQUIRE(5.421010862427522e-20 == Approx(powfi(2.f, -64)));
}

// 3**x

TEST_CASE("powfi should calculate 3**0") {
    REQUIRE(1.f == powfi(3.f, 0));
}

TEST_CASE("powfi should calculate 3**1") {
    REQUIRE(3.f == powfi(3.f, 1));
}

TEST_CASE("powfi should calculate 3**2") {
    REQUIRE(9.f == powfi(3.f, 2));
}

TEST_CASE("powfi should calculate 3**3") {
    REQUIRE(27.f == powfi(3.f, 3));
}

TEST_CASE("powfi should calculate 3**-1") {
    REQUIRE(1.f / 3.f == Approx(powfi(3.f, -1)));
}

TEST_CASE("powfi should calculate 3**-2") {
    REQUIRE(1.f / 9.f == Approx(powfi(3.f, -2)));
}

TEST_CASE("powfi should calculate 3**-3") {
    REQUIRE(1.f / 27.f == Approx(powfi(3.f, -3)));
}

// ---------------------------------------------------------------------------
// MARK: `exp2fi`
// ---------------------------------------------------------------------------

TEST_CASE("exp2fi should calculate 2**0") {
    REQUIRE(exp2fi(0) == 1.f);
}

TEST_CASE("exp2fi should calculate 2**1") {
    REQUIRE(exp2fi(1) == 2.f);
}

TEST_CASE("exp2fi should calculate 2**2") {
    REQUIRE(exp2fi(2) == 4.f);
}

TEST_CASE("exp2fi should calculate 2**3") {
    REQUIRE(exp2fi(3) == 8.f);
}

TEST_CASE("exp2fi should calculate 2**31") {
    REQUIRE(exp2fi(31) == 2147483648.f);
}

TEST_CASE("exp2fi should be undefined for 2**32") {
    REQUIRE(exp2fi(32) != Approx(4294967296.f));
}

TEST_CASE("exp2fi should calculate 2**-1") {
    REQUIRE(exp2fi(-1) == Approx(1.f / 2.f));
}

TEST_CASE("exp2fi should calculate 2**-2") {
    REQUIRE(exp2fi(-2) == Approx(1.f / 4.f));
}

TEST_CASE("exp2fi should calculate 2**-3") {
    REQUIRE(exp2fi(-3) == Approx(1.f / 8.f));
}

TEST_CASE("exp2fi should calculate 2**-31") {
    REQUIRE(exp2fi(-31) == Approx(4.656612873077393e-10));
}

TEST_CASE("exp2fi should be undefined for 2**-32") {
    REQUIRE(exp2fi(-32) != Approx(0.000000000232831f));
}

// ---------------------------------------------------------------------------
// MARK: `factorialf`
// ---------------------------------------------------------------------------

SCENARIO("n! needs to be approximated using Stirling's Formula") {
    WHEN("0! is computed") {
        THEN("1 is returned") {
            REQUIRE(1.f == factorialf(0));
        }
    }
    WHEN("1! is computed") {
        THEN("~1 is returned") {
            REQUIRE(1.f == round(factorialf(1)));
        }
    }
    WHEN("2! is computed") {
        THEN("~2 is returned") {
            REQUIRE(2.f == round(factorialf(2)));
        }
    }
    WHEN("3! is computed") {
        THEN("~6 is returned") {
            REQUIRE(6.f == round(factorialf(3)));
        }
    }
    WHEN("4! is computed") {
        THEN("~24 is returned") {
            REQUIRE(24.f == round(factorialf(4)));
        }
    }
    WHEN("5! is computed") {
        THEN("~120 is returned") {
            REQUIRE(118.f == round(factorialf(5)));
        }
    }
    WHEN("6! is computed") {
        THEN("~720 is returned") {
            REQUIRE(710.f == round(factorialf(6)));
        }
    }
    WHEN("7! is computed") {
        THEN("~5040 is returned") {
            REQUIRE(4980.f == round(factorialf(7)));
        }
    }
    WHEN("8! is computed") {
        THEN("~40320 is returned") {
            REQUIRE(39902.f == round(factorialf(8)));
        }
    }
    WHEN("9! is computed") {
        THEN("~362880 is returned") {
            REQUIRE(359537.f == round(factorialf(9)));
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: `amplitude_to_dbf`
// ---------------------------------------------------------------------------

TEST_CASE("amplitude_to_dbf should convert 1 to 0dB") {
    REQUIRE(0.f == amplitude_to_dbf(1.f));
}

TEST_CASE("amplitude_to_dbf should convert 0.5 to -6dB") {
    REQUIRE(Math::IEEE754::approx_equal<float>(-6.f, amplitude_to_dbf(0.5f), 1e-1f));
}

TEST_CASE("amplitude_to_dbf should convert 1e-3 to -60dB") {
    REQUIRE(-60.f == amplitude_to_dbf(1e-3f));
}

TEST_CASE("amplitude_to_dbf should convert 1e-6 to -120dB") {
    REQUIRE(-120.f == amplitude_to_dbf(1e-6f));
}

TEST_CASE("amplitude_to_dbf should convert 2 to 6dB") {
    REQUIRE(Math::IEEE754::approx_equal<float>(6.f, amplitude_to_dbf(2.f), 1e-1f));
}

TEST_CASE("amplitude_to_dbf should convert 0 to -infinity dB") {
    REQUIRE(-std::numeric_limits<float>::infinity() == amplitude_to_dbf(0.f));
}

// ---------------------------------------------------------------------------
// MARK: `db_to_amplitudef`
// ---------------------------------------------------------------------------

TEST_CASE("db_to_amplitudef should convert 0dB to 1") {
    REQUIRE(1.f == db_to_amplitudef(0.f));
}

TEST_CASE("db_to_amplitudef should convert -6dB to 0.5") {
    REQUIRE(Math::IEEE754::approx_equal<float>(0.5f, db_to_amplitudef(-6.f), 1e-2f));
}

TEST_CASE("db_to_amplitudef should convert -60dB to 1e-3") {
    REQUIRE(1e-3f == db_to_amplitudef(-60.f));
}

TEST_CASE("db_to_amplitudef should convert -120dB to 1e-6") {
    REQUIRE(1e-6f == db_to_amplitudef(-120.f));
}

TEST_CASE("db_to_amplitudef should convert 6dB to 2") {
    REQUIRE(Math::IEEE754::approx_equal<float>(2.f, db_to_amplitudef(6.f), 1e-2f));
}

TEST_CASE("db_to_amplitudef should convert -infinity dB to 0") {
    REQUIRE(0.f == db_to_amplitudef(-std::numeric_limits<float>::infinity()));
}

// ---------------------------------------------------------------------------
// MARK: `quantizef`
// ---------------------------------------------------------------------------

TEST_CASE("should quantize positive values to 1-bit") {
    REQUIRE(0.f / 1.f == quantizef(0.0f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.1f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.2f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.3f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.4f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.5f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.6f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.7f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.8f, 1));
    REQUIRE(0.f / 1.f == quantizef(0.9f, 1));
    REQUIRE(1.f / 1.f == quantizef(1.0f, 1));
}

TEST_CASE("should quantize negative values to 1-bit") {
    REQUIRE(0.f / -1.f == quantizef(-0.0f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.1f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.2f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.3f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.4f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.5f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.6f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.7f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.8f, 1));
    REQUIRE(0.f / -1.f == quantizef(-0.9f, 1));
    REQUIRE(1.f / -1.f == quantizef(-1.0f, 1));
}

TEST_CASE("should quantize positive values to 2-bit") {
    REQUIRE(0.f / 3.f == quantizef(0.0f, 2));
    REQUIRE(0.f / 3.f == quantizef(0.1f, 2));
    REQUIRE(0.f / 3.f == quantizef(0.2f, 2));
    REQUIRE(0.f / 3.f == quantizef(0.3f, 2));
    REQUIRE(1.f / 3.f == quantizef(0.4f, 2));
    REQUIRE(1.f / 3.f == quantizef(0.5f, 2));
    REQUIRE(1.f / 3.f == quantizef(0.6f, 2));
    REQUIRE(2.f / 3.f == quantizef(0.7f, 2));
    REQUIRE(2.f / 3.f == quantizef(0.8f, 2));
    REQUIRE(2.f / 3.f == quantizef(0.9f, 2));
    REQUIRE(3.f / 3.f == quantizef(1.0f, 2));
}

TEST_CASE("should quantize negative values to 2-bit") {
    REQUIRE(0.f / -3.f == quantizef(-0.0f, 2));
    REQUIRE(0.f / -3.f == quantizef(-0.1f, 2));
    REQUIRE(0.f / -3.f == quantizef(-0.2f, 2));
    REQUIRE(0.f / -3.f == quantizef(-0.3f, 2));
    REQUIRE(1.f / -3.f == quantizef(-0.4f, 2));
    REQUIRE(1.f / -3.f == quantizef(-0.5f, 2));
    REQUIRE(1.f / -3.f == quantizef(-0.6f, 2));
    REQUIRE(2.f / -3.f == quantizef(-0.7f, 2));
    REQUIRE(2.f / -3.f == quantizef(-0.8f, 2));
    REQUIRE(2.f / -3.f == quantizef(-0.9f, 2));
    REQUIRE(3.f / -3.f == quantizef(-1.0f, 2));
}

TEST_CASE("should quantize positive values to 3-bit") {
    REQUIRE(0.f / 7.f == quantizef(0.0f, 3));
    REQUIRE(0.f / 7.f == quantizef(0.1f, 3));
    REQUIRE(1.f / 7.f == quantizef(0.2f, 3));
    REQUIRE(2.f / 7.f == quantizef(0.3f, 3));
    REQUIRE(2.f / 7.f == quantizef(0.4f, 3));
    REQUIRE(3.f / 7.f == quantizef(0.5f, 3));
    REQUIRE(4.f / 7.f == quantizef(0.6f, 3));
    REQUIRE(4.f / 7.f == quantizef(0.7f, 3));
    REQUIRE(5.f / 7.f == quantizef(0.8f, 3));
    REQUIRE(6.f / 7.f == quantizef(0.9f, 3));
    REQUIRE(7.f / 7.f == quantizef(1.0f, 3));
}

TEST_CASE("should quantize negative values to 3-bit") {
    REQUIRE(0.f / -7.f == quantizef(-0.0f, 3));
    REQUIRE(0.f / -7.f == quantizef(-0.1f, 3));
    REQUIRE(1.f / -7.f == quantizef(-0.2f, 3));
    REQUIRE(2.f / -7.f == quantizef(-0.3f, 3));
    REQUIRE(2.f / -7.f == quantizef(-0.4f, 3));
    REQUIRE(3.f / -7.f == quantizef(-0.5f, 3));
    REQUIRE(4.f / -7.f == quantizef(-0.6f, 3));
    REQUIRE(4.f / -7.f == quantizef(-0.7f, 3));
    REQUIRE(5.f / -7.f == quantizef(-0.8f, 3));
    REQUIRE(6.f / -7.f == quantizef(-0.9f, 3));
    REQUIRE(7.f / -7.f == quantizef(-1.0f, 3));
}

TEST_CASE("should quantize positive values to 4-bit") {
    REQUIRE(0.f  / 15.f == quantizef(0.0f, 4));
    REQUIRE(1.f  / 15.f == quantizef(0.1f, 4));
    REQUIRE(3.f  / 15.f == quantizef(0.2f, 4));
    REQUIRE(4.f  / 15.f == quantizef(0.3f, 4));
    REQUIRE(6.f  / 15.f == quantizef(0.4f, 4));
    REQUIRE(7.f  / 15.f == quantizef(0.5f, 4));
    REQUIRE(9.f  / 15.f == quantizef(0.6f, 4));
    REQUIRE(10.f / 15.f == quantizef(0.7f, 4));
    REQUIRE(12.f / 15.f == quantizef(0.8f, 4));
    REQUIRE(13.f / 15.f == quantizef(0.9f, 4));
    REQUIRE(15.f / 15.f == quantizef(1.0f, 4));
}

TEST_CASE("should quantize negative values to 4-bit") {
    REQUIRE(0.f  / -15.f == quantizef(-0.0f, 4));
    REQUIRE(1.f  / -15.f == quantizef(-0.1f, 4));
    REQUIRE(3.f  / -15.f == quantizef(-0.2f, 4));
    REQUIRE(4.f  / -15.f == quantizef(-0.3f, 4));
    REQUIRE(6.f  / -15.f == quantizef(-0.4f, 4));
    REQUIRE(7.f  / -15.f == quantizef(-0.5f, 4));
    REQUIRE(9.f  / -15.f == quantizef(-0.6f, 4));
    REQUIRE(10.f / -15.f == quantizef(-0.7f, 4));
    REQUIRE(12.f / -15.f == quantizef(-0.8f, 4));
    REQUIRE(13.f / -15.f == quantizef(-0.9f, 4));
    REQUIRE(15.f / -15.f == quantizef(-1.0f, 4));
}

// ---------------------------------------------------------------------------
// MARK: `interpolatef`
// ---------------------------------------------------------------------------

TEST_CASE("should interpolate between 0 and 1") {
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, interpolatef(0.f, 1.f, 0.f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.1f, interpolatef(0.f, 1.f, 0.1f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.5f, interpolatef(0.f, 1.f, 0.5f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.9f, interpolatef(0.f, 1.f, 0.9f)));
    REQUIRE(Math::IEEE754::epsilon_equal(1.f, interpolatef(0.f, 1.f, 1.f)));
}

TEST_CASE("should interpolate between 1 and 0") {
    REQUIRE(Math::IEEE754::epsilon_equal(1.f, interpolatef(1.f, 0.f, 0.f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.9f, interpolatef(1.f, 0.f, 0.1f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.5f, interpolatef(1.f, 0.f, 0.5f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.1f, interpolatef(1.f, 0.f, 0.9f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, interpolatef(1.f, 0.f, 1.f)));
}

TEST_CASE("should interpolate between 0 and -1") {
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, interpolatef(0.f, -1.f, 0.f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.1f, interpolatef(0.f, -1.f, 0.1f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.5f, interpolatef(0.f, -1.f, 0.5f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.9f, interpolatef(0.f, -1.f, 0.9f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-1.f, interpolatef(0.f, -1.f, 1.f)));
}

TEST_CASE("should interpolate between -1 and 0") {
    REQUIRE(Math::IEEE754::epsilon_equal(-1.f, interpolatef(-1.f, 0.f, 0.f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.9f, interpolatef(-1.f, 0.f, 0.1f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.5f, interpolatef(-1.f, 0.f, 0.5f)));
    REQUIRE(Math::IEEE754::epsilon_equal(-0.1f, interpolatef(-1.f, 0.f, 0.9f)));
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, interpolatef(-1.f, 0.f, 1.f)));
}

// ---------------------------------------------------------------------------
// MARK: `argmaxf`
// ---------------------------------------------------------------------------

SCENARIO("the maximum argument needs to be found in an array") {
    GIVEN("an array of floats of equal value (0)") {
        std::vector<float> sequence{0, 0, 0, 0, 0};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = argmaxf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats of equal value (-1)") {
        std::vector<float> sequence{-1, -1, -1, -1, -1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = argmaxf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats of equal value (1)") {
        std::vector<float> sequence{1, 1, 1, 1, 1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = argmaxf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats in ascending order") {
        std::vector<float> sequence{1, 2, 3, 4, 5};
        WHEN("the argmax is calculated") {
            THEN("4 is returned") {
                const auto idx = argmaxf(sequence.data(), sequence.size());
                REQUIRE(4 == idx);
            }
        }
    }
    GIVEN("an array of floats in descending order") {
        std::vector<float> sequence{5, 4, 3, 2, 1};
        WHEN("the argmax is calculated") {
            THEN("0 is returned") {
                const auto idx = argmaxf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats in triangular order") {
        std::vector<float> sequence{1, 2, 3, 2, 1};
        WHEN("the argmax is calculated") {
            THEN("2 is returned") {
                const auto idx = argmaxf(sequence.data(), sequence.size());
                REQUIRE(2 == idx);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: `argminf`
// ---------------------------------------------------------------------------

SCENARIO("the minimum argument needs to be found in an array") {
    GIVEN("an array of floats of equal value (0)") {
        std::vector<float> sequence{0, 0, 0, 0, 0};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = argminf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats of equal value (-1)") {
        std::vector<float> sequence{-1, -1, -1, -1, -1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = argminf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats of equal value (1)") {
        std::vector<float> sequence{1, 1, 1, 1, 1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = argminf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats in ascending order") {
        std::vector<float> sequence{1, 2, 3, 4, 5};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = argminf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
    GIVEN("an array of floats in descending order") {
        std::vector<float> sequence{5, 4, 3, 2, 1};
        WHEN("the argmin is calculated") {
            THEN("4 is returned") {
                const auto idx = argminf(sequence.data(), sequence.size());
                REQUIRE(4 == idx);
            }
        }
    }
    GIVEN("an array of floats in triangular order") {
        std::vector<float> sequence{1, 2, 3, 2, 1};
        WHEN("the argmin is calculated") {
            THEN("0 is returned") {
                const auto idx = argminf(sequence.data(), sequence.size());
                REQUIRE(0 == idx);
            }
        }
    }
}
