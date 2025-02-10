// Test cases for the Clock::Generator structure.
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

#include "dsp/clock/generator.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

SCENARIO("accessors and mutators are used") {
    GIVEN("an initialized Clock::Generator") {
        auto clock = Clock::Generator();
        WHEN("the initial values are accessed") {
            THEN("the initial values are exactly correct") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getPulseWidth() == 0.5f);
                REQUIRE(clock.getFrequency() == 120.f);
                REQUIRE(clock.getPeriod() == 1.f / 120.f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the isRunning flag is set high") {
            clock.isRunning = true;
            THEN("the isRunning flag is set high") {
                REQUIRE(clock.isRunning);
            }
            THEN("the pulse goes high") {
                REQUIRE(clock.getGate());
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getPulseWidth() == 0.5f);
                REQUIRE(clock.getFrequency() == 120.f);
                REQUIRE(clock.getPeriod() == 1.f / 120.f);
                // no time has elapsed, time remaining should equal the period
                REQUIRE(clock.getTime() == clock.getPeriod());
            }
        }
        WHEN("the pulse width is set") {
            clock.setPulseWidth(0.6f);
            THEN("the pulse width should be updated") {
                REQUIRE(0.6f == clock.getPulseWidth());
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getFrequency() == 120.f);
                REQUIRE(clock.getPeriod() == 1.f / 120.f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the pulse width is set below the minimal value") {
            clock.setPulseWidth(0.0f);
            THEN("the pulse width should be set to the minimal value") {
                REQUIRE(0.01f == clock.getPulseWidth());
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getFrequency() == 120.f);
                REQUIRE(clock.getPeriod() == 1.f / 120.f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the pulse width is set above the maximal value") {
            clock.setPulseWidth(1.6f);
            THEN("the pulse width should be set to the maximal value") {
                REQUIRE(0.99f == clock.getPulseWidth());
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getFrequency() == 120.f);
                REQUIRE(clock.getPeriod() == 1.f / 120.f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the frequency is set") {
            clock.setFrequency(50.f);
            THEN("the frequency should be updated") {
                REQUIRE(clock.getFrequency() == 50.f);
            }
            THEN("the period should be updated") {
                REQUIRE(clock.getPeriod() == 0.02f);
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getPulseWidth() == 0.5f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the frequency is set below the minimal value") {
            clock.setFrequency(0.f);
            THEN("the frequency is set to exactly the minimal value") {
                REQUIRE(clock.getFrequency() == 0.0001f);
            }
            THEN("the period is set to exactly the maximal value") {
                REQUIRE(clock.getPeriod() == 10000.f);
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getPulseWidth() == 0.5f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the frequency is set above the maximal value") {
            clock.setFrequency(1000000.f);
            THEN("the frequency is set to exactly the maximal value") {
                REQUIRE(clock.getFrequency() == 20000.f);
            }
            THEN("the period is set to exactly the minimal value") {
                REQUIRE(clock.getPeriod() == 0.00005f);
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getPulseWidth() == 0.5f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the period is set") {
            clock.setPeriod(0.02f);
            THEN("the period should be updated") {
                REQUIRE(clock.getPeriod() == 0.02f);
            }
            THEN("the frequency should be updated") {
                REQUIRE(clock.getFrequency() == 50.f);
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getPulseWidth() == 0.5f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the period is set below the minimal value") {
            clock.setPeriod(0.f);
            THEN("the period is set to exactly the minimal value") {
                REQUIRE(clock.getPeriod() == 0.00005f);
            }
            THEN("the frequency is set to exactly the maximal value") {
                REQUIRE(clock.getFrequency() == 20000.f);
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getPulseWidth() == 0.5f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
        WHEN("the period is set above the maximal value") {
            clock.setPeriod(20000.f);
            THEN("the period is set to exactly the maximal value") {
                REQUIRE(clock.getPeriod() == 10000.f);
            }
            THEN("the frequency is set to exactly the minimal value") {
                REQUIRE(clock.getFrequency() == 0.0001f);
            }
            THEN("other values should not be effected") {
                REQUIRE(clock.getPhase() == 0.f);
                REQUIRE(clock.getPulseWidth() == 0.5f);
                // no time has elapsed, time remaining should equal period
                REQUIRE(clock.getTime() == clock.getPeriod());
                REQUIRE_FALSE(clock.isRunning);
                // the clock is not running, so the value should be low
                REQUIRE_FALSE(clock.getGate());
            }
        }
    }
}

SCENARIO("a clock signal is processed") {
    GIVEN("an initialized clock") {
        auto clock = Clock::Generator();
        clock.setPeriod(0.001);
        const float sample_time = 0.0001;
        WHEN("the clock processes while not running") {
            THEN("the output value stays low") {
                for (unsigned i = 0; i < 10; i++) {
                    clock.process(sample_time);
                    REQUIRE_FALSE(clock.getGate());
                    // time should not be advancing
                    REQUIRE(clock.getTime() == clock.getPeriod());
                    // phase should not change
                    REQUIRE(clock.getPhase() == 0.f);
                }
            }
        }
        WHEN("the clock processes while running") {
            clock.isRunning = true;
            THEN("the output value goes high when phase is greater than PW") {
                REQUIRE(0.f == clock.getPhase());
                // the pulse goes high at 0 phase when clock is running
                REQUIRE(clock.getGate());
                for (unsigned i = 0; i < 9; i++) {
                    clock.process(sample_time);
                    // The first 4 processing steps will complete 5 for 50% PW.
                    // The fifth comes from the phase=0.f step before the loop.
                    REQUIRE((i < 4) == clock.getGate());
                    // processing will increase the phase by one more than i
                    const auto phase = (i + 1) * 0.1f;
                    REQUIRE((clock.getPhase() == Approx(phase)));
                    REQUIRE(clock.getTime() == Approx((1.f - phase) * 0.001f));
                }
                // wrap back around to phase of 0
                clock.process(sample_time);
                REQUIRE(0.f == clock.getPhase());
                REQUIRE(clock.getGate());
            }
        }
    }
}
