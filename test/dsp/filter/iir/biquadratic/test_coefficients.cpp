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

#include "dsp/filter/iir/biquadratic/coefficients.hpp"
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
// MARK: sallen_key_lowpass
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point Sallen-Key low-pass parameters with O = 0.4 * PI, Q = 10") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_lowpass(b, a, 0.4f * Math::pi<float>(), 10.f);
    float bTarget[3] = {0.32980819, 0.65961638, 0.32980819};
    float aTarget[2] = {-0.58997883, 0.90921159};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key low-pass parameters with O = 0.4 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_lowpass(b, a, 0.4f * Math::pi<float>(), 0.f);
    float bTarget[3] = {0., 0., 0.};
    float aTarget[2] = {0., -1.};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key low-pass parameters with O = 0.4 * PI, Q = 100") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_lowpass(b, a, 0.4f * Math::pi<float>(), 100.f);
    float bTarget[3] = {0.34385637, 0.68771274, 0.34385637};
    float aTarget[2] = {-0.61510897, 0.99053445};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key low-pass parameters with O = 0.8 * PI, Q = 3") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_lowpass(b, a, 0.8f * Math::pi<float>(), 3.f);
    float bTarget[3] = {0.82380508, 1.64761017, 0.82380508};
    float aTarget[2] = {1.47366733, 0.821553};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key low-pass parameters with O = 0, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_lowpass(b, a, 0.f, 1.f);
    float bTarget[3] = {0., 0., 0.};
    float aTarget[2] = {-2., 1.};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key low-pass parameters with O = PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_lowpass(b, a, Math::pi<float>(), 1.f);
    float bTarget[3] = {1., 2., 1.};
    float aTarget[2] = {2., 1.};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key low-pass parameters with O = 0, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_lowpass(b, a, 0.f, 0.f);
    REQUIRE(std::isnan(b[0]));
    REQUIRE(std::isnan(b[1]));
    REQUIRE(std::isnan(b[2]));
    REQUIRE(std::isnan(a[0]));
    REQUIRE(std::isnan(a[1]));
}

// ---------------------------------------------------------------------------
// MARK: sallen_key_highpass
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point Sallen-Key high-pass parameters with O = 0.4 * PI, Q = 10") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_highpass(b, a, 0.4f * Math::pi<float>(), 10.f);
    float bTarget[3] = {0.62479761, -1.24959521,  0.62479761};
    float aTarget[2] = {-0.58997883, 0.90921159};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key high-pass parameters with O = 0.4 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_highpass(b, a, 0.4f * Math::pi<float>(), 0.f);
    float bTarget[3] = {0., 0., 0.};
    float aTarget[2] = {0., -1.};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key high-pass parameters with O = 0.4 * PI, Q = 100") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_highpass(b, a, 0.4f * Math::pi<float>(), 100.f);
    float bTarget[3] = {0.65141085, -1.30282171,  0.65141085};
    float aTarget[2] = {-0.61510897, 0.99053445};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key high-pass parameters with O = 0.8 * PI, Q = 3") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_highpass(b, a, 0.8f * Math::pi<float>(), 3.f);
    float bTarget[3] = {0.08697142, -0.17394283,  0.08697142};
    float aTarget[2] = {1.47366733, 0.821553};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key high-pass parameters with O = 0, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_highpass(b, a, 0.f, 1.f);
    float bTarget[3] = {1., -2., 1.};
    float aTarget[2] = {-2., 1.};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key high-pass parameters with O = PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_highpass(b, a, Math::pi<float>(), 1.f);
    float bTarget[3] = {0., 0., 0.};
    float aTarget[2] = {2., 1.};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key high-pass parameters with O = 0, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_highpass(b, a, 0.f, 0.f);
    REQUIRE(std::isnan(b[0]));
    REQUIRE(std::isnan(b[1]));
    REQUIRE(std::isnan(b[2]));
    REQUIRE(std::isnan(a[0]));
    REQUIRE(std::isnan(a[1]));
}

// ---------------------------------------------------------------------------
// MARK: sallen_key_bandpass
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point Sallen-Key band-pass parameters with O = 0.4 * PI, Q = 10") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_bandpass(b, a, 0.4f * Math::pi<float>(), 10.f);
    float bTarget[3] = {0.45394203, 0., -0.45394203};
    float aTarget[2] = {-0.58997883, 0.90921159};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key band-pass parameters with O = 0.4 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_bandpass(b, a, 0.4f * Math::pi<float>(), 0.f);
    float bTarget[3] = {0., 0., 0.};
    float aTarget[2] = {0., -1.};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key band-pass parameters with O = 0.4 * PI, Q = 100") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_bandpass(b, a, 0.4f * Math::pi<float>(), 100.f);
    float bTarget[3] = {0.47327769, 0., -0.47327769};
    float aTarget[2] = {-0.61510897, 0.99053445};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key band-pass parameters with O = 0.8 * PI, Q = 3") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_bandpass(b, a, 0.8f * Math::pi<float>(), 3.f);
    float bTarget[3] = {0.2676705, 0., -0.2676705};
    float aTarget[2] = {1.47366733, 0.821553};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key band-pass parameters with O = 0, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_bandpass(b, a, 0.f, 1.f);
    float bTarget[3] = {0., 0., 0.};
    float aTarget[2] = {-2., 1.};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key band-pass parameters with O = PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_bandpass(b, a, Math::pi<float>(), 1.f);
    float bTarget[3] = {0., 0., 0.};
    float aTarget[2] = {2., 1.};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

TEST_CASE("compute floating point Sallen-Key band-pass parameters with O = 0, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::sallen_key_bandpass(b, a, 0.f, 0.f);
    REQUIRE(std::isnan(b[0]));
    REQUIRE(std::isnan(b[1]));
    REQUIRE(std::isnan(b[2]));
    REQUIRE(std::isnan(a[0]));
    REQUIRE(std::isnan(a[1]));
}

// ---------------------------------------------------------------------------
// MARK: bainter_bandstop
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0 * PI, Op = 0 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.f * Math::pi<float>(), 0.f * Math::pi<float>(), 1.f);
    float bTarget[3] = {1.0, -2.0, 1.0};
    float aTarget[2] = {-2.0, 1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0 * PI, Op = 0.1 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.f * Math::pi<float>(), 0.1f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.8449727832441988, -1.6899455664883976, 0.8449727832441988};
    float aTarget[2] = {-1.647552215703991, 0.7323389172728038};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0.4 * PI, Op = 0.3 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.4f * Math::pi<float>(), 0.3f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.863618839977648, -0.5337457964309429, 0.863618839977648};
    float aTarget[2] = {-0.8369977874388261, 0.42398568894741273};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0.4 * PI, Op = 0.4 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.4f * Math::pi<float>(), 0.4f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.6777233810861952, -0.41885608448176626, 0.6777233810861952};
    float aTarget[2] = {-0.41885608448176626, 0.3554467621723904};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0.4 * PI, Op = 0.4 * PI, Q = 10") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.4f * Math::pi<float>(), 0.4f * Math::pi<float>(), 10.f);
    float bTarget[3] = {0.954605796821967, -0.589978828293652, 0.954605796821967};
    float aTarget[2] = {-0.589978828293652, 0.909211593643934};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0.4 * PI, Op = 0.5 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.4f * Math::pi<float>(), 0.5f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.5092880150001403, -0.31475730333305296, 0.5092880150001403};
    float aTarget[2] = {-1.4802973661668756e-16, 0.33333333333333337};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0.9 * PI, Op = 1.0 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.9f * Math::pi<float>(), 1.f * Math::pi<float>(), 1.f);
    float bTarget[3] = {1.5321342793109648e-31, 2.9142925803557435e-31, 1.5321342793109648e-31};
    float aTarget[2] = {2.0, 1.0};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 1 * PI, Op = 1 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 1.0f * Math::pi<float>(), 1.0f * Math::pi<float>(), 1.f);
    float bTarget[3] = {1.0, 2.0, 1.0};
    float aTarget[2] = {2.0, 1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0 * PI, Op = 0 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.0f * Math::pi<float>(), 0.0f * Math::pi<float>(), 0.f);
    REQUIRE(std::isnan(b[0]));
    REQUIRE(std::isnan(b[1]));
    REQUIRE(std::isnan(b[2]));
    REQUIRE(std::isnan(a[0]));
    REQUIRE(std::isnan(a[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0 * PI, Op = 0.1 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.0f * Math::pi<float>(), 0.1f * Math::pi<float>(), 0.f);
    float bTarget[3] = {0.0, 0.0, 0.0};
    float aTarget[2] = {0.0, -1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 0.9 * PI, Op = 1.0 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 0.9f * Math::pi<float>(), 1.0f * Math::pi<float>(), 0.f);
    float bTarget[3] = {0.0, 0.0, 0.0};
    float aTarget[2] = {0.0, -1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point Bainter band-stop parameters with Oz = 1.0 * PI, Op = 1.0 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter_bandstop(b, a, 1.0f * Math::pi<float>(), 1.0f * Math::pi<float>(), 0.f);
    float bTarget[3] = {0.0, 0.0, 0.0};
    float aTarget[2] = {0.0, -1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

// ---------------------------------------------------------------------------
// MARK: bainter
// ---------------------------------------------------------------------------

// high shelf
TEST_CASE("compute floating point Bainter parameters with Oz = 0.5 * PI, Op = 0.4 * PI, Qz = 1, Qp = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.5f * Math::pi<float>(), 0.4f * Math::pi<float>(), 1.f, 1.f);
    float bTarget[3] = {1.3307271349906173, -1.9698718730134033e-16, 0.44357571166353915};
    float aTarget[2] = {-0.41885608448176626, 0.3554467621723904};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// low shelf
TEST_CASE("compute floating point Bainter parameters with Oz = 0.5 * PI, Op = 0.6 * PI, Qz = 1, Qp = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.5f * Math::pi<float>(), 0.6f * Math::pi<float>(), 1.f, 1.f);
    float bTarget[3] = {0.7024430082679682, -1.0398245350214102e-16, 0.23414766942265608};
    float aTarget[2] = {0.4188560844817659, 0.35544676217239035};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// high band-stop
TEST_CASE("compute floating point Bainter parameters with Oz = 0.5 * PI, Op = 0.4 * PI, Qz = 10, Qp = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.5f * Math::pi<float>(), 0.4f * Math::pi<float>(), 10.f, 1.f);
    float bTarget[3] = {0.9315089944934324, -1.5758974984107227e-16, 0.8427938521607244};
    float aTarget[2] = {-0.4188560844817663, 0.3554467621723904};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// low band-stop
TEST_CASE("compute floating point Bainter parameters with Oz = 0.5 * PI, Op = 0.6 * PI, Qz = 10, Qp = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.5f * Math::pi<float>(), 0.6f * Math::pi<float>(), 10.f, 1.f);
    float bTarget[3] = {0.4917101057875778, -8.318596280171281e-17, 0.4448805719030466};
    float aTarget[2] = {0.4188560844817659, 0.35544676217239035};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// flat band-stop
TEST_CASE("compute floating point Bainter parameters with Oz = 0.5 * PI, Op = 0.5 * PI, Qz = 10, Qp = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.5f * Math::pi<float>(), 0.5f * Math::pi<float>(), 10.f, 1.f);
    float bTarget[3] = {0.7000000000000001, -1.1842378929335005e-16, 0.6333333333333334};
    float aTarget[2] = {-1.4802973661668756e-16, 0.3333333333333333};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// flat band-pass
TEST_CASE("compute floating point Bainter parameters with Oz = 0.5 * PI, Op = 0.5 * PI, Qz = 1, Qp = 10") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.5f * Math::pi<float>(), 0.5f * Math::pi<float>(), 1.f, 10.f);
    float bTarget[3] = {1.4285714285714284, -2.1147105230955362e-16, 0.4761904761904761};
    float aTarget[2] = {-1.691768418476429e-16, 0.9047619047619048};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// low band-pass
TEST_CASE("compute floating point Bainter parameters with Oz = 0.4 * PI, Op = 0.5 * PI, Qz = 1, Qp = 10") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.4f * Math::pi<float>(), 0.5f * Math::pi<float>(), 1.f, 10.f);
    float bTarget[3] = {1.0735269395265625, -0.44965329047578995, 0.38158167475955224};
    float aTarget[2] = {-1.691768418476429e-16, 0.9047619047619048};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// high band-pass
TEST_CASE("compute floating point Bainter parameters with Oz = 0.6 * PI, Op = 0.5 * PI, Qz = 1, Qp = 10") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.6f * Math::pi<float>(), 0.5f * Math::pi<float>(), 1.f, 10.f);
    float bTarget[3] = {2.033718624510042, 0.851835419999919, 0.7228787002517818};
    float aTarget[2] = {-1.691768418476429e-16, 0.9047619047619048};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// all-pass
TEST_CASE("compute floating point Bainter parameters with Oz = 0.5 * PI, Op = 0.5 * PI, Qz = 1, Qp = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.5f * Math::pi<float>(), 0.5f * Math::pi<float>(), 1.f, 1.f);
    float bTarget[3] = {1.0, -1.4802973661668756e-16, 0.33333333333333337};
    float aTarget[2] = {-1.4802973661668756e-16, 0.33333333333333337};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// all-pass
TEST_CASE("compute floating point Bainter parameters with Oz = 0.5 * PI, Op = 0.5 * PI, Qz = 2, Qp = 2") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bainter(b, a, 0.5f * Math::pi<float>(), 0.5f * Math::pi<float>(), 2.f, 2.f);
    float bTarget[3] = {1.0, -1.7763568394002508e-16, 0.6};
    float aTarget[2] = {-1.7763568394002508e-16, 0.6};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

// ---------------------------------------------------------------------------
// MARK: allpass
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point all-pass parameters with O = 0.0 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 0.0f * Math::pi<float>(), 0.f);
    REQUIRE(std::isnan(b[0]));
    REQUIRE(std::isnan(b[1]));
    REQUIRE(std::isnan(b[2]));
    REQUIRE(std::isnan(a[0]));
    REQUIRE(std::isnan(a[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.0 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 0.0f * Math::pi<float>(), 1.f);
    float bTarget[3] = {1.0, -2.0, 1.0};
    float aTarget[2] = {-2.0, 1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.2 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 0.2f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.5457233155094577, -1.2505164308487404, 1.0};
    float aTarget[2] = {-1.2505164308487404, 0.5457233155094577};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.4 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 0.4f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.3554467621723904, -0.41885608448176626, 1.0};
    float aTarget[2] = {-0.41885608448176626, 0.3554467621723904};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.4 * PI, Q = 2") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 0.4f * Math::pi<float>(), 2.f);
    float bTarget[3] = {0.6158167400574035, -0.4993148324732645, 1.0};
    float aTarget[2] = {-0.4993148324732645, 0.6158167400574035};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.4 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 0.4f * Math::pi<float>(), 0.f);
    float bTarget[3] = {-1.0, 0.0, 1.0};
    float aTarget[2] = {0.0, -1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.6 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 0.6f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.35544676217239035, 0.4188560844817659, 1.0};
    float aTarget[2] = {0.4188560844817659, 0.35544676217239035};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 0.8 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 0.8f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.5457233155094577, 1.25051643084874, 1.0};
    float aTarget[2] = {1.25051643084874, 0.5457233155094577};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 1.0 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 1.0f * Math::pi<float>(), 1.f);
    float bTarget[3] = {1.0, 2.0, 1.0};
    float aTarget[2] = {2.0, 1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point all-pass parameters with O = 1.0 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::allpass(b, a, 1.0f * Math::pi<float>(), 0.f);
    float bTarget[3] = {-1.0, 0.0, 1.0};
    float aTarget[2] = {0.0, -1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

// ---------------------------------------------------------------------------
// MARK: bandpass
// ---------------------------------------------------------------------------

TEST_CASE("compute floating point band-pass parameters with O = 0.0 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 0.0f * Math::pi<float>(), 0.f);
    REQUIRE(std::isnan(b[0]));
    REQUIRE(std::isnan(b[1]));
    REQUIRE(std::isnan(b[2]));
    REQUIRE(std::isnan(a[0]));
    REQUIRE(std::isnan(a[1]));
}

TEST_CASE("compute floating point band-pass parameters with O = 0.0 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 0.0f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.0, 0.0, 0.0};
    float aTarget[2] = {-2.0, 1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point band-pass parameters with O = 0.4 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 0.4f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.3222766189138048, 0.0, -0.3222766189138048};
    float aTarget[2] = {-0.41885608448176626, 0.3554467621723904};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point band-pass parameters with O = 0.4 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 0.4f * Math::pi<float>(), 0.f);
    float bTarget[3] = {1.0, 0.0, -1.0};
    float aTarget[2] = {0.0, -1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point band-pass parameters with O = 0.4 * PI, Q = 10") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 0.4f * Math::pi<float>(), 10.f);
    float bTarget[3] = {0.04539420317803296, 0.0, -0.04539420317803296};
    float aTarget[2] = {-0.589978828293652, 0.909211593643934};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point band-pass parameters with O = 0.6 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 0.6f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.32227661891380477, 0.0, -0.32227661891380477};
    float aTarget[2] = {0.4188560844817659, 0.35544676217239035};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point band-pass parameters with O = 0.9 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 0.9f * Math::pi<float>(), 1.f);
    float bTarget[3] = {0.13383054136359818, 0.0, -0.13383054136359818};
    float aTarget[2] = {1.6475522157039908, 0.7323389172728038};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}

TEST_CASE("compute floating point band-pass parameters with O = 1.0 * PI, Q = 1") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 1.0f * Math::pi<float>(), 1.f);
    float bTarget[3] = {6.123233995736766e-17, 0.0, -6.123233995736766e-17};
    float aTarget[2] = {2.0, 1.0};
    REQUIRE(approx_equal(b[0], bTarget[0]));
    REQUIRE(approx_equal(b[1], bTarget[1]));
    REQUIRE(approx_equal(b[2], bTarget[2]));
    REQUIRE(approx_equal(a[0], aTarget[0]));
    REQUIRE(approx_equal(a[1], aTarget[1]));
}

TEST_CASE("compute floating point band-pass parameters with O = 1.0 * PI, Q = 0") {
    float b[3] = {0, 0, 0};
    float a[2] = {0, 0};
    Filter::IIR::Biquadratic::bandpass(b, a, 1.0f * Math::pi<float>(), 0.f);
    float bTarget[3] = {1.0, 0.0, -1.0};
    float aTarget[2] = {0.0, -1.0};
    REQUIRE(b[0] == Approx(bTarget[0]));
    REQUIRE(b[1] == Approx(bTarget[1]));
    REQUIRE(b[2] == Approx(bTarget[2]));
    REQUIRE(a[0] == Approx(aTarget[0]));
    REQUIRE(a[1] == Approx(aTarget[1]));
}
