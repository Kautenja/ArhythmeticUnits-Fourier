// Test cases for Math::IEEE754 functions.
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

#include "dsp/math/ieee754.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: `epsilon_equal`
// ---------------------------------------------------------------------------

TEST_CASE("epsilon_equal should be true at 0") {
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, 0.f));
    REQUIRE(Math::IEEE754::epsilon_equal(0.0, 0.0));
}

TEST_CASE("epsilon_equal should be true at 0 + epsilon") {
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, 0.f + std::numeric_limits<float>::epsilon()));
    REQUIRE(Math::IEEE754::epsilon_equal(0.0, 0.0 + std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 0 - epsilon") {
    REQUIRE(Math::IEEE754::epsilon_equal(0.f, 0.f - std::numeric_limits<float>::epsilon()));
    REQUIRE(Math::IEEE754::epsilon_equal(0.0, 0.0 - std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 0 + 2 * epsilon") {
    REQUIRE_FALSE(Math::IEEE754::epsilon_equal(0.f, 0.f + 2 * std::numeric_limits<float>::epsilon()));
    REQUIRE_FALSE(Math::IEEE754::epsilon_equal(0.0, 0.0 + 2 * std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 0 - 2 * epsilon") {
    REQUIRE_FALSE(Math::IEEE754::epsilon_equal(0.f, 0.f - 2 * std::numeric_limits<float>::epsilon()));
    REQUIRE_FALSE(Math::IEEE754::epsilon_equal(0.0, 0.0 - 2 * std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 1") {
    REQUIRE(Math::IEEE754::epsilon_equal(1.f, 1.f));
    REQUIRE(Math::IEEE754::epsilon_equal(1.0, 1.0));
}

TEST_CASE("epsilon_equal should be true at 1 + epsilon") {
    REQUIRE(Math::IEEE754::epsilon_equal(1.f, 1.f + std::numeric_limits<float>::epsilon()));
    REQUIRE(Math::IEEE754::epsilon_equal(1.0, 1.0 + std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 1 - epsilon") {
    REQUIRE(Math::IEEE754::epsilon_equal(1.f, 1.f - std::numeric_limits<float>::epsilon()));
    REQUIRE(Math::IEEE754::epsilon_equal(1.0, 1.0 - std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be false at 1 + 2 * epsilon") {
    REQUIRE_FALSE(Math::IEEE754::epsilon_equal(1.f, 1.f + 2 * std::numeric_limits<float>::epsilon()));
    REQUIRE_FALSE(Math::IEEE754::epsilon_equal(1.0, 1.0 + 2 * std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be false at 1 - 2 * epsilon") {
    REQUIRE_FALSE(Math::IEEE754::epsilon_equal(1.f, 1.f - 2 * std::numeric_limits<float>::epsilon()));
    REQUIRE_FALSE(Math::IEEE754::epsilon_equal(1.0, 1.0 - 2 * std::numeric_limits<double>::epsilon()));
}

// ---------------------------------------------------------------------------
// MARK: `float`
// ---------------------------------------------------------------------------

TEST_CASE("IEEE754::FloatUnion should construct float 0.15625") {
    Math::IEEE754::FloatUnion num;
    num.components.sign = 0;
    num.components.exponent = 124;//0b01111100
    num.components.mantissa = 2097152;//0b01000000000000000000000
    REQUIRE(0.15625f == num.value);
}

TEST_CASE("IEEE754::FloatUnion should construct float -0.15625") {
    Math::IEEE754::FloatUnion num;
    num.components.sign = 1;
    num.components.exponent = 124;//0b01111100
    num.components.mantissa = 2097152;//0b01000000000000000000000
    REQUIRE(-0.15625f == num.value);
}

// ---------------------------------------------------------------------------
// MARK: `double`
// ---------------------------------------------------------------------------

TEST_CASE("IEEE754::DoubleUnion should construct double 1") {
    Math::IEEE754::DoubleUnion num;
    num.components.sign = 0;
    num.components.exponent = 1023;//0b01111111111
    num.components.mantissa = 0;//0b0000000000000000000000000000000000000000000000000000
    REQUIRE(1 == num.value);
}

TEST_CASE("IEEE754::DoubleUnion should construct double -1") {
    Math::IEEE754::DoubleUnion num;
    num.components.sign = 1;
    num.components.exponent = 1023;//0b01111111111
    num.components.mantissa = 0;//0b0000000000000000000000000000000000000000000000000000
    REQUIRE(-1 == num.value);
}

TEST_CASE("IEEE754::DoubleUnion should construct double 1.0000000000000002") {
    Math::IEEE754::DoubleUnion num;
    num.components.sign = 0;
    num.components.exponent = 1023;//0b01111111111
    num.components.mantissa = 1;//0b0000000000000000000000000000000000000000000000000001
    REQUIRE(1.0000000000000002 == num.value);
}

TEST_CASE("IEEE754::DoubleUnion should construct double -1.0000000000000002") {
    Math::IEEE754::DoubleUnion num;
    num.components.sign = 1;
    num.components.exponent = 1023;//0b01111111111
    num.components.mantissa = 1;//0b0000000000000000000000000000000000000000000000000001
    REQUIRE(-1.0000000000000002 == num.value);
}

TEST_CASE("IEEE754::DoubleUnion should construct double 1.0000000000000004") {
    Math::IEEE754::DoubleUnion num;
    num.components.sign = 0;
    num.components.exponent = 1023;//0b01111111111
    num.components.mantissa = 2;//0b0000000000000000000000000000000000000000000000000010
    REQUIRE(1.0000000000000004 == num.value);
}

TEST_CASE("IEEE754::DoubleUnion should construct double -1.0000000000000004") {
    Math::IEEE754::DoubleUnion num;
    num.components.sign = 1;
    num.components.exponent = 1023;//0b01111111111
    num.components.mantissa = 2;//0b0000000000000000000000000000000000000000000000000010
    REQUIRE(-1.0000000000000004 == num.value);
}

// ---------------------------------------------------------------------------
// MARK: `exp2`
// ---------------------------------------------------------------------------

TEST_CASE("exp2 should calculate 2**0") {
    REQUIRE(1.f == Math::IEEE754::exp2(0));
}

TEST_CASE("exp2 should calculate 2**1") {
    REQUIRE(2.f == Math::IEEE754::exp2(1));
}

TEST_CASE("exp2 should calculate 2**2") {
    REQUIRE(4.f == Math::IEEE754::exp2(2));
}

TEST_CASE("exp2 should calculate 2**3") {
    REQUIRE(8.f == Math::IEEE754::exp2(3));
}

TEST_CASE("exp2 should calculate 2**127") {
    REQUIRE(170141183460469231731687303715884105728.f == Math::IEEE754::exp2(127));
}

TEST_CASE("exp2 should calculate 2**-1") {
    REQUIRE(1.f / 2.f == Approx(Math::IEEE754::exp2(-1)));
}

TEST_CASE("exp2 should calculate 2**-2") {
    REQUIRE(1.f / 4.f == Approx(Math::IEEE754::exp2(-2)));
}

TEST_CASE("exp2 should calculate 2**-3") {
    REQUIRE(1.f / 8.f == Approx(Math::IEEE754::exp2(-3)));
}

TEST_CASE("exp2 should calculate 2**-32") {
    REQUIRE(2.3283064365386963e-10 == Approx(Math::IEEE754::exp2(-32)));
}

TEST_CASE("exp2 should calculate 2**-64") {
    REQUIRE(5.421010862427522e-20 == Approx(Math::IEEE754::exp2(-64)));
}

TEST_CASE("exp2 should throw error for 2**-65") {
    REQUIRE_THROWS(Math::IEEE754::exp2(-65));
}

TEST_CASE("exp2 should throw error for 2**128") {
    REQUIRE_THROWS(Math::IEEE754::exp2(128));
}
