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
#include "../../ieee754.hpp"
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
    REQUIRE(approx_equal<float>(-6.f, Math::amplitude2decibels<float>(0.5f), 1e-1f));
}

TEST_CASE("amplitude2decibels<double> should convert 0.5 to -6dB") {
    REQUIRE(approx_equal<float>(-6.0, Math::amplitude2decibels<double>(0.5), 1e-1));
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
    REQUIRE(approx_equal<double>(-120.0, Math::amplitude2decibels<double>(1e-6f), 1e-6));
}

TEST_CASE("amplitude2decibels<float> should convert 2 to 6dB") {
    REQUIRE(approx_equal<float>(6.f, Math::amplitude2decibels<float>(2.f), 1e-1f));
}

TEST_CASE("amplitude2decibels<double> should convert 2 to 6dB") {
    REQUIRE(approx_equal<float>(6.0, Math::amplitude2decibels<double>(2.0), 1e-1));
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
    REQUIRE(approx_equal<float>(0.5f, Math::decibels2amplitude<float>(-6.f), 1e-2f));
}

TEST_CASE("decibels2amplitude<double> should convert -6dB to 0.5") {
    REQUIRE(approx_equal<float>(0.5, Math::decibels2amplitude<float>(-6.0), 1e-2f));
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
    REQUIRE(approx_equal<float>(2.f, Math::decibels2amplitude<float>(6.f), 1e-2f));
}

TEST_CASE("decibels2amplitude<double> should convert 6dB to 2") {
    REQUIRE(approx_equal<float>(2.0, Math::decibels2amplitude<float>(6.0), 1e-2f));
}

TEST_CASE("decibels2amplitude<float> should convert -infinity dB to 0") {
    REQUIRE(0.f == Math::decibels2amplitude<float>(-std::numeric_limits<float>::infinity()));
}

TEST_CASE("decibels2amplitude<double> should convert -infinity dB to 0") {
    REQUIRE(0.f == Math::decibels2amplitude<double>(-std::numeric_limits<double>::infinity()));
}
