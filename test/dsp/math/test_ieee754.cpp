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

#include "../../ieee754.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: `epsilon_equal`
// ---------------------------------------------------------------------------

TEST_CASE("epsilon_equal should be true at 0") {
    REQUIRE(epsilon_equal(0.f, 0.f));
    REQUIRE(epsilon_equal(0.0, 0.0));
}

TEST_CASE("epsilon_equal should be true at 0 + epsilon") {
    REQUIRE(epsilon_equal(0.f, 0.f + std::numeric_limits<float>::epsilon()));
    REQUIRE(epsilon_equal(0.0, 0.0 + std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 0 - epsilon") {
    REQUIRE(epsilon_equal(0.f, 0.f - std::numeric_limits<float>::epsilon()));
    REQUIRE(epsilon_equal(0.0, 0.0 - std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 0 + 2 * epsilon") {
    REQUIRE_FALSE(epsilon_equal(0.f, 0.f + 2 * std::numeric_limits<float>::epsilon()));
    REQUIRE_FALSE(epsilon_equal(0.0, 0.0 + 2 * std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 0 - 2 * epsilon") {
    REQUIRE_FALSE(epsilon_equal(0.f, 0.f - 2 * std::numeric_limits<float>::epsilon()));
    REQUIRE_FALSE(epsilon_equal(0.0, 0.0 - 2 * std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 1") {
    REQUIRE(epsilon_equal(1.f, 1.f));
    REQUIRE(epsilon_equal(1.0, 1.0));
}

TEST_CASE("epsilon_equal should be true at 1 + epsilon") {
    REQUIRE(epsilon_equal(1.f, 1.f + std::numeric_limits<float>::epsilon()));
    REQUIRE(epsilon_equal(1.0, 1.0 + std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be true at 1 - epsilon") {
    REQUIRE(epsilon_equal(1.f, 1.f - std::numeric_limits<float>::epsilon()));
    REQUIRE(epsilon_equal(1.0, 1.0 - std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be false at 1 + 2 * epsilon") {
    REQUIRE_FALSE(epsilon_equal(1.f, 1.f + 2 * std::numeric_limits<float>::epsilon()));
    REQUIRE_FALSE(epsilon_equal(1.0, 1.0 + 2 * std::numeric_limits<double>::epsilon()));
}

TEST_CASE("epsilon_equal should be false at 1 - 2 * epsilon") {
    REQUIRE_FALSE(epsilon_equal(1.f, 1.f - 2 * std::numeric_limits<float>::epsilon()));
    REQUIRE_FALSE(epsilon_equal(1.0, 1.0 - 2 * std::numeric_limits<double>::epsilon()));
}
