// Benchmark code for functions in the Math namespace.
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
#include "dsp/math/functions.hpp"
#include "dsp/math/ieee754.hpp"

TEST_CASE("process arbitrary exponents") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.f, 0.f);
    // test the baseline
    BENCHMARK("baseline") {
        distribution(generator);
    };
    // base case
    BENCHMARK("std::powf computes 2**0") {
        powf(2 + distribution(generator), 0);
    };
    BENCHMARK("Math::powT2i computes 2**0") {
        Math::powT2i(2 + distribution(generator), 0);
    };
    // larger exponent
    BENCHMARK("std::powf computes 3**5") {
        powf(3 + distribution(generator), 5);
    };
    BENCHMARK("Math::powT2i computes 3**5") {
        Math::powT2i(3 + distribution(generator), 5);
    };
    // weird base value
    BENCHMARK("std::powf computes 3.14159**5") {
        powf(3.14159 + distribution(generator), 5);
    };
    BENCHMARK("Math::powT2i computes 3.14159**5") {
        Math::powT2i(3.14159 + distribution(generator), 5);
    };
    // even larger exponent
    BENCHMARK("std::powf computes 3**10") {
        powf(3 + distribution(generator), 10);
    };
    BENCHMARK("Math::powT2i computes 3**10") {
        Math::powT2i(3 + distribution(generator), 10);
    };
}

TEST_CASE("process exponents of 2") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.f, 0.f);
    // test the baseline
    BENCHMARK("baseline") {
        distribution(generator);
    };
    // 2**10
    BENCHMARK("std::powf computes 2**10") {
        powf(2 + distribution(generator), 10);
    };
    BENCHMARK("Math::powT2i computes 2**10") {
        Math::powT2i(2 + distribution(generator), 10);
    };
    BENCHMARK("Math::IEEE754::exp2 computes 2**10") {
        Math::IEEE754::exp2(10 + distribution(generator));
    };
    BENCHMARK("Math::exp2shift computes 2**10") {
        Math::exp2shift(10 + distribution(generator));
    };
    // 2**31
    BENCHMARK("std::powf computes 2**31") {
        powf(2 + distribution(generator), 31);
    };
    BENCHMARK("Math::powT2i computes 2**31") {
        Math::powT2i(2 + distribution(generator), 31);
    };
    BENCHMARK("Math::IEEE754::exp2 computes 2**31") {
        Math::IEEE754::exp2(31 + distribution(generator));
    };
    BENCHMARK("Math::exp2shift computes 2**31") {
        Math::exp2shift(31 + distribution(generator));
    };
    // 2**100
    BENCHMARK("std::powf computes 2**100") {
        powf(2 + distribution(generator), 100);
    };
    BENCHMARK("Math::powT2i computes 2**100") {
        Math::powT2i(2 + distribution(generator), 100);
    };
    BENCHMARK("Math::IEEE754::exp2 computes 2**100") {
        Math::IEEE754::exp2(100 + distribution(generator));
    };
}

// TEST_CASE("process sine functions") {
//     // a random distribution of sample times to ensure that the loop is not
//     // optimized away
//     auto generator = std::default_random_engine();
//     auto distribution = std::normal_distribution<float>(0.f, Math::PI);
//     // test the baseline
//     BENCHMARK("baseline") {
//         distribution(generator);
//     };
//     BENCHMARK("std::sin") {
//         std::sin(distribution(generator));
//     };
//     BENCHMARK("Math::sin_bhaskara") {
//         Math::sin_bhaskara(distribution(generator));
//     };
//     BENCHMARK("Math::sin_poly3") {
//         Math::sin_poly3(distribution(generator));
//     };
// }

// TEST_CASE("process cosine functions") {
//     // a random distribution of sample times to ensure that the loop is not
//     // optimized away
//     auto generator = std::default_random_engine();
//     auto distribution = std::normal_distribution<float>(0.f, Math::PI);
//     // test the baseline
//     BENCHMARK("baseline") {
//         distribution(generator);
//     };
//     BENCHMARK("std::cos") {
//         std::cos(distribution(generator));
//     };
//     BENCHMARK("Math::cos_bhaskara") {
//         Math::cos_bhaskara(distribution(generator));
//     };
// }
