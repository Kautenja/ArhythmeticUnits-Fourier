// Test cases for the FunctionGenerator::AR structure.
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

#include "dsp/function_generator/ar.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("initial values should be null; accessors should not mutate") {
    // define object as const to ensure that all accessors do not mutate
    const auto env = FunctionGenerator::AR();
    // stages
    REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
    // shapes
    REQUIRE(env.getAttackShape() == 1.f);
    REQUIRE(env.getReleaseShape() == 1.f);
    // output
    REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Stopped);
    REQUIRE(env.getValue() == 0.f);
    REQUIRE_FALSE(env.getIsGateHigh());
    REQUIRE(env.isEOC());
    REQUIRE_FALSE(env.isEON());
}

SCENARIO("mutators should set values") {
    GIVEN("an envelope generator") {
        auto env = FunctionGenerator::AR();
        // stage timings
        WHEN("the attack time is set") {
            env.setAttack(1.f);
            THEN("the attack time should be accessible") {
                REQUIRE(env.getAttack() == 1.f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
        WHEN("the attack time is set below the minimal value") {
            env.setAttack(0.f);
            THEN("the attack time should be the minimal value") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
            }
        }
        WHEN("the release time is set") {
            env.setRelease(1.f);
            THEN("the release time should be accessible") {
                REQUIRE(env.getRelease() == 1.f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
        WHEN("the release time is set below the minimal value") {
            env.setRelease(0.f);
            THEN("the release time should be the release value") {
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
            }
        }
        // stage shapes
        WHEN("the attack shape is set") {
            env.setAttackShape(0.5f);
            THEN("the attack shape should be accessible") {
                REQUIRE(env.getAttackShape() == 0.5f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
        WHEN("the release shape is set") {
            env.setReleaseShape(0.5f);
            THEN("the release shape should be accessible") {
                REQUIRE(env.getReleaseShape() == 0.5f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
    }
}

TEST_CASE("processing should not change outputs when stopped") {
    auto env = FunctionGenerator::AR();
    // process for an arbitrary time of 1ms.
    env.process(0.001f);
    // output values should not change because the generator was not triggered
    REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Stopped);
    REQUIRE(env.getValue() == 0.f);
    REQUIRE_FALSE(env.getIsGateHigh());
    REQUIRE(env.isEOC());
    REQUIRE_FALSE(env.isEON());
    // parameters should not change because they are immutable in class scope
    REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getAttackShape() == 1.f);
    REQUIRE(env.getReleaseShape() == 1.f);
}

TEST_CASE("triggering the gate should enter the attack stage") {
    auto env = FunctionGenerator::AR();
    // trigger to enter attack stage
    env.setIsGateHigh(true);
    REQUIRE(env.getIsGateHigh());
    REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Attack);
}

TEST_CASE("releasing the gate should enter the release stage") {
    auto env = FunctionGenerator::AR();
    // trigger to enter attack stage
    env.setIsGateHigh(true);
    // reset to enter release stage
    env.setIsGateHigh(false);
    REQUIRE_FALSE(env.getIsGateHigh());
    REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Release);
}

TEST_CASE("should step through stages") {
    auto env = FunctionGenerator::AR();
    // trigger
    env.setIsGateHigh(true);
    // attack stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AR::Event::None);
        REQUIRE(env.getValue() == Approx((i + 1) * 0.1f));
        REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Attack);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // end of attack event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AR::Event::EndOfAttack);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Sustain);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // sustain
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AR::Event::None);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Sustain);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // release stage
    env.setIsGateHigh(false);
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        const float expected = 1.f - (i + 1) * 0.1f;
        if (expected != 0.0) REQUIRE(env.getValue() == Approx(expected));
        REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Release);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // end of release event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AR::Event::EndOfRelease);
        REQUIRE(env.getValue() == Approx(0.f));
        REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Stopped);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
}

TEST_CASE("should step through stages (half sustain)") {
    auto env = FunctionGenerator::AR();
    // key-on
    env.setIsGateHigh(true);
    // attack stage
    for (unsigned i = 0; i < 5; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AR::Event::None);
        REQUIRE(env.getValue() == Approx((i + 1) * 0.1f));
        REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Attack);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // key-off
    env.setIsGateHigh(false);
    REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Release);
    REQUIRE_FALSE(env.isEOC());
    REQUIRE(env.isEON());
    // release stage
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        const float expected = 0.5f - (i + 1) * 0.05f;
        if (expected != 0.0) REQUIRE(env.getValue() == Approx(expected));
        REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Release);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // end of release event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AR::Event::EndOfRelease);
        REQUIRE(env.getValue() == Approx(0.f));
        REQUIRE(env.getStage() == FunctionGenerator::AR::Stage::Stopped);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
}
