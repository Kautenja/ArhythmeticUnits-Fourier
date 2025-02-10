// Benchmark code for the FunctionGenerator::TriggerIntegrator structure.
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
#include "dsp/function_generator/trigger_integrator.hpp"

TEST_CASE("process samples with FunctionGenerator::TriggerIntegrator<float>()") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<double>(0.0001, 0.0);
    // test the performance of sampling
    BENCHMARK("baseline") {
        for (int i = 0; i < 1000; i++) distribution(generator);
    };
    // create a single envelope generator for all tests
    auto env = FunctionGenerator::TriggerIntegrator<float>();
    BENCHMARK("stopped") {
        for (int i = 0; i < 1000; i++)
            env.process(0.f, distribution(generator));
    };
    BENCHMARK("rise to sustain") {
        bool running = true;
        for (int i = 0; i < 1000; i++) {
            if (running && env.isEOR()) running = false;
            auto signal = static_cast<float>(running && env.isEOF());
            env.process(signal, distribution(generator));
        }
    };
    BENCHMARK("one-shot (trigger)") {
        env.trigger();
        for (int i = 0; i < 1000; i++)
            env.process(0.f, distribution(generator));
    };
    BENCHMARK("looping (LFO)") {
        env.isCycling = true;
        for (int i = 0; i < 1000; i++)
            env.process(1.f, distribution(generator));
    };
}

TEST_CASE("process samples with FunctionGenerator::TriggerIntegrator<double>()") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<double>(0.0001, 0.0);
    // test the performance of sampling
    BENCHMARK("baseline") {
        for (int i = 0; i < 1000; i++) distribution(generator);
    };
    // create a single envelope generator for all tests
    auto env = FunctionGenerator::TriggerIntegrator<double>();
    BENCHMARK("stopped") {
        for (int i = 0; i < 1000; i++)
            env.process(0.f, distribution(generator));
    };
    BENCHMARK("rise to sustain") {
        bool running = true;
        for (int i = 0; i < 1000; i++) {
            if (running && env.isEOR()) running = false;
            auto signal = static_cast<double>(running && env.isEOF());
            env.process(signal, distribution(generator));
        }
    };
    BENCHMARK("one-shot (trigger)") {
        env.trigger();
        for (int i = 0; i < 1000; i++)
            env.process(0.f, distribution(generator));
    };
    BENCHMARK("looping (LFO)") {
        env.isCycling = true;
        for (int i = 0; i < 1000; i++)
            env.process(1.f, distribution(generator));
    };
}
