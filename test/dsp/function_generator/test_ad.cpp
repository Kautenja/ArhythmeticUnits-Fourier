// Test cases for the FunctionGenerator::AD structure.
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

#include "dsp/function_generator/ad.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("initial values should be null; accessors should not mutate") {
    // define object as const to ensure that all accessors do not mutate
    const auto env = FunctionGenerator::AD();
    // stages
    REQUIRE_FALSE(env.isLooping);
    REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getDuration() == 3 * FunctionGenerator::MIN_STAGE_TIME);
    // shapes
    REQUIRE(env.getAttackShape() == 1.f);
    REQUIRE(env.getDecayShape() == 1.f);
    // output
    REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
    REQUIRE(env.getValue() == 0.f);
}

SCENARIO("mutators should set values") {
    GIVEN("an envelope generator") {
        auto env = FunctionGenerator::AD();
        // stage timings
        WHEN("the attack time is set") {
            env.setAttack(1.f);
            THEN("the attack time should be accessible") {
                REQUIRE(env.getAttack() == 1.f);
            }
            THEN("the total duration should be correct") {
                REQUIRE(env.getDuration() == Approx(1.f + 2 * FunctionGenerator::MIN_STAGE_TIME));
            }
            THEN("other variables should be unaffected") {
                REQUIRE_FALSE(env.isLooping);
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
            }
        }
        WHEN("the attack time is set below the minimal value") {
            env.setAttack(0.f);
            THEN("the attack time should be the minimal value") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
            }
            THEN("the total duration should be correct") {
                REQUIRE(env.getDuration() == 3 * FunctionGenerator::MIN_STAGE_TIME);
            }
        }
        WHEN("the hold time is set") {
            env.setHold(1.f);
            THEN("the hold time should be accessible") {
                REQUIRE(env.getHold() == 1.f);
            }
            THEN("the total duration should be correct") {
                REQUIRE(env.getDuration() == Approx(1.f + 2 * FunctionGenerator::MIN_STAGE_TIME));
            }
            THEN("other variables should be unaffected") {
                REQUIRE_FALSE(env.isLooping);
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
            }
        }
        WHEN("the hold time is set below the minimal value") {
            env.setHold(0.f);
            THEN("the hold time should be the minimal value") {
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
            }
            THEN("the total duration should be correct") {
                REQUIRE(env.getDuration() == 3 * FunctionGenerator::MIN_STAGE_TIME);
            }
        }
        WHEN("the decay time is set") {
            env.setDecay(1.f);
            THEN("the decay time should be accessible") {
                REQUIRE(env.getDecay() == 1.f);
            }
            THEN("the total duration should be correct") {
                REQUIRE(env.getDuration() == Approx(1.f + 2 * FunctionGenerator::MIN_STAGE_TIME));
            }
            THEN("other variables should be unaffected") {
                REQUIRE_FALSE(env.isLooping);
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
            }
        }
        WHEN("the decay time is set below the minimal value") {
            env.setDecay(0.f);
            THEN("the decay time should be the decay value") {
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
            }
            THEN("the total duration should be correct") {
                REQUIRE(env.getDuration() == 3 * FunctionGenerator::MIN_STAGE_TIME);
            }
        }
        // stage shapes
        WHEN("the attack shape is set") {
            env.setAttackShape(0.5f);
            THEN("the attack shape should be accessible") {
                REQUIRE(env.getAttackShape() == 0.5f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE_FALSE(env.isLooping);
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDuration() == 3 * FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
            }
        }
        WHEN("the decay shape is set") {
            env.setDecayShape(0.5f);
            THEN("the decay shape should be accessible") {
                REQUIRE(env.getDecayShape() == 0.5f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE_FALSE(env.isLooping);
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDuration() == 3 * FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
            }
        }
    }
}

TEST_CASE("processing should not change outputs when stopped") {
    auto env = FunctionGenerator::AD();
    // process for an arbitrary time of 1ms.
    env.process(0.001f);
    // output values should not change because the generator was not triggered
    REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
    REQUIRE(env.getValue() == 0.f);
    // parameters should not change because they are immutable in class scope
    REQUIRE_FALSE(env.isLooping);
    REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getDuration() == 3 * FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getAttackShape() == 1.f);
    REQUIRE(env.getDecayShape() == 1.f);
}

TEST_CASE("triggering the generator should enter the attack stage") {
    auto env = FunctionGenerator::AD();
    // trigger to enter attack stage
    env.reset(true);
    REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Attack);
}

TEST_CASE("resetting the generator should enter the stopped stage") {
    auto env = FunctionGenerator::AD();
    // trigger to enter attack stage
    env.reset(true);
    // reset to enter stopped stage
    env.reset();
    REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
}

TEST_CASE("should step through stages") {
    auto env = FunctionGenerator::AD();
    // trigger
    env.reset(true);
    // attack stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AD::Event::None);
        REQUIRE(env.getValue() == Approx((i + 1) * 0.1f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Attack);
    }
    // end of attack event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AD::Event::EndOfAttack);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Hold);
    }
    // hold stage
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Hold);
    }
    // end of hold event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AD::Event::EndOfHold);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Decay);
    }
    // decay stage
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        const float expected = 1.f - (i + 1) * 0.1f;
        if (expected != 0.0) REQUIRE(env.getValue() == Approx(expected));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Decay);
    }
    // end of decay event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AD::Event::EndOfDecay);
        REQUIRE(env.getValue() == Approx(0.f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Stopped);
    }
}

TEST_CASE("should step through stages and loop") {
    auto env = FunctionGenerator::AD();
    env.isLooping = true;
    // trigger
    env.reset(true);
    // attack stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AD::Event::None);
        REQUIRE(env.getValue() == Approx((i + 1) * 0.1f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Attack);
    }
    // end of attack event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AD::Event::EndOfAttack);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Hold);
    }
    // hold stage
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Hold);
    }
    // end of hold event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AD::Event::EndOfHold);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Decay);
    }
    // decay stage
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        const float expected = 1.f - (i + 1) * 0.1f;
        if (expected != 0.0) REQUIRE(env.getValue() == Approx(expected));
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Decay);
    }
    // end of decay event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AD::Event::EndOfDecay);
        REQUIRE(env.getValue() == Approx(0.f));
        // loop back around to the attack stage instead of the stopped stage
        REQUIRE(env.getStage() == FunctionGenerator::AD::Stage::Attack);
    }
}
