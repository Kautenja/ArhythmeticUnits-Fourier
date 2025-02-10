// Test cases for the FunctionGenerator::Pulse structure.
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

#include "dsp/function_generator/pulse.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: FunctionGenerator::Pulse
// ---------------------------------------------------------------------------

SCENARIO("FunctionGenerator::Pulse processes signals at 10000Hz") {
    GIVEN("an initialized pulse generator and sample rate") {
        float sampleTime = 0.0001f;
        FunctionGenerator::Pulse generator;
        WHEN("the generator is initialized") {
            THEN("the default pulse time is 1ms") {
                REQUIRE(0.001f == generator.getPulseTime());
            }
            THEN("the pulse is low") {
                REQUIRE_FALSE(generator.getIsTriggered());
            }
        }
        WHEN("the pulse time is set to an arbitrary value") {
            generator.setPulseTime(1.f);
            THEN("the pulse time is set") {
                REQUIRE(1.f == generator.getPulseTime());
            }
        }
        WHEN("the pulse time is set below the minimal value") {
            generator.setPulseTime(1.f);
            generator.setPulseTime(0.f);
            THEN("the pulse time is set to the minimum of 1ms") {
                REQUIRE(0.001f == generator.getPulseTime());
            }
        }
        WHEN("the generator processes from stopped stage (not triggered)") {
            auto value = generator.process(sampleTime);
            THEN("the pulse does not go high") {
                REQUIRE_FALSE(value);
                REQUIRE_FALSE(generator.getIsTriggered());
            }
        }
        WHEN("the generator is triggered") {
            generator.trigger();
            THEN("the generator fires and is held for 10 samples") {
                for (unsigned i = 0; i < 10; i++) {
                    REQUIRE(generator.process(sampleTime));
                    REQUIRE(generator.getIsTriggered());
                }
                REQUIRE_FALSE(generator.process(sampleTime));
                REQUIRE_FALSE(generator.getIsTriggered());
            }
        }
        WHEN("the generator is triggered then reset") {
            generator.trigger();
            THEN("the generator fires and is held until resetting") {
                for (unsigned i = 0; i < 5; i++) {
                    REQUIRE(generator.process(sampleTime));
                    REQUIRE(generator.getIsTriggered());
                }
                generator.reset();
                REQUIRE_FALSE(generator.getIsTriggered());
                // process a sample
                REQUIRE_FALSE(generator.process(sampleTime));
                REQUIRE_FALSE(generator.getIsTriggered());
            }
        }
    }
}
