// Test cases for Math::CircularBuffer structure.
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

#include "dsp/math/circular_buffer.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: CircularBuffer
// ---------------------------------------------------------------------------

TEST_CASE("initial values of Math::CircularBuffer<float> should be null") {
    auto buffer = Math::CircularBuffer<float>();
    REQUIRE(1 == buffer.size());
    REQUIRE(0.f == buffer.at(0));
    REQUIRE(0.f == buffer.at(-1));
    REQUIRE(0.f == buffer.at(1));
}

TEST_CASE("(Math::CircularBuffer<float>).resize should resize") {
    auto buffer = Math::CircularBuffer<float>();
    buffer.resize(100);
    REQUIRE(100 == buffer.size());
    for (unsigned i = 0; i < 100; i++) REQUIRE(0.f == buffer.at(i));
}

TEST_CASE("Math::CircularBuffer<float>(size) should initialize with size") {
    auto buffer = Math::CircularBuffer<float>(100);
    REQUIRE(100 == buffer.size());
    for (unsigned i = 0; i < 100; i++) REQUIRE(0.f == buffer.at(i));
}

SCENARIO("values are inserted into a Math::CircularBuffer<float>") {
    GIVEN("an empty circular buffer with size of 10") {
        auto buffer = Math::CircularBuffer<int>();
        buffer.resize(10);
        WHEN("a value is added to the buffer") {
            buffer.insert(1);
            THEN("the head is incremented") {
                REQUIRE(1 == buffer.at(0));
                REQUIRE(0 == buffer.at(-1));
                REQUIRE(0 == buffer.at(1));
            }
        }
        WHEN("values are added to the buffer") {
            for (unsigned i = 0; i < 10; i++) buffer.insert(i);
            THEN("the limit of the buffer has been reached") {
                REQUIRE(9 == buffer.at(0));
                REQUIRE(8 == buffer.at(-1));
                REQUIRE(0 == buffer.at(1));
            }
            THEN("adding additional values overwrites existing values") {
                buffer.insert(10);
                REQUIRE(10 == buffer.at(0));
                REQUIRE(9 == buffer.at(-1));
                REQUIRE(1 == buffer.at(1));
                buffer.insert(11);
                REQUIRE(11 == buffer.at(0));
                REQUIRE(10 == buffer.at(-1));
                REQUIRE(2 == buffer.at(1));
            }
        }
        WHEN("values are added to the buffer, then cleared") {
            for (unsigned i = 0; i < 10; i++) buffer.insert(i);
            buffer.clear();
            THEN("the buffer is empty") {
                for(unsigned i = 0; i < 10; i++)
                    REQUIRE(0 == buffer.at(i));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: ContiguousCircularBuffer
// ---------------------------------------------------------------------------

TEST_CASE("initial values of Math::ContiguousCircularBuffer<float> should be null") {
    auto buffer = Math::ContiguousCircularBuffer<float>();
    REQUIRE(1 == buffer.size());
    REQUIRE(0.f == buffer.at(0));
    REQUIRE(0.f == buffer.at(-1));
    REQUIRE(0.f == buffer.at(1));
}

TEST_CASE("(Math::ContiguousCircularBuffer<float>).resize should resize") {
    auto buffer = Math::ContiguousCircularBuffer<float>();
    buffer.resize(100);
    REQUIRE(100 == buffer.size());
    for (unsigned i = 0; i < 100; i++) REQUIRE(0.f == buffer.at(i));
}

TEST_CASE("Math::ContiguousCircularBuffer<float>(size) should initialize with size") {
    auto buffer = Math::ContiguousCircularBuffer<float>(100);
    REQUIRE(100 == buffer.size());
    for (unsigned i = 0; i < 100; i++) REQUIRE(0.f == buffer.at(i));
}

SCENARIO("values are inserted into a Math::ContiguousCircularBuffer<float>") {
    GIVEN("an empty circular buffer with size of 10") {
        auto buffer = Math::ContiguousCircularBuffer<int>();
        buffer.resize(10);
        WHEN("a value is added to the buffer") {
            buffer.insert(1);
            THEN("the head is incremented") {
                REQUIRE(1 == buffer.at(0));
                REQUIRE(0 == buffer.at(-1));
                REQUIRE(0 == buffer.at(1));
            }
            THEN("the contiguous array is correct") {
                for (unsigned i = 0; i < 10; i++) {
                    REQUIRE(buffer.contiguous()[i] == (i >= 9));
                }
            }
        }
        WHEN("values are added to the buffer") {
            for (unsigned i = 0; i < 10; i++) buffer.insert(i);
            THEN("the limit of the buffer has been reached") {
                REQUIRE(9 == buffer.at(0));
                REQUIRE(8 == buffer.at(-1));
                REQUIRE(0 == buffer.at(1));
            }
            THEN("the contiguous array is correct") {
                for (int i = 0; i < 10; i++) {
                    REQUIRE(buffer.contiguous()[i] == i);
                }
            }
            THEN("adding additional values overwrites existing values") {
                buffer.insert(10);
                REQUIRE(10 == buffer.at(0));
                REQUIRE(9 == buffer.at(-1));
                REQUIRE(1 == buffer.at(1));
                for (int i = 0; i < 10; i++) {
                    REQUIRE(buffer.contiguous()[i] == i + 1);
                }
                buffer.insert(11);
                REQUIRE(11 == buffer.at(0));
                REQUIRE(10 == buffer.at(-1));
                REQUIRE(2 == buffer.at(1));
                for (int i = 0; i < 10; i++) {
                    REQUIRE(buffer.contiguous()[i] == i + 2);
                }
            }
        }
        WHEN("values are added to the buffer, then cleared") {
            for (unsigned i = 0; i < 10; i++) buffer.insert(i);
            buffer.clear();
            THEN("the buffer is empty") {
                for(unsigned i = 0; i < 10; i++) {
                    REQUIRE(0 == buffer.at(i));
                    REQUIRE(0 == buffer.contiguous()[i]);
                }
            }
        }
    }
}
