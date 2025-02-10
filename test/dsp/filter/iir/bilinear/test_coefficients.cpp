// Test cases for Filter::IIR namespace coefficient calculators.
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

#include "dsp/filter/iir/bilinear/coefficients.hpp"
#include "dsp/math/constants.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

/// @brief Return true if the given floats are approximately equal.
///
/// @param a the first operand
/// @param b the second operand
/// @param p the number of digits of precision
/// @returns true if the floating point numbers are approximately equal
/// @details
/// This is best suited for floats in the range [-1, 1]
///
inline bool approx_equal(float a, float b, int p = 7) {
    return (roundf(a * p) / p) == (roundf(b * p) / p);
}

// ---------------------------------------------------------------------------
// MARK: lowpass
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point low-pass parameters with O = 0.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowpass(b, a, 0.0f * Math::pi<float>());
    float bTarget[2] = {0.0, 0.0};
    float aTarget[1] = {-1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point low-pass parameters with O = 0.2 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowpass(b, a, 0.2f * Math::pi<float>());
    float bTarget[2] = {0.24523728, 0.24523728};
    float aTarget[1] = {-0.50952545};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point low-pass parameters with O = 0.4 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowpass(b, a, 0.4f * Math::pi<float>());
    float bTarget[2] = {0.42080778, 0.42080778};
    float aTarget[1] = {-0.15838444};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point low-pass parameters with O = 0.6 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowpass(b, a, 0.6f * Math::pi<float>());
    float bTarget[2] = {0.57919222, 0.57919222};
    float aTarget[1] = {0.15838444};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point low-pass parameters with O = 0.8 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowpass(b, a, 0.8f * Math::pi<float>());
    float bTarget[2] = {0.75476272, 0.75476272};
    float aTarget[1] = {0.50952545};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point low-pass parameters with O = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowpass(b, a, 1.0f * Math::pi<float>());
    float bTarget[2] = {1.0, 1.0};
    float aTarget[1] = {1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

// ---------------------------------------------------------------------------
// MARK: highpass
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point high-pass parameters with O = 0.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highpass(b, a, 0.0f * Math::pi<float>());
    float bTarget[2] = {1.0, -1.0};
    float aTarget[1] = {-1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point high-pass parameters with O = 0.2 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highpass(b, a, 0.2f * Math::pi<float>());
    float bTarget[2] = {0.75476272, -0.75476272};
    float aTarget[1] = {-0.50952545};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point high-pass parameters with O = 0.4 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highpass(b, a, 0.4f * Math::pi<float>());
    float bTarget[2] = {0.57919222, -0.57919222};
    float aTarget[1] = {-0.15838444};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point high-pass parameters with O = 0.6 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highpass(b, a, 0.6f * Math::pi<float>());
    float bTarget[2] = {0.42080778, -0.42080778};
    float aTarget[1] = {0.15838444};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point high-pass parameters with O = 0.8 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highpass(b, a, 0.8f * Math::pi<float>());
    float bTarget[2] = {0.24523728, -0.24523728};
    float aTarget[1] = {0.50952545};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point high-pass parameters with O = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highpass(b, a, 1.0f * Math::pi<float>());
    float bTarget[2] = {6.123234e-17, -6.123234e-17};
    float aTarget[1] = {1.0};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

// ---------------------------------------------------------------------------
// MARK: allpass
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point all-pass parameters with O = 0.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::allpass(b, a, 0.0f * Math::pi<float>());
    float bTarget[2] = {1.0, -1.0};
    float aTarget[1] = {-1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.2 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::allpass(b, a, 0.2f * Math::pi<float>());
    float bTarget[2] = {0.50952545, -1.0};
    float aTarget[1] = {-0.50952545};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.4 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::allpass(b, a, 0.4f * Math::pi<float>());
    float bTarget[2] = {0.15838444, -1.0};
    float aTarget[1] = {-0.15838444};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.6 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::allpass(b, a, 0.6f * Math::pi<float>());
    float bTarget[2] = {-0.15838444, -1.0};
    float aTarget[1] = {0.15838444};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.8 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::allpass(b, a, 0.8f * Math::pi<float>());
    float bTarget[2] = {-0.50952545, -1.0};
    float aTarget[1] = {0.50952545};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point all-pass parameters with O = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::allpass(b, a, 1.0f * Math::pi<float>());
    float bTarget[2] = {-1.0, -1.0};
    float aTarget[1] = {1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

// ---------------------------------------------------------------------------
// MARK: lowshelf
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point low-shelf parameters with Oz = 0.0 * PI, O1 = 0.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowshelf(b, a, 0.0f * Math::pi<float>(), 0.0f * Math::pi<float>());
    float bTarget[2] = {1.0, -1.0};
    float aTarget[1] = {-1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point low-shelf parameters with Oz = 0.0 * PI, O1 = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowshelf(b, a, 0.0f * Math::pi<float>(), 1.0f * Math::pi<float>());
    float bTarget[2] = {6.123234e-17, -6.123234e-17};
    float aTarget[1] = {1.0};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

// TODO: why does this overflow?
// TEST_CASE("compute floating point low-shelf parameters with Oz = 1.0 * PI, O1 = 0.0 * PI") {
//     float b[2] = {0, 0};
//     float a[1] = {0};
//     Filter::IIR::Bilinear::lowshelf(b, a, 1.0f * Math::pi<float>(), 0.0f * Math::pi<float>());
//     float bTarget[2] = {1.63312394e+16, 1.63312394e+16};
//     float aTarget[1] = {-1.0};
//     REQUIRE(b[0] == Approx(bTarget[0]));
//     REQUIRE(b[1] == Approx(bTarget[1]));
//     REQUIRE(a[0] == Approx(aTarget[0]));
// }

TEST_CASE("compute floating point low-shelf parameters with Oz = 1.0 * PI, O1 = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowshelf(b, a, 1.0f * Math::pi<float>(), 1.0f * Math::pi<float>());
    float bTarget[2] = {1.0, 1.0};
    float aTarget[1] = {1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point low-shelf parameters with Oz = 0.0 * PI, O1 = 0.5 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowshelf(b, a, 0.0f * Math::pi<float>(), 0.5f * Math::pi<float>());
    float bTarget[2] = {0.5, -0.5};
    float aTarget[1] = {-5.55111512e-17};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

TEST_CASE("compute floating point low-shelf parameters with Oz = 0.5 * PI, O1 = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowshelf(b, a, 0.5f * Math::pi<float>(), 1.0f * Math::pi<float>());
    float bTarget[2] = {1.22464680e-16, -6.79815537e-33};
    float aTarget[1] = {1.0};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

TEST_CASE("compute floating point low-shelf parameters with Oz = 0.4 * PI, O1 = 0.5 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowshelf(b, a, 0.4f * Math::pi<float>(), 0.5f * Math::pi<float>());
    float bTarget[2] = {0.86327126, -0.13672874};
    float aTarget[1] = {-5.55111512e-17};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

TEST_CASE("compute floating point low-shelf parameters with Oz = 0.5 * PI, O1 = 0.4 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::lowshelf(b, a, 0.5f * Math::pi<float>(), 0.4f * Math::pi<float>());
    float bTarget[2] = {1.15838444e+00, -6.43032539e-17};
    float aTarget[1] = {-0.15838444};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

// ---------------------------------------------------------------------------
// MARK: highshelf
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point high-shelf parameters with Oz = 0.0 * PI, O1 = 0.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highshelf(b, a, 0.0f * Math::pi<float>(), 0.0f * Math::pi<float>());
    float bTarget[2] = {1.0, 1.0};
    float aTarget[1] = {1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point high-shelf parameters with Oz = 0.0 * PI, O1 = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highshelf(b, a, 0.0f * Math::pi<float>(), 1.0f * Math::pi<float>());
    float bTarget[2] = {6.123234e-17, 6.123234e-17};
    float aTarget[1] = {-1.0};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

// TODO: why does this overflow?
// TEST_CASE("compute floating point high-shelf parameters with Oz = 1.0 * PI, O1 = 0.0 * PI") {
//     float b[2] = {0, 0};
//     float a[1] = {0};
//     Filter::IIR::Bilinear::highshelf(b, a, 1.0f * Math::pi<float>(), 0.0f * Math::pi<float>());
//     float bTarget[2] = {1.63312394e+16, -1.63312394e+16};
//     float aTarget[1] = {1.0};
//     REQUIRE(b[0] == Approx(bTarget[0]));
//     REQUIRE(b[1] == Approx(bTarget[1]));
//     REQUIRE(a[0] == Approx(aTarget[0]));
// }

TEST_CASE("compute floating point high-shelf parameters with Oz = 1.0 * PI, O1 = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highshelf(b, a, 1.0f * Math::pi<float>(), 1.0f * Math::pi<float>());
    float bTarget[2] = {1.0, -1.0};
    float aTarget[1] = {-1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(a[0] == Approx(aTarget[0]));
}

TEST_CASE("compute floating point high-shelf parameters with Oz = 0.0 * PI, O1 = 0.5 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highshelf(b, a, 0.0f * Math::pi<float>(), 0.5f * Math::pi<float>());
    float bTarget[2] = {0.5, 0.5};
    float aTarget[1] = {5.55111512e-17};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

TEST_CASE("compute floating point high-shelf parameters with Oz = 0.5 * PI, O1 = 1.0 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highshelf(b, a, 0.5f * Math::pi<float>(), 1.0f * Math::pi<float>());
    float bTarget[2] = {1.22464680e-16, 6.79815537e-33};
    float aTarget[1] = {-1.0};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

TEST_CASE("compute floating point high-shelf parameters with Oz = 0.4 * PI, O1 = 0.5 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highshelf(b, a, 0.4f * Math::pi<float>(), 0.5f * Math::pi<float>());
    float bTarget[2] = {0.86327126, 0.13672874};
    float aTarget[1] = {5.55111512e-17};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}

TEST_CASE("compute floating point high-shelf parameters with Oz = 0.5 * PI, O1 = 0.4 * PI") {
    float b[2] = {0, 0};
    float a[1] = {0};
    Filter::IIR::Bilinear::highshelf(b, a, 0.5f * Math::pi<float>(), 0.4f * Math::pi<float>());
    float bTarget[2] = {1.15838444e+00, 6.43032539e-17};
    float aTarget[1] = {0.15838444};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
}
