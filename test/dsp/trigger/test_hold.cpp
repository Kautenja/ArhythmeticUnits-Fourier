// Test cases for the Trigger::Hold structure.
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

#include "dsp/trigger/hold.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("Trigger::Hold accessors should be const and correctly initialized") {
    const Trigger::Hold trigger;
    REQUIRE(trigger.getState() == Trigger::Hold::State::Off);
    REQUIRE(trigger.isOff());
    REQUIRE_FALSE(trigger.isPressed());
    REQUIRE_FALSE(trigger.isHeld());
}

SCENARIO("Trigger::Hold processes signals at 100Hz") {
    GIVEN("an initialized trigger and sample rate") {
        float sampleTime = 0.01f;
        Trigger::Hold trigger;
        WHEN("the trigger starts to go high") {
            auto value = trigger.process(1.f, sampleTime);
            THEN("the trigger does not fire") {
                REQUIRE(value == Trigger::Hold::Event::None);
            }
            THEN("the trigger advances to the Pressed state") {
                REQUIRE(trigger.isPressed());
                REQUIRE(trigger.getState() == Trigger::Hold::State::Pressed);
                REQUIRE_FALSE(trigger.isOff());
                REQUIRE_FALSE(trigger.isHeld());
            }
        }
        WHEN("the trigger goes high then low within the press window") {
            trigger.process(1.f, 0.01f);
            auto value = trigger.process(0.f, sampleTime);
            THEN("the trigger fires") {
                REQUIRE(value == Trigger::Hold::Event::Press);
            }
            THEN("the trigger advances to the Off state") {
                REQUIRE(trigger.getState() == Trigger::Hold::State::Off);
                REQUIRE(trigger.isOff());
                REQUIRE_FALSE(trigger.isPressed());
                REQUIRE_FALSE(trigger.isHeld());
            }
        }
        WHEN("the trigger goes high past the press window") {
            THEN("the trigger does not fire, but is held after the window") {
                float time = 0.f;
                // enter the press stage
                while (time < Trigger::Hold::HOLD_TIME) {
                    time += sampleTime;
                    auto value = trigger.process(1.f, sampleTime);
                    REQUIRE(value == Trigger::Hold::Event::None);
                    REQUIRE(trigger.getState() == Trigger::Hold::State::Pressed);
                    REQUIRE(trigger.isPressed());
                    REQUIRE_FALSE(trigger.isOff());
                    REQUIRE_FALSE(trigger.isHeld());
                }
                // enter the held stage for a single sample
                {
                    auto value = trigger.process(1.f, sampleTime);
                    REQUIRE(value == Trigger::Hold::Event::BeginHold);
                    REQUIRE(trigger.getState() == Trigger::Hold::State::Held);
                    REQUIRE(trigger.isHeld());
                    REQUIRE_FALSE(trigger.isPressed());
                    REQUIRE_FALSE(trigger.isOff());
                }
                // take the trigger low in a single sample
                {
                    auto value = trigger.process(0.f, sampleTime);
                    REQUIRE(value == Trigger::Hold::Event::ReleaseHold);
                    REQUIRE(trigger.getState() == Trigger::Hold::State::Off);
                    REQUIRE(trigger.isOff());
                    REQUIRE_FALSE(trigger.isPressed());
                    REQUIRE_FALSE(trigger.isHeld());
                }
            }
        }
    }
}
