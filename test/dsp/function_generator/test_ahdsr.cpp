// Test cases for the FunctionGenerator::AHDSR structure.
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

#include "dsp/function_generator/ahdsr.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("initial values should be null; accessors should not mutate") {
    // define object as const to ensure that all accessors do not mutate
    const auto env = FunctionGenerator::AHDSR();
    // stages
    REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getSustain() == 0.f);
    REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
    // shapes
    REQUIRE(env.getAttackShape() == 1.f);
    REQUIRE(env.getDecayShape() == 1.f);
    REQUIRE(env.getReleaseShape() == 1.f);
    // output
    REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
    REQUIRE(env.getValue() == 0.f);
    REQUIRE_FALSE(env.getIsGateHigh());
    REQUIRE(env.isEOC());
    REQUIRE_FALSE(env.isEON());
}

SCENARIO("mutators should set values") {
    GIVEN("an envelope generator") {
        auto env = FunctionGenerator::AHDSR();
        // stage timings
        WHEN("the attack time is set") {
            env.setAttack(1.f);
            THEN("the attack time should be accessible") {
                REQUIRE(env.getAttack() == 1.f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getSustain() == 0.f);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
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
        WHEN("the hold time is set") {
            env.setHold(1.f);
            THEN("the hold time should be accessible") {
                REQUIRE(env.getHold() == 1.f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getSustain() == 0.f);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
        WHEN("the hold time is set below the minimal value") {
            env.setHold(0.f);
            THEN("the hold time should be the minimal value") {
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
            }
        }
        WHEN("the decay time is set") {
            env.setDecay(1.f);
            THEN("the decay time should be accessible") {
                REQUIRE(env.getDecay() == 1.f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getSustain() == 0.f);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
        WHEN("the decay time is set below the minimal value") {
            env.setDecay(0.f);
            THEN("the decay time should be the minimal value") {
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
            }
        }
        WHEN("the sustain level is set") {
            env.setSustain(0.5f);
            THEN("the sustain level should be accessible") {
                REQUIRE(env.getSustain() == 0.5f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
        WHEN("the sustain level is set below the minimal value") {
            env.setSustain(-1.f);
            THEN("the sustain level should be the minimal value") {
                REQUIRE(env.getSustain() == 0.f);
            }
        }
        WHEN("the sustain level is set above the maximal value") {
            env.setSustain(2.f);
            THEN("the sustain level should be the maximal value") {
                REQUIRE(env.getSustain() == 1.f);
            }
        }
        WHEN("the release time is set") {
            env.setRelease(1.f);
            THEN("the release time should be accessible") {
                REQUIRE(env.getRelease() == 1.f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getSustain() == 0.f);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
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
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getSustain() == 0.f);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
        WHEN("the decay shape is set") {
            env.setDecayShape(0.5f);
            THEN("the decay shape should be accessible") {
                REQUIRE(env.getDecayShape() == 0.5f);
            }
            THEN("other variables should be unaffected") {
                REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getSustain() == 0.f);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getReleaseShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
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
                REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getSustain() == 0.f);
                REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
                REQUIRE(env.getAttackShape() == 1.0f);
                REQUIRE(env.getDecayShape() == 1.0f);
                REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
                REQUIRE(env.getValue() == 0.f);
                REQUIRE_FALSE(env.getIsGateHigh());
                REQUIRE(env.isEOC());
                REQUIRE_FALSE(env.isEON());
            }
        }
    }
}

TEST_CASE("processing should not change outputs when stopped") {
    auto env = FunctionGenerator::AHDSR();
    // process for an arbitrary time of 1ms.
    env.process(0.001f);
    // output values should not change because the generator was not triggered
    REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
    REQUIRE(env.getValue() == 0.f);
    REQUIRE_FALSE(env.getIsGateHigh());
    REQUIRE(env.isEOC());
    REQUIRE_FALSE(env.isEON());
    // parameters should not change because they are immutable in class scope
    REQUIRE(env.getAttack() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getHold() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getDecay() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getSustain() == 0.f);
    REQUIRE(env.getRelease() == FunctionGenerator::MIN_STAGE_TIME);
    REQUIRE(env.getAttackShape() == 1.f);
    REQUIRE(env.getDecayShape() == 1.f);
    REQUIRE(env.getReleaseShape() == 1.f);
}

TEST_CASE("triggering the gate should enter the attack stage") {
    auto env = FunctionGenerator::AHDSR();
    // trigger to enter attack stage
    env.setIsGateHigh(true);
    REQUIRE(env.getIsGateHigh());
    REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Attack);
}

TEST_CASE("releasing the gate should enter the release stage") {
    auto env = FunctionGenerator::AHDSR();
    // trigger to enter attack stage
    env.setIsGateHigh(true);
    // reset to enter release stage
    env.setIsGateHigh(false);
    REQUIRE_FALSE(env.getIsGateHigh());
    REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Release);
}

TEST_CASE("should step through stages (no sustain)") {
    auto env = FunctionGenerator::AHDSR();
    // trigger
    env.setIsGateHigh(true);
    // attack stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx((i + 1) * 0.1f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Attack);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // end of attack event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfAttack);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Hold);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // hold stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Hold);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // end of hold event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfHold);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Decay);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // decay stage (to 0 because there is no sustain)
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        const float expected = 1.f - (i + 1) * 0.1f;
        if (expected != 0.0) REQUIRE(env.getValue() == Approx(expected));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Decay);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // end of decay event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfDecay);
        REQUIRE(env.getValue() == Approx(0.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Sustain);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // sustain
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx(0.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Sustain);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // release
    env.setIsGateHigh(false);
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfRelease);
        REQUIRE(env.getValue() == Approx(0.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
}

TEST_CASE("should step through stages (full sustain)") {
    auto env = FunctionGenerator::AHDSR();
    env.setSustain(1.f);
    // trigger
    env.setIsGateHigh(true);
    // attack stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx((i + 1) * 0.1f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Attack);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // end of attack event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfAttack);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Hold);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // hold stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Hold);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // end of hold event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfHold);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Decay);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // end of decay event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfDecay);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Sustain);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // sustain
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Sustain);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // release stage
    env.setIsGateHigh(false);
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        const float expected = 1.f - (i + 1) * 0.1f;
        if (expected != 0.0) REQUIRE(env.getValue() == Approx(expected));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Release);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // end of release event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfRelease);
        REQUIRE(env.getValue() == Approx(0.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
}

TEST_CASE("should step through stages (half sustain)") {
    auto env = FunctionGenerator::AHDSR();
    env.setSustain(0.5f);
    // trigger
    env.setIsGateHigh(true);
    // attack stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx((i + 1) * 0.1f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Attack);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // end of attack event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfAttack);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Hold);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // hold stage
    for (unsigned i = 0; i < 10; i++) {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Hold);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
    // end of hold event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfHold);
        REQUIRE(env.getValue() == Approx(1.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Decay);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // decay stage
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        const float expected = 1.f - (i + 1) * 0.05f;
        if (expected != 0.0) REQUIRE(env.getValue() == Approx(expected));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Decay);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // end of decay event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfDecay);
        REQUIRE(env.getValue() == Approx(0.5f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Sustain);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // sustain
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::None);
        REQUIRE(env.getValue() == Approx(0.5f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Sustain);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // release stage
    env.setIsGateHigh(false);
    for (unsigned i = 0; i < 10; i++) {
        env.process(0.0001);
        const float expected = 0.5f - (i + 1) * 0.05f;
        if (expected != 0.0) REQUIRE(env.getValue() == Approx(expected));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Release);
        REQUIRE_FALSE(env.isEOC());
        REQUIRE(env.isEON());
    }
    // end of release event
    {
        auto event = env.process(0.0001);
        REQUIRE(event == FunctionGenerator::AHDSR::Event::EndOfRelease);
        REQUIRE(env.getValue() == Approx(0.f));
        REQUIRE(env.getStage() == FunctionGenerator::AHDSR::Stage::Stopped);
        REQUIRE(env.isEOC());
        REQUIRE_FALSE(env.isEON());
    }
}
