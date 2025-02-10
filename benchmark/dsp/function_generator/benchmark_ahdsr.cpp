// Benchmark code for the FunctionGenerator::ADSR structure.
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

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <random>
#include "catch.hpp"
#include "dsp/function_generator/ahdsr.hpp"

TEST_CASE("process samples with FunctionGenerator::AHDSR()") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<double>(0.0001, 0.0);
    // test the performance of sampling
    BENCHMARK("baseline") {
        for (int i = 0; i < 1000; i++) distribution(generator);
    };
    // create a single envelope generator for all tests
    auto env = FunctionGenerator::AHDSR();
    // test the performance while totally stopped
    BENCHMARK("stopped") {
        for (int i = 0; i < 1000; i++) env.process(distribution(generator));
    };
    // test the performance while looping as an LFO
    BENCHMARK("one-shot (trigger)") {
        bool running = true;
        for (int i = 0; i < 1000; i++) {
            if (running && env.isEON()) running = false;
            env.setIsGateHigh(running && env.isEOC());
            env.process(distribution(generator));
        }
    };
    // test the performance while looping as an LFO
    BENCHMARK("looping (LFO)") {
        for (int i = 0; i < 1000; i++) {
            env.setIsGateHigh(env.isEOC());
            env.process(distribution(generator));
        }
    };
}
