// Test cases for the FunctionGenerator::Integrator<float> structure.
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

#include "dsp/function_generator/integrator.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

/// the minimal stage time
static const float MIN_STAGE_TIME = 0.0001f;

TEST_CASE("initial values should be null; accessors should not mutate") {
    // define object as const to ensure that all accessors do not mutate
    const auto env = FunctionGenerator::Integrator<float>();
    // stages
    REQUIRE(env.getRise() == MIN_STAGE_TIME);
    REQUIRE(env.getFall() == MIN_STAGE_TIME);
    // shapes
    REQUIRE(env.getRiseShape() == 0.f);
    REQUIRE(env.getFallShape() == 0.f);
    // output
    REQUIRE(env.getValue() == 0.f);
}

SCENARIO("mutators should set values") {
    GIVEN("an envelope generator") {
        auto env = FunctionGenerator::Integrator<float>();
        // stage timings
        WHEN("the rise time is set") {
            env.setRise(1.f);
            THEN("the rise time should be accessible") {
                REQUIRE(env.getRise() == 1.f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getFall() == MIN_STAGE_TIME);
                REQUIRE(env.getRiseShape() == 0.0f);
                REQUIRE(env.getFallShape() == 0.0f);
                REQUIRE(env.getValue() == 0.f);
            }
        }
        WHEN("the rise time is set below the minimal value") {
            env.setRise(0.f);
            THEN("the rise time should be the minimal value") {
                REQUIRE(env.getRise() == MIN_STAGE_TIME);
            }
        }
        WHEN("the fall time is set") {
            env.setFall(1.f);
            THEN("the fall time should be accessible") {
                REQUIRE(env.getFall() == 1.f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getRise() == MIN_STAGE_TIME);
                REQUIRE(env.getRiseShape() == 0.0f);
                REQUIRE(env.getFallShape() == 0.0f);
                REQUIRE(env.getValue() == 0.f);
            }
        }
        WHEN("the fall time is set below the minimal value") {
            env.setFall(0.f);
            THEN("the fall time should be the fall value") {
                REQUIRE(env.getFall() == MIN_STAGE_TIME);
            }
        }
        // stage shapes
        WHEN("the rise shape is set") {
            env.setRiseShape(0.5f);
            THEN("the rise shape should be accessible") {
                REQUIRE(env.getRiseShape() == 0.5f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getRise() == MIN_STAGE_TIME);
                REQUIRE(env.getFall() == MIN_STAGE_TIME);
                REQUIRE(env.getFallShape() == 0.0f);
                REQUIRE(env.getValue() == 0.f);
            }
        }
        WHEN("the fall shape is set") {
            env.setFallShape(0.5f);
            THEN("the fall shape should be accessible") {
                REQUIRE(env.getFallShape() == 0.5f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getRise() == MIN_STAGE_TIME);
                REQUIRE(env.getFall() == MIN_STAGE_TIME);
                REQUIRE(env.getRiseShape() == 0.0f);
                REQUIRE(env.getValue() == 0.f);
            }
        }
    }
}

TEST_CASE("processing should not change parameters") {
    auto env = FunctionGenerator::Integrator<float>();
    // process for an arbitrary time of 1ms with a signal of 0, i.e., no change.
    env.process(0.f, 0.001f);
    // output values should not change because 0 (signal) is not different
    // from 0 (slew value)
    REQUIRE(env.getValue() == 0.f);
    // parameters should not change because they are immutable in class scope
    REQUIRE(env.getRise() == MIN_STAGE_TIME);
    REQUIRE(env.getFall() == MIN_STAGE_TIME);
    REQUIRE(env.getRiseShape() == 0.f);
    REQUIRE(env.getFallShape() == 0.f);
}

TEST_CASE("should slew positive signal (1.f)") {
    auto env = FunctionGenerator::Integrator<float>();
    // rise stage
    for (int i = 0; i < 10; i++) {
        env.process(1.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), (i + 1) * 0.1f));
    }
    // end of rise event
    {
        env.process(1.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 1.f));
    }
    // sustain
    for (int i = 0; i < 10; i++) {
        env.process(1.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 1.f));
    }
    // fall stage
    for (int i = 0; i < 10; i++) {
        env.process(0.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 1.f - (i + 1) * 0.1f));
    }
    // end of fall event
    {
        env.process(0.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 0.f));
    }
}

TEST_CASE("should slew positive signal (0.5f)") {
    auto env = FunctionGenerator::Integrator<float>();
    // rise stage
    for (int i = 0; i < 5; i++) {
        env.process(0.5f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), (i + 1) * 0.1f));
    }
    // end of rise event
    {
        env.process(0.5f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 0.5f));
    }
    // sustain
    for (int i = 0; i < 5; i++) {
        env.process(0.5f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 0.5f));
    }
    // fall stage
    for (int i = 0; i < 5; i++) {
        env.process(0.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 0.5f - (i + 1) * 0.1f));
    }
    // end of fall event
    {
        env.process(0.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 0.f));
    }
}

TEST_CASE("should slew negative signal (-1.f)") {
    auto env = FunctionGenerator::Integrator<float>();
    // rise stage
    for (int i = 0; i < 10; i++) {
        env.process(-1.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), -(i + 1) * 0.1f));
    }
    // end of rise event
    {
        env.process(-1.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), -1.f));
    }
    // sustain
    for (int i = 0; i < 10; i++) {
        env.process(-1.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), -1.f));
    }
    // fall stage
    for (int i = 0; i < 10; i++) {
        env.process(0.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), -(1.f - (i + 1) * 0.1f)));
    }
    // end of fall event
    {
        env.process(0.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 0.f));
    }
}

TEST_CASE("should slew negative signal (-0.5f)") {
    auto env = FunctionGenerator::Integrator<float>();
    // rise stage
    for (int i = 0; i < 5; i++) {
        env.process(-0.5f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), -(i + 1) * 0.1f));
    }
    // end of rise event
    {
        env.process(-0.5f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), -0.5f));
    }
    // sustain
    for (int i = 0; i < 5; i++) {
        env.process(-0.5f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), -0.5f));
    }
    // fall stage
    for (int i = 0; i < 5; i++) {
        env.process(0.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), -(0.5f - (i + 1) * 0.1f)));
    }
    // end of fall event
    {
        env.process(0.f, MIN_STAGE_TIME / 10.f);
        REQUIRE(Math::IEEE754::epsilon_equal(env.getValue(), 0.f));
    }
}
