// Benchmark code for the Math::Clipping functions.
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
#include "dsp/math/clipping.hpp"

TEST_CASE("process full range of values through Math::Clipping functions") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.f, 0.f);
    // test the performance of sampling
    static constexpr float YLIM = 10000.f;
    static constexpr int ITERATIONS = 4 * YLIM;
    #define X distribution(generator) + static_cast<float>(i) / YLIM - 2.f
    BENCHMARK("baseline") {
        for (int i = 0; i < ITERATIONS; i++)
            X;
    };
    BENCHMARK("hard") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::hard(X);
    };
    BENCHMARK("leaky_hard") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::leaky_hard(X);
    };
    BENCHMARK("square") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::square(X);
    };
    BENCHMARK("cubic") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::cubic(X);
    };
    BENCHMARK("polynomial") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::polynomial(X);
    };
    BENCHMARK("logistic") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::logistic(X);
    };
    BENCHMARK("arctangent") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::arctangent(X);
    };
    BENCHMARK("hyperbolic_tangent") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::hyperbolic_tangent(X);
    };
    BENCHMARK("absolute_value") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::absolute_value(X);
    };
    BENCHMARK("gudermannian") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::gudermannian(X);
    };
    BENCHMARK("square_root") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::square_root(X);
    };
    BENCHMARK("wrap") {
        for (int i = 0; i < ITERATIONS; i++)
            Math::Clipping::wrap(X);
    };
    // BENCHMARK("variable_hard") {
    //     for (int i = 0; i < ITERATIONS; i++)
    //         Math::Clipping::variable_hard(X);
    // };
}
