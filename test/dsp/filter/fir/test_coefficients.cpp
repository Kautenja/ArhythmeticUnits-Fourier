// Test cases for Filter::FIR coefficient calculation functions.
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

#include "dsp/filter/fir/coefficients.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: Filter::FIR::lowpass
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an impulse train with order of 15 and a cut-off of 0.0") {
    const std::vector<float> reference = {
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto impulse = Filter::FIR::lowpass<float>(i, reference.size(), 0.0);
        REQUIRE(Math::IEEE754::approx_equal(reference[i], impulse, 1e-6f));
    }
}

TEST_CASE("Calculate an impulse train with order of 16 and a cut-off of 0.0") {
    const std::vector<float> reference = {
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto impulse = Filter::FIR::lowpass<float>(i, reference.size(), 0.0);
        REQUIRE(Math::IEEE754::approx_equal(reference[i], impulse, 1e-6f));
    }
}

TEST_CASE("Calculate an impulse train with order of 15 and a cut-off of 0.5") {
    const std::vector<float> reference = {
        -0.030010543871903578,
        -0.034627550621427174,
         0.04092346891623215,
         0.05001757311983922,
        -0.06430830829693616,
        -0.09003163161571061,
         0.1500527193595177,
         0.45015815807855303,
         0.45015815807855303,
         0.1500527193595177,
        -0.09003163161571061,
        -0.06430830829693616,
         0.05001757311983922,
         0.04092346891623215,
        -0.034627550621427174
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto impulse = Filter::FIR::lowpass<float>(i, reference.size(), 0.5);
        REQUIRE(Math::IEEE754::approx_equal(reference[i], impulse, 1e-6f));
    }
}

TEST_CASE("Calculate an impulse train with order of 16 and a cut-off of 0.5") {
    const std::vector<float> reference = {
        -0.030010543871903578,
        -0.034627550621427174,
         0.04092346891623215,
         0.05001757311983922,
        -0.06430830829693616,
        -0.09003163161571061,
         0.1500527193595177,
         0.45015815807855303,
         0.45015815807855303,
         0.1500527193595177,
        -0.09003163161571061,
        -0.06430830829693616,
         0.05001757311983922,
         0.04092346891623215,
        -0.034627550621427174,
        -0.030010543871903578
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto impulse = Filter::FIR::lowpass<float>(i, reference.size(), 0.5);
        REQUIRE(Math::IEEE754::approx_equal(reference[i], impulse, 1e-6f));
    }
}

TEST_CASE("Calculate an impulse train with order of 15 and a cut-off of 1.0") {
    const std::vector<float> reference = {
        -0.04244131815783876,
         0.048970751720583176,
        -0.057874524760689224,
         0.0707355302630646,
        -0.09094568176679733,
         0.12732395447351627,
        -0.2122065907891938,
         0.6366197723675814,
         0.6366197723675814,
        -0.2122065907891938,
         0.12732395447351627,
        -0.09094568176679733,
         0.0707355302630646,
        -0.057874524760689224,
         0.048970751720583176
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto impulse = Filter::FIR::lowpass<float>(i, reference.size(), 1.0);
        REQUIRE(Math::IEEE754::approx_equal(reference[i], impulse, 1e-6f));
    }
}

TEST_CASE("Calculate an impulse train with order of 16 and a cut-off of 1.0") {
    const std::vector<float> reference = {
        -0.04244131815783876,
         0.048970751720583176,
        -0.057874524760689224,
         0.0707355302630646,
        -0.09094568176679733,
         0.12732395447351627,
        -0.2122065907891938,
         0.6366197723675814,
         0.6366197723675814,
        -0.2122065907891938,
         0.12732395447351627,
        -0.09094568176679733,
         0.0707355302630646,
        -0.057874524760689224,
         0.048970751720583176,
        -0.04244131815783876
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto impulse = Filter::FIR::lowpass<float>(i, reference.size(), 1.0);
        REQUIRE(Math::IEEE754::approx_equal(reference[i], impulse, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Filter::FIR::highpass
// ---------------------------------------------------------------------------

// TODO

// ---------------------------------------------------------------------------
// MARK: Filter::FIR::bandpass
// ---------------------------------------------------------------------------

// TODO

// ---------------------------------------------------------------------------
// MARK: Filter::FIR::bandstop
// ---------------------------------------------------------------------------

// TODO
