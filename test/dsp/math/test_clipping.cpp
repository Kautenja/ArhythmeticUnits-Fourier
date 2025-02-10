// Test cases for functions in Math::Clipping.
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

#include "dsp/math/clipping.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

const float input[30] = {
    -1.5, -1.4, -1.3, -1.2, -1.1, -1. , -0.9, -0.8, -0.7, -0.6,
    -0.5, -0.4, -0.3, -0.2, -0.1,  0. ,  0.1,  0.2,  0.3,  0.4,
     0.5,  0.6,  0.7,  0.8,  0.9,  1. ,  1.1,  1.2,  1.3,  1.4
};

// ---------------------------------------------------------------------------
// MARK: `hard`
// ---------------------------------------------------------------------------

TEST_CASE("should compute hard") {
    const float output[30] = {
        -1. , -1. , -1. , -1. , -1. , -1. , -0.9, -0.8, -0.7, -0.6, -0.5,
        -0.4, -0.3, -0.2, -0.1,  0. ,  0.1,  0.2,  0.3,  0.4,  0.5,  0.6,
         0.7,  0.8,  0.9,  1. ,  1. ,  1. ,  1. ,  1.
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::hard(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `leaky_hard`
// ---------------------------------------------------------------------------

TEST_CASE("should compute leaky_hard with m = 0.1") {
    const float output[30] = {
        -1.05, -1.04, -1.03, -1.02, -1.01, -1.  , -0.9 , -0.8 , -0.7 ,
        -0.6 , -0.5 , -0.4 , -0.3 , -0.2 , -0.1 ,  0.  ,  0.1 ,  0.2 ,
         0.3 ,  0.4 ,  0.5 ,  0.6 ,  0.7 ,  0.8 ,  0.9 ,  1.  ,  1.01,
         1.02,  1.03,  1.04
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::leaky_hard(input[i], 0.1f)));
}

TEST_CASE("should compute leaky_hard with m = 0.5") {
    const float output[30] = {
        -1.25, -1.2 , -1.15, -1.1 , -1.05, -1.  , -0.9 , -0.8 , -0.7 ,
        -0.6 , -0.5 , -0.4 , -0.3 , -0.2 , -0.1 ,  0.  ,  0.1 ,  0.2 ,
         0.3 ,  0.4 ,  0.5 ,  0.6 ,  0.7 ,  0.8 ,  0.9 ,  1.  ,  1.05,
         1.1 ,  1.15,  1.2
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::leaky_hard(input[i], 0.5f)));
}

// ---------------------------------------------------------------------------
// MARK: `square`
// ---------------------------------------------------------------------------

TEST_CASE("should compute square") {
    const float output[30] = {
        -1.  , -1.  , -1.  , -1.  , -1.  , -1.  , -0.99, -0.96, -0.91,
        -0.84, -0.75, -0.64, -0.51, -0.36, -0.19,  0.  ,  0.19,  0.36,
         0.51,  0.64,  0.75,  0.84,  0.91,  0.96,  0.99,  1.  ,  1.  ,
         1.  ,  1.  ,  1.
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::square(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `cubic`
// ---------------------------------------------------------------------------

TEST_CASE("should compute cubic") {
    const float output[30] = {
        -1.    , -1.    , -1.    , -1.    , -1.    , -1.    , -0.9855,
        -0.944 , -0.8785, -0.792 , -0.6875, -0.568 , -0.4365, -0.296 ,
        -0.1495,  0.    ,  0.1495,  0.296 ,  0.4365,  0.568 ,  0.6875,
         0.792 ,  0.8785,  0.944 ,  0.9855,  1.    ,  1.    ,  1.    ,
         1.    ,  1.
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::cubic(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `polynomial`
// ---------------------------------------------------------------------------

TEST_CASE("should compute even polynomial with order = 2") {
    const float output[30] = {
        -1.  , -1.  , -1.  , -1.  , -1.  , -1.  , -0.99, -0.96, -0.91,
        -0.84, -0.75, -0.64, -0.51, -0.36, -0.19,  0.  ,  0.19,  0.36,
         0.51,  0.64,  0.75,  0.84,  0.91,  0.96,  0.99,  1.  ,  1.  ,
         1.  ,  1.  ,  1.
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::polynomial(input[i], 2)));
}

TEST_CASE("should compute even polynomial with order = 4") {
    const float output[30] = {
        -1.        , -1.        , -1.        , -1.        , -1.        ,
        -1.        , -0.9813    , -0.93013333, -0.8533    , -0.7568    ,
        -0.64583333, -0.5248    , -0.3973    , -0.26613333, -0.1333    ,
         0.        ,  0.1333    ,  0.26613333,  0.3973    ,  0.5248    ,
         0.64583333,  0.7568    ,  0.8533    ,  0.93013333,  0.9813    ,
         1.        ,  1.        ,  1.        ,  1.        ,  1.
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::polynomial(input[i], 4)));
}

TEST_CASE("should compute odd polynomial with order = 3") {
    const float output[30] = {
        -1.    , -1.    , -1.    , -1.    , -1.    , -1.    , -0.9855,
        -0.944 , -0.8785, -0.792 , -0.6875, -0.568 , -0.4365, -0.296 ,
        -0.1495,  0.    ,  0.1495,  0.296 ,  0.4365,  0.568 ,  0.6875,
         0.792 ,  0.8785,  0.944 ,  0.9855,  1.    ,  1.    ,  1.    ,
         1.    ,  1.
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::polynomial(input[i], 3)));
}

TEST_CASE("should compute odd polynomial with order = 5") {
    const float output[30] = {
        -1.       , -1.       , -1.       , -1.       , -1.       ,
        -1.       , -0.9773775, -0.91808  , -0.8329825, -0.73056  ,
        -0.6171875, -0.49744  , -0.3743925, -0.24992  , -0.1249975,
         0.       ,  0.1249975,  0.24992  ,  0.3743925,  0.49744  ,
         0.6171875,  0.73056  ,  0.8329825,  0.91808  ,  0.9773775,
         1.       ,  1.       ,  1.       ,  1.       ,  1.
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::polynomial(input[i], 5)));
}

// ---------------------------------------------------------------------------
// MARK: `logistic`
// ---------------------------------------------------------------------------

TEST_CASE("should compute logistic") {
    const float output[30] = {
        -0.63514895, -0.60436778, -0.57166997, -0.53704957, -0.50052021,
        -0.46211716, -0.42189901, -0.37994896, -0.33637554, -0.29131261,
        -0.24491866, -0.19737532, -0.14888503, -0.09966799, -0.04995837,
         0.        ,  0.04995837,  0.09966799,  0.14888503,  0.19737532,
         0.24491866,  0.29131261,  0.33637554,  0.37994896,  0.42189901,
         0.46211716,  0.50052021,  0.53704957,  0.57166997,  0.60436778
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::logistic(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `arctangent`
// ---------------------------------------------------------------------------

TEST_CASE("should compute arctangent") {
    const float output[30] = {
        -0.74447769, -0.72830398, -0.71009623, -0.68948125, -0.66600102,
        -0.63909293, -0.60806798, -0.57209014, -0.53016483, -0.48115341,
        -0.42384473, -0.35713231, -0.28035152, -0.19378438, -0.0991895 ,
         0.        ,  0.0991895 ,  0.19378438,  0.28035152,  0.35713231,
         0.42384473,  0.48115341,  0.53016483,  0.57209014,  0.60806798,
         0.63909293,  0.66600102,  0.68948125,  0.71009623,  0.72830398
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::arctangent(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `hyperbolic_tangent`
// ---------------------------------------------------------------------------

TEST_CASE("should compute hyperbolic tangent") {
    const float output[30] = {
        -0.90514825, -0.88535165, -0.86172316, -0.83365461, -0.80049902,
        -0.76159416, -0.71629787, -0.66403677, -0.60436778, -0.53704957,
        -0.46211716, -0.37994896, -0.29131261, -0.19737532, -0.09966799,
         0.        ,  0.09966799,  0.19737532,  0.29131261,  0.37994896,
         0.46211716,  0.53704957,  0.60436778,  0.66403677,  0.71629787,
         0.76159416,  0.80049902,  0.83365461,  0.86172316,  0.88535165
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::hyperbolic_tangent(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `absolute_value`
// ---------------------------------------------------------------------------

TEST_CASE("should compute absolute value") {
    const float output[30] = {
        -0.6       , -0.58333333, -0.56521739, -0.54545455, -0.52380952,
        -0.5       , -0.47368421, -0.44444444, -0.41176471, -0.375     ,
        -0.33333333, -0.28571429, -0.23076923, -0.16666667, -0.09090909,
         0.        ,  0.09090909,  0.16666667,  0.23076923,  0.28571429,
         0.33333333,  0.375     ,  0.41176471,  0.44444444,  0.47368421,
         0.5       ,  0.52380952,  0.54545455,  0.56521739,  0.58333333
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::absolute_value(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `gudermannian`
// ---------------------------------------------------------------------------

TEST_CASE("should compute gudermannian") {
    const float output[30] = {
        -1.13172835, -1.08724983, -1.03865614, -0.98569224, -0.92812738,
        -0.86576948, -0.7984823 , -0.72620482, -0.64897208, -0.56693556,
        -0.48038108, -0.38974112, -0.29559868, -0.19867985, -0.09983375,
         0.        ,  0.09983375,  0.19867985,  0.29559868,  0.38974112,
         0.48038108,  0.56693556,  0.64897208,  0.72620482,  0.7984823 ,
         0.86576948,  0.92812738,  0.98569224,  1.03865614,  1.08724983
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::gudermannian(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `square_root`
// ---------------------------------------------------------------------------

TEST_CASE("should compute square root") {
    const float output[30] = {
        -0.83205029, -0.81373347, -0.79262399, -0.76822128, -0.73994007,
        -0.70710678, -0.66896473, -0.62469505, -0.57346234, -0.51449576,
        -0.4472136 , -0.37139068, -0.28734789, -0.19611614, -0.09950372,
         0.        ,  0.09950372,  0.19611614,  0.28734789,  0.37139068,
         0.4472136 ,  0.51449576,  0.57346234,  0.62469505,  0.66896473,
         0.70710678,  0.73994007,  0.76822128,  0.79262399,  0.81373347
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::square_root(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `wrap`
// ---------------------------------------------------------------------------

TEST_CASE("should compute wrap with k = 2") {
    const float output[30] = {
         0.5,  0.6,  0.7,  0.8,  0.9,  1. , -0.9, -0.8, -0.7, -0.6, -0.5,
        -0.4, -0.3, -0.2, -0.1,  0. ,  0.1,  0.2,  0.3,  0.4,  0.5,  0.6,
         0.7,  0.8,  0.9, -1. , -0.9, -0.8, -0.7, -0.6
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::wrap(input[i], 2.f)));
}

TEST_CASE("should compute wrap with k = 1") {
    const float output[30] = {
        -0.5, -0.4, -0.3, -0.2, -0.1,  0. , -0.9, -0.8, -0.7, -0.6, -0.5,
        -0.4, -0.3, -0.2, -0.1,  0. ,  0.1,  0.2,  0.3,  0.4,  0.5,  0.6,
         0.7,  0.8,  0.9,  0. ,  0.1,  0.2,  0.3,  0.4
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Clipping::wrap(input[i], 1.f)));
}

// ---------------------------------------------------------------------------
// MARK: `variable_hard`
// ---------------------------------------------------------------------------

// TEST_CASE("should compute variable_hard with k = 10") {
//     const float output[30] = {
//         -1.04503277, -1.04386867, -1.04126757, -1.03500548, -1.01863797,
//         -0.97613298, -0.89653833, -0.79969414, -0.69998139, -0.59999927,
//         -0.49999998, -0.4       , -0.3       , -0.2       , -0.1       ,
//          0.        ,  0.1       ,  0.2       ,  0.3       ,  0.4       ,
//          0.49999998,  0.59999927,  0.69998139,  0.79969414,  0.89653833,
//          0.97613298,  1.01863797,  1.03500548,  1.04126757,  1.04386867
//     };
//     for (unsigned i = 0; i < 30; i++)
//         REQUIRE(output[i] == Approx(Math::Clipping::variable_hard(input[i], 10.f)));
// }

// TEST_CASE("should compute variable_hard with k = 100") {
//     const float output[30] = {
//         -1.00452604, -1.00452604, -1.00452604, -1.00452604, -1.00452557,
//         -0.99758727, -0.9       , -0.8       , -0.7       , -0.6       ,
//         -0.5       , -0.4       , -0.3       , -0.2       , -0.1       ,
//          0.        ,  0.1       ,  0.2       ,  0.3       ,  0.4       ,
//          0.5       ,  0.6       ,  0.7       ,  0.8       ,  0.9       ,
//          0.99758727,  1.00452557,  1.00452604,  1.00452604,  1.00452604
//     };
//     for (unsigned i = 0; i < 30; i++)
//         REQUIRE(output[i] == Approx(Math::Clipping::variable_hard(input[i], 100.f)));
// }

// TEST_CASE("should compute variable_hard with k = 1000") {
//     const float output[30] = {
//         -1.00045168, -1.00045168, -1.00045168, -1.00045168, -1.00045168,
//         -0.99975846, -0.9       , -0.8       , -0.7       , -0.6       ,
//         -0.5       , -0.        , -0.        , -0.        , -0.        ,
//          0.        ,  0.        ,  0.        ,  0.        ,  0.        ,
//          0.5       ,  0.6       ,  0.7       ,  0.8       ,  0.9       ,
//          0.99975846,  1.00045168,  1.00045168,  1.00045168,  1.00045168
//     };
//     for (unsigned i = 0; i < 30; i++){
//         REQUIRE(output[i] == Approx(Math::Clipping::variable_hard(input[i], 1000.f)));
//     }
// }
