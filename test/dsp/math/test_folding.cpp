// Test cases for functions in Math::Folding.
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

#include "dsp/math/folding.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

const float input[30] = {
    -1.5, -1.4, -1.3, -1.2, -1.1, -1. , -0.9, -0.8, -0.7, -0.6,
    -0.5, -0.4, -0.3, -0.2, -0.1,  0. ,  0.1,  0.2,  0.3,  0.4,
     0.5,  0.6,  0.7,  0.8,  0.9,  1. ,  1.1,  1.2,  1.3,  1.4
};

// ---------------------------------------------------------------------------
// MARK: `square`
// ---------------------------------------------------------------------------

TEST_CASE("should fold using square") {
    const float output[30] = {
        -0.75, -0.84, -0.91, -0.96, -0.99, -1.  , -0.99, -0.96, -0.91,
        -0.84, -0.75, -0.64, -0.51, -0.36, -0.19,  0.  ,  0.19,  0.36,
         0.51,  0.64,  0.75,  0.84,  0.91,  0.96,  0.99,  1.  ,  0.99,
         0.96,  0.91,  0.84
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Folding::square(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `cubic`
// ---------------------------------------------------------------------------

TEST_CASE("should fold using cubic") {
    const float output[30] = {
        -0.5625, -0.728 , -0.8515, -0.936 , -0.9845, -1.    , -0.9855,
        -0.944 , -0.8785, -0.792 , -0.6875, -0.568 , -0.4365, -0.296 ,
        -0.1495,  0.    ,  0.1495,  0.296 ,  0.4365,  0.568 ,  0.6875,
         0.792 ,  0.8785,  0.944 ,  0.9855,  1.    ,  0.9845,  0.936 ,
         0.8515,  0.728
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Folding::cubic(input[i])));
}

// ---------------------------------------------------------------------------
// MARK: `polynomial`
// ---------------------------------------------------------------------------

TEST_CASE("should fold using even polynomial with order = 2") {
    const float output[30] = {
        -0.75, -0.84, -0.91, -0.96, -0.99, -1.  , -0.99, -0.96, -0.91,
        -0.84, -0.75, -0.64, -0.51, -0.36, -0.19,  0.  ,  0.19,  0.36,
         0.51,  0.64,  0.75,  0.84,  0.91,  0.96,  0.99,  1.  ,  0.99,
         0.96,  0.91,  0.84
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Folding::polynomial(input[i], 2)));
}

TEST_CASE("should fold using even polynomial with order = 4") {
    const float output[30] = {
        -0.3125    , -0.58613333, -0.7813    , -0.9088    , -0.97863333,
       - 1.        , -0.9813    , -0.93013333, -0.8533    , -0.7568    ,
        -0.64583333, -0.5248    , -0.3973    , -0.26613333, -0.1333    ,
         0.        ,  0.1333    ,  0.26613333,  0.3973    ,  0.5248    ,
         0.64583333,  0.7568    ,  0.8533    ,  0.93013333,  0.9813    ,
         1.        ,  0.97863333,  0.9088    ,  0.7813    ,  0.58613333
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Folding::polynomial(input[i], 4)));
}

TEST_CASE("should fold using odd polynomial with order = 3") {
    const float output[30] = {
        -0.5625, -0.728 , -0.8515, -0.936 , -0.9845, -1.    , -0.9855,
        -0.944 , -0.8785, -0.792 , -0.6875, -0.568 , -0.4365, -0.296 ,
        -0.1495,  0.    ,  0.1495,  0.296 ,  0.4365,  0.568 ,  0.6875,
         0.792 ,  0.8785,  0.944 ,  0.9855,  1.    ,  0.9845,  0.936 ,
         0.8515,  0.728
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Folding::polynomial(input[i], 3)));
}

TEST_CASE("should fold using odd polynomial with order = 5") {
    const float output[30] = {
        0.0234375, -0.40544  , -0.6967675, -0.87792  , -0.9723725,
        -1.       , -0.9773775, -0.91808  , -0.8329825, -0.73056  ,
        -0.6171875, -0.49744  , -0.3743925, -0.24992  , -0.1249975,
         0.       ,  0.1249975,  0.24992  ,  0.3743925,  0.49744  ,
         0.6171875,  0.73056  ,  0.8329825,  0.91808  ,  0.9773775,
         1.       ,  0.9723725,  0.87792  ,  0.6967675,  0.40544
    };
    for (unsigned i = 0; i < 30; i++)
        REQUIRE(output[i] == Approx(Math::Folding::polynomial(input[i], 5)));
}
