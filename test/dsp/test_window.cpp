// Test cases for Math windowing methods.
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

#include <cstddef>

#include "dsp/window.hpp"
#include "../ieee754.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: Bartlett Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Bartlett window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.2857142857142857,
        0.5714285714285714,
        0.8571428571428571,
        0.8571428571428572,
        0.5714285714285714,
        0.2857142857142858
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::bartlett<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Bartlett window impulse response of length 8") {
    const std::vector<float> reference = {0.0, 0.25, 0.5, 0.75, 1.0, 0.75, 0.5, 0.25};
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::bartlett<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Bartlett window impulse response of length 7") {
    const std::vector<float> reference = {
        0.f / 3.f,
        1.f / 3.f,
        2.f / 3.f,
        3.f / 3.f,
        2.f / 3.f,
        1.f / 3.f,
        0.f / 3.f
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::bartlett<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Bartlett window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.2857142857142857,
        0.5714285714285714,
        0.8571428571428571,
        0.8571428571428572,
        0.5714285714285714,
        0.2857142857142858,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::bartlett<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Hann Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Hann window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.18825509907063326,
        0.6112604669781572,
        0.9504844339512095,
        0.9504844339512095,
        0.6112604669781572,
        0.18825509907063326
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hann<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Hann window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.14644660940672627,
        0.5,
        0.8535533905932737,
        1.0,
        0.8535533905932737,
        0.5,
        0.14644660940672627
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hann<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Hann window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.2499999999999999,
        0.75,
        1.0,
        0.7500000000000002,
        0.25000000000000033,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hann<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Hann window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.18825509907063326,
        0.6112604669781572,
        0.9504844339512095,
        0.9504844339512095,
        0.6112604669781572,
        0.18825509907063326,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hann<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Bartlett-Hann Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Bartlett-Hann window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.21164530386510988,
        0.6017008120462566,
        0.9280824555172049,
        0.9280824555172051,
        0.6017008120462566,
        0.21164530386511007
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::barthann<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Bartlett-Hann window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.17129942314911195,
        0.5,
        0.8287005768508882,
        1.0,
        0.8287005768508882,
        0.5,
        0.17129942314911195
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::barthann<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Bartlett-Hann window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.2699999999999999,
        0.73,
        1.0,
        0.7300000000000002,
        0.2699999999999999,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::barthann<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Bartlett-Hann window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.21164530386510988,
        0.6017008120462566,
        0.9280824555172049,
        0.9280824555172051,
        0.6017008120462566,
        0.21164530386511007,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::barthann<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Hamming Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Hamming window impulse response of length 7") {
    const std::vector<float> reference = {
        0.08000000000000007,
        0.25319469114498266,
        0.6423596296199047,
        0.9544456792351128,
        0.9544456792351128,
        0.6423596296199047,
        0.25319469114498266
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hamming<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Hamming window impulse response of length 8") {
    const std::vector<float> reference = {
        0.08000000000000007,
        0.21473088065418822,
        0.54,
        0.865269119345812,
        1.0,
        0.865269119345812,
        0.54,
        0.21473088065418822
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hamming<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Hamming window impulse response of length 7") {
    const std::vector<float> reference = {
        0.08000000000000007,
        0.30999999999999994,
        0.77,
        1.0,
        0.7700000000000002,
        0.31000000000000033,
        0.08000000000000007
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hamming<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Hamming window impulse response of length 8") {
    const std::vector<float> reference = {
        0.08000000000000007,
        0.25319469114498266,
        0.6423596296199047,
        0.9544456792351128,
        0.9544456792351128,
        0.6423596296199047,
        0.25319469114498266,
        0.08000000000000007
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hamming<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Blackman Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Blackman window impulse response of length 7") {
    const std::vector<float> reference = {
        -1.3877787807814457e-17,
        0.09045342435412808,
        0.45918295754596367,
        0.9203636180999082,
        0.9203636180999082,
        0.45918295754596367,
        0.09045342435412808
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackman<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Blackman window impulse response of length 8") {
    const std::vector<float> reference = {
        -1.3877787807814457e-17,
        0.06644660940672624,
        0.34,
        0.7735533905932738,
        0.9999999999999999,
        0.7735533905932738,
        0.34,
        0.06644660940672624
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackman<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Blackman window impulse response of length 7") {
    const std::vector<float> reference = {
        -1.3877787807814457e-17,
        0.1299999999999999,
        0.6299999999999999,
        0.9999999999999999,
        0.6300000000000002,
        0.1300000000000002,
        -1.3877787807814457e-17
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackman<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Blackman window impulse response of length 8") {
    const std::vector<float> reference = {
        -1.3877787807814457e-17,
        0.09045342435412808,
        0.45918295754596367,
        0.9203636180999082,
        0.9203636180999082,
        0.45918295754596367,
        0.09045342435412808,
        -1.3877787807814457e-17
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackman<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Blackman-Harris Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Blackman-Harris window impulse response of length 7") {
    const std::vector<float> reference = {
        6.0000000000001025e-05,
        0.03339172347815125,
        0.33283350429856506,
        0.8893697722232838,
        0.8893697722232838,
        0.33283350429856506,
        0.03339172347815125
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackmanharris<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Blackman-Harris window impulse response of length 8") {
    const std::vector<float> reference = {
        6.0000000000001025e-05,
        0.021735837018679628,
        0.21747000000000008,
        0.6957641629813204,
        1.0,
        0.6957641629813204,
        0.21747000000000008,
        0.021735837018679628
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackmanharris<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Blackman-Harris window impulse response of length 7") {
    const std::vector<float> reference = {
        6.0000000000001025e-05,
        0.055645,
        0.5205749999999999,
        1.0,
        0.5205750000000002,
        0.05564500000000015,
        6.0000000000001025e-05
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackmanharris<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Blackman-Harris window impulse response of length 8") {
    const std::vector<float> reference = {
        6.0000000000001025e-05,
        0.03339172347815125,
        0.33283350429856506,
        0.8893697722232838,
        0.8893697722232838,
        0.33283350429856506,
        0.03339172347815125,
        6.0000000000001025e-05
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackmanharris<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Blackman-Nuttall Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Blackman-Nuttall window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0003628000000000381,
        0.03777576895352033,
        0.34272761996881956,
        0.8918518610776603,
        0.8918518610776603,
        0.34272761996881956,
        0.03777576895352033
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackmannuttall<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Blackman-Nuttall window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0003628000000000381,
        0.025205566515401824,
        0.22698240000000006,
        0.7019582334845982,
        1.0,
        0.7019582334845982,
        0.22698240000000006,
        0.025205566515401824
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackmannuttall<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Blackman-Nuttall window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0003628000000000381,
        0.0613345,
        0.5292298,
        1.0,
        0.5292298000000003,
        0.06133450000000015,
        0.0003628000000000381
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackmannuttall<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Blackman-Nuttall window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0003628000000000381,
        0.03777576895352033,
        0.34272761996881956,
        0.8918518610776603,
        0.8918518610776603,
        0.34272761996881956,
        0.03777576895352033,
        0.0003628000000000381
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::blackmannuttall<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Flattop Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Flattop window impulse response of length 7") {
    const std::vector<float> reference = {
        -0.0004210510000000013,
        -0.03684078115492343,
        0.010703716716153475,
        0.78087391493877,
        0.78087391493877,
        0.010703716716153475,
        -0.03684078115492343
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::flattop<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Flattop window impulse response of length 8") {
    const std::vector<float> reference = {
        -0.0004210510000000013,
        -0.026872193286334545,
        -0.05473684,
        0.4441353572863345,
        1.000000003,
        0.4441353572863345,
        -0.05473684,
        -0.026872193286334545
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::flattop<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Flattop window impulse response of length 7") {
    const std::vector<float> reference = {
        -0.0004210510000000013,
        -0.05126315599999999,
        0.1982105299999999,
        1.000000003,
        0.1982105300000003,
        -0.05126315600000008,
        -0.0004210510000000013
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::flattop<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Flattop window impulse response of length 8") {
    const std::vector<float> reference = {
        -0.0004210510000000013,
        -0.03684078115492343,
        0.010703716716153475,
        0.78087391493877,
        0.78087391493877,
        0.010703716716153475,
        -0.03684078115492343,
        -0.0004210510000000013
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::flattop<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Bohman Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Bohman window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.07072474681109345,
        0.43748401216760524,
        0.9103685132461523,
        0.9103685132461525,
        0.4374840121676056,
        0.07072474681109353
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::bohman<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Bohman window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.048302383742639676,
        0.31830988618379075,
        0.7554091649291872,
        1.0,
        0.7554091649291872,
        0.31830988618379075,
        0.048302383742639676
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::bohman<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Bohman window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.10899778104422939,
        0.6089977810442293,
        1.0,
        0.6089977810442295,
        0.10899778104422961,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::bohman<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Bohman window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.07072474681109345,
        0.43748401216760524,
        0.9103685132461523,
        0.9103685132461525,
        0.4374840121676056,
        0.07072474681109353,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::bohman<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Cosine Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Cosine window impulse response of length 7") {
    const std::vector<float> reference = {
        0.19509032201612825,
        0.5555702330196022,
        0.8314696123025451,
        0.9807852804032304,
        0.9807852804032304,
        0.8314696123025455,
        0.5555702330196022
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::cosine<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Cosine window impulse response of length 8") {
    const std::vector<float> reference = {
        0.17364817766693033,
        0.49999999999999994,
        0.766044443118978,
        0.9396926207859083,
        1.0,
        0.9396926207859084,
        0.7660444431189781,
        0.49999999999999994
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::cosine<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Cosine window impulse response of length 7") {
    const std::vector<float> reference = {
        0.2225209339563144,
        0.6234898018587335,
        0.9009688679024191,
        1.0,
        0.9009688679024191,
        0.6234898018587336,
        0.2225209339563145
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::cosine<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Cosine window impulse response of length 8") {
    const std::vector<float> reference = {
        0.19509032201612825,
        0.5555702330196022,
        0.8314696123025451,
        0.9807852804032304,
        0.9807852804032304,
        0.8314696123025455,
        0.5555702330196022,
        0.19509032201612858
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::cosine<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Parzen Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Parzen window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.0466472303206997,
        0.3702623906705537,
        0.8950437317784256,
        0.8950437317784257,
        0.3702623906705537,
        0.046647230320699756
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::parzen<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Parzen window impulse response of length 8") {
    const std::vector<float> reference = {0.0, 0.03125, 0.25, 0.71875, 1.0, 0.71875, 0.25, 0.03125};
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::parzen<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Parzen window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.07407407407407403,
        0.5555555555555556,
        1.0,
        0.5555555555555557,
        0.07407407407407403,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::parzen<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Parzen window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.0466472303206997,
        0.3702623906705537,
        0.8950437317784256,
        0.8950437317784257,
        0.3702623906705537,
        0.046647230320699756,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::parzen<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Lanczos Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Lanczos window impulse response of length 7") {
    const std::vector<float> reference = {
        3.898171832520617e-17,
        0.3484105662791794,
        0.7241014497831975,
        1.0,
        0.9667663853063981,
        0.7241014497821217,
        0.348410566278869
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::lanczos<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Lanczos window impulse response of length 8") {
    const std::vector<float> reference = {
        3.898171832520617e-17,
        0.3001054387191628,
        0.6366197723679867,
        0.9003163161582524,
        1.0,
        0.9003163161559598,
        0.636619772367176,
        0.300105438718908
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::lanczos<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Lanczos window impulse response of length 7") {
    const std::vector<float> reference = {
        3.898171832520617e-17,
        0.4134966715665414,
        0.8269933431334778,
        1.0,
        0.8269933431318984,
        0.4134966715661465,
        3.898171832518135e-17
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::lanczos<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Lanczos window impulse response of length 8") {
    const std::vector<float> reference = {
        3.898171832520617e-17,
        0.3484105662791794,
        0.7241014497831975,
        1.0,
        0.9667663853063981,
        0.7241014497821217,
        0.348410566278869,
        3.898171832518135e-17
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::lanczos<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Welch Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Welch window impulse response of length 7") {
    const std::vector<float> reference = {0.4375, 0.75, 0.9375, 1.0, 0.9375, 0.75, 0.4375};
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::welch<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Welch window impulse response of length 8") {
    const std::vector<float> reference = {
        0.3950617283950617,
        0.691358024691358,
        0.8888888888888888,
        0.9876543209876543,
        0.9876543209876543,
        0.8888888888888888,
        0.691358024691358,
        0.3950617283950617
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::welch<float>(i, reference.size(), false);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Welch window impulse response of length 7") {
    const std::vector<float> reference = {
        0.4897959183673469,
        0.8163265306122449,
        0.9795918367346939,
        0.9795918367346939,
        0.8163265306122449,
        0.4897959183673469,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::welch<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Welch window impulse response of length 8") {
    const std::vector<float> reference = {0.4375, 0.75, 0.9375, 1.0, 0.9375, 0.75, 0.4375, 0.0};
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::welch<float>(i, reference.size(), true);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: names
// ---------------------------------------------------------------------------

SCENARIO("window function names are indexed by their enum values") {
    const auto& names = Fourier::Window::names();
    REQUIRE(names.size() == 15);

    WHEN("the name of Fourier::Window::Function::Boxcar is queried") {
        THEN("\"Boxcar\" is returned") {
            REQUIRE("Boxcar" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Boxcar)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Bartlett is queried") {
        THEN("\"Bartlett\" is returned") {
            REQUIRE("Bartlett" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Bartlett)));
        }
    }
    WHEN("the name of Fourier::Window::Function::BartlettHann is queried") {
        THEN("\"BartlettHann\" is returned") {
            REQUIRE("BartlettHann" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::BartlettHann)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Parzen is queried") {
        THEN("\"Parzen\" is returned") {
            REQUIRE("Parzen" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Parzen)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Welch is queried") {
        THEN("\"Welch\" is returned") {
            REQUIRE("Welch" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Welch)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Cosine is queried") {
        THEN("\"Cosine\" is returned") {
            REQUIRE("Cosine" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Cosine)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Bohman is queried") {
        THEN("\"Bohman\" is returned") {
            REQUIRE("Bohman" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Bohman)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Lanczos is queried") {
        THEN("\"Lanczos\" is returned") {
            REQUIRE("Lanczos" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Lanczos)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Hann is queried") {
        THEN("\"Hann\" is returned") {
            REQUIRE("Hann" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Hann)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Hamming is queried") {
        THEN("\"Hamming\" is returned") {
            REQUIRE("Hamming" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Hamming)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Blackman is queried") {
        THEN("\"Blackman\" is returned") {
            REQUIRE("Blackman" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Blackman)));
        }
    }
    WHEN("the name of Fourier::Window::Function::BlackmanHarris is queried") {
        THEN("\"BlackmanHarris\" is returned") {
            REQUIRE("BlackmanHarris" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::BlackmanHarris)));
        }
    }
    WHEN("the name of Fourier::Window::Function::BlackmanNuttall is queried") {
        THEN("\"BlackmanNuttall\" is returned") {
            REQUIRE("BlackmanNuttall" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::BlackmanNuttall)));
        }
    }
    WHEN("the name of Fourier::Window::Function::KaiserBessel is queried") {
        THEN("\"KaiserBessel\" is returned") {
            REQUIRE("KaiserBessel" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::KaiserBessel)));
        }
    }
    WHEN("the name of Fourier::Window::Function::Flattop is queried") {
        THEN("\"Flattop\" is returned") {
            REQUIRE("Flattop" ==
                names.at(static_cast<std::size_t>(Fourier::Window::Function::Flattop)));
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: coherent_gain
// ---------------------------------------------------------------------------

SCENARIO("window functions need to be mapped to coherent gains at run-time") {
    WHEN("the coherent gain of Fourier::Window::Function::Boxcar is queried") {
        THEN("1.0 is returned") {
            REQUIRE(1.0f == Fourier::Window::coherent_gain(Fourier::Window::Function::Boxcar));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Bartlett is queried") {
        THEN("0.5 is returned") {
            REQUIRE(0.5f == Fourier::Window::coherent_gain(Fourier::Window::Function::Bartlett));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::BartlettHann is queried") {
        THEN("0.5 is returned") {
            REQUIRE(0.5f == Fourier::Window::coherent_gain(Fourier::Window::Function::BartlettHann));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Parzen is queried") {
        THEN("0.375 is returned") {
            REQUIRE(0.375f == Fourier::Window::coherent_gain(Fourier::Window::Function::Parzen));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Welch is queried") {
        THEN("0.667317 is returned") {
            REQUIRE(0.667317f == Fourier::Window::coherent_gain(Fourier::Window::Function::Welch));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Cosine is queried") {
        THEN("0.63724 is returned") {
            REQUIRE(0.63724f == Fourier::Window::coherent_gain(Fourier::Window::Function::Cosine));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Bohman is queried") {
        THEN("0.405285 is returned") {
            REQUIRE(0.405285f == Fourier::Window::coherent_gain(Fourier::Window::Function::Bohman));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Lanczos is queried") {
        THEN("0.58949 is returned") {
            REQUIRE(0.58949f == Fourier::Window::coherent_gain(Fourier::Window::Function::Lanczos));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Hann is queried") {
        THEN("0.5 is returned") {
            REQUIRE(0.5f == Fourier::Window::coherent_gain(Fourier::Window::Function::Hann));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Hamming is queried") {
        THEN("0.54 is returned") {
            REQUIRE(0.54f == Fourier::Window::coherent_gain(Fourier::Window::Function::Hamming));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Blackman is queried") {
        THEN("0.42 is returned") {
            REQUIRE(0.42f == Fourier::Window::coherent_gain(Fourier::Window::Function::Blackman));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::BlackmanHarris is queried") {
        THEN("0.35875 is returned") {
            REQUIRE(0.35875f == Fourier::Window::coherent_gain(Fourier::Window::Function::BlackmanHarris));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::BlackmanNuttall is queried") {
        THEN("0.363582 is returned") {
            REQUIRE(0.363582f == Fourier::Window::coherent_gain(Fourier::Window::Function::BlackmanNuttall));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::KaiserBessel is queried") {
        THEN("0.402 is returned") {
            REQUIRE(0.402f == Fourier::Window::coherent_gain(Fourier::Window::Function::KaiserBessel));
        }
    }
    WHEN("the coherent gain of Fourier::Window::Function::Flattop is queried") {
        THEN("0.215579 is returned") {
            REQUIRE(0.215579f == Fourier::Window::coherent_gain(Fourier::Window::Function::Flattop));
        }
    }
}

TEST_CASE("Window metadata preserves attenuation and length-dependent bandwidth") {
    using Function = Fourier::Window::Function;
    struct Reference {
        Function window;
        float side_lobe_db;
        float stopband_db;
        double bandwidth;
    };
    // Public window-selection metadata. Keep the dB values signed, and check
    // bandwidth at two lengths so an omitted division by N cannot pass.
    const Reference references[] = {
        {Function::Boxcar, -13.2f, -21.f, 0.9},
        {Function::Bartlett, -26.4f, -25.f, 1.8},
        {Function::BartlettHann, -35.7f, -42.f, 3.2},
        {Function::Parzen, -53.f, -31.f, 4.0},
        {Function::Welch, -21.2f, -31.f, 3.3},
        {Function::Cosine, -22.8f, -33.f, 3.1},
        {Function::Bohman, -46.f, -28.f, 3.3},
        {Function::Lanczos, -26.3f, -28.f, 3.3},
        {Function::Hann, -31.5f, -44.f, 3.1},
        {Function::Hamming, -41.7f, -53.f, 3.3},
        {Function::Blackman, -58.1f, -74.f, 5.5},
        {Function::BlackmanHarris, -91.8f, -92.f, 6.3},
        {Function::BlackmanNuttall, -88.7f, -93.f, 6.4},
        {Function::KaiserBessel, -65.4f, -60.f, 3.6},
        {Function::Flattop, -83.f, -99.f, 7.5},
    };
    for (const auto& reference : references) {
        CAPTURE(static_cast<int>(reference.window));
        CHECK(Fourier::Window::side_lobe_amplitude(reference.window)
            == Approx(reference.side_lobe_db));
        CHECK(Fourier::Window::stopband_attenuation(reference.window)
            == Approx(reference.stopband_db));
        for (double length : {64.0, 1024.0})
            CHECK(Fourier::Window::transition_width(reference.window, length)
                == Approx(reference.bandwidth / length));
    }
}

TEST_CASE("Window functions and metadata reject unknown window identifiers") {
    for (int value : {-1, 15}) {
        const auto window = static_cast<Fourier::Window::Function>(value);
        CAPTURE(value);
        CHECK_THROWS_AS(Fourier::Window::window(window, 0.0, 16.0), std::runtime_error);
        CHECK_THROWS_AS(Fourier::Window::coherent_gain(window), std::runtime_error);
        CHECK_THROWS_AS(Fourier::Window::side_lobe_amplitude(window), std::runtime_error);
        CHECK_THROWS_AS(Fourier::Window::stopband_attenuation(window), std::runtime_error);
        CHECK_THROWS_AS(Fourier::Window::transition_width(window, 16.0), std::runtime_error);
    }
}

// ---------------------------------------------------------------------------
// MARK: Exponential Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric exponential window impulse response of length 7") {
    const std::vector<float> reference = {
        0.6065306597126334,
        0.6996725373751304,
        0.8071177470053893,
        0.9310627797040227,
        0.9310627797040227,
        0.8071177470053893,
        0.6996725373751304
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::exponential<float>(i, reference.size(), false, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric exponential window impulse response of length 8") {
    const std::vector<float> reference = {
        0.6065306597126334,
        0.6872892787909722,
        0.7788007830714049,
        0.8824969025845955,
        1.0,
        0.8824969025845955,
        0.7788007830714049,
        0.6872892787909722
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::exponential<float>(i, reference.size(), false, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric exponential window impulse response of length 7") {
    const std::vector<float> reference = {
        0.6065306597126334,
        0.7165313105737893,
        0.8464817248906141,
        1.0,
        0.8464817248906141,
        0.7165313105737893,
        0.6065306597126334
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::exponential<float>(i, reference.size(), true, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric exponential window impulse response of length 8") {
    const std::vector<float> reference = {
        0.6065306597126334,
        0.6996725373751304,
        0.8071177470053893,
        0.9310627797040227,
        0.9310627797040227,
        0.8071177470053893,
        0.6996725373751304,
        0.6065306597126334
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::exponential<float>(i, reference.size(), true, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Hann-Poisson Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Hann-Poisson window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.13171692284055647,
        0.4933591709408724,
        0.8849606791400177,
        0.8849606791400177,
        0.49335917094087245,
        0.13171692284055656
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hannpoisson<float>(i, reference.size(), false, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
        // This window should be the product of a Hann and Poisson window.
        const auto equivalent = Fourier::Window::exponential<float>(i, reference.size(), false) * Fourier::Window::hann<float>(i, reference.size(), false);
        REQUIRE(approx_equal(equivalent, window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Hann-Poisson window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.10065118456053207,
        0.3894003915357024,
        0.7532582233891434,
        1.0,
        0.7532582233891435,
        0.38940039153570255,
        0.10065118456053211
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hannpoisson<float>(i, reference.size(), false, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
        // This window should be the product of a Hann and Poisson window.
        const auto equivalent = Fourier::Window::exponential<float>(i, reference.size(), false) * Fourier::Window::hann<float>(i, reference.size(), false);
        REQUIRE(approx_equal(equivalent, window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Hann-Poisson window impulse response of length 7") {
    const std::vector<float> reference = {
        0.0,
        0.1791328276434473,
        0.6348612936679605,
        1.0,
        0.6348612936679607,
        0.17913282764344732,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hannpoisson<float>(i, reference.size(), true, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
        // This window should be the product of a Hann and Poisson window.
        const auto equivalent = Fourier::Window::exponential<float>(i, reference.size(), true) * Fourier::Window::hann<float>(i, reference.size(), true);
        REQUIRE(approx_equal(equivalent, window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Hann-Poisson window impulse response of length 8") {
    const std::vector<float> reference = {
        0.0,
        0.13171692284055647,
        0.4933591709408724,
        0.8849606791400177,
        0.8849606791400177,
        0.49335917094087245,
        0.13171692284055656,
        0.0
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::hannpoisson<float>(i, reference.size(), true, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
        // This window should be the product of a Hann and Poisson window.
        const auto equivalent = Fourier::Window::exponential<float>(i, reference.size(), true) * Fourier::Window::hann<float>(i, reference.size(), true);
        REQUIRE(approx_equal(equivalent, window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Gaussian Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Gaussian window impulse response of length 7") {
    const std::vector<float> reference = {
        0.00033546262790251185,
        0.01687988414878991,
        0.23006629899380912,
        0.8493658165683124,
        0.8493658165683124,
        0.23006629899380912,
        0.01687988414878991
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::gaussian<float>(i, reference.size(), false, 0.25);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Gaussian window impulse response of length 8") {
    const std::vector<float> reference = {
        0.00033546262790251185,
        0.011108996538242306,
        0.1353352832366127,
        0.6065306597126334,
        1.0,
        0.6065306597126334,
        0.1353352832366127,
        0.011108996538242306
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::gaussian<float>(i, reference.size(), false, 0.25);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Gaussian window impulse response of length 7") {
    const std::vector<float> reference = {
        0.00033546262790251185,
        0.028565500784550377,
        0.41111229050718745,
        1.0,
        0.41111229050718745,
        0.028565500784550377,
        0.00033546262790251185
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::gaussian<float>(i, reference.size(), true, 0.25);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Gaussian window impulse response of length 8") {
    const std::vector<float> reference = {
        0.00033546262790251185,
        0.01687988414878991,
        0.23006629899380912,
        0.8493658165683124,
        0.8493658165683124,
        0.23006629899380912,
        0.01687988414878991,
        0.00033546262790251185
    };
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::gaussian<float>(i, reference.size(), true, 0.25);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: Tukey Window
// ---------------------------------------------------------------------------

TEST_CASE("Calculate an asymmetric Tukey window impulse response of length 7") {
    const std::vector<float> reference = {0.0, 0.6112604669781572, 1.0, 1.0, 1.0, 1.0, 0.6112604669781575};
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::tukey<float>(i, reference.size(), false, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate an asymmetric Tukey window impulse response of length 8") {
    const std::vector<float> reference = {0.0, 0.5, 1.0, 1.0, 1.0, 1.0, 1.0, 0.5};
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::tukey<float>(i, reference.size(), false, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Tukey window impulse response of length 7") {
    const std::vector<float> reference = {0.0, 0.75, 1.0, 1.0, 1.0, 0.7499999999999999, 0.0};
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::tukey<float>(i, reference.size(), true, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

TEST_CASE("Calculate a symmetric Tukey window impulse response of length 8") {
    const std::vector<float> reference = {0.0, 0.6112604669781572, 1.0, 1.0, 1.0, 1.0, 0.6112604669781575, 0.0};
    for (unsigned i = 0; i < reference.size(); i++) {
        const auto window = Fourier::Window::tukey<float>(i, reference.size(), true, 0.5);
        REQUIRE(approx_equal(reference[i], window, 1e-6f));
    }
}

// ---------------------------------------------------------------------------
// MARK: CachedWindow
// ---------------------------------------------------------------------------

SCENARIO("Cached windows need to be represented") {
    GIVEN("Parameters for an asymmetric Bartlett window of length 7") {
        const auto function = Fourier::Window::Function::Bartlett;
        const size_t N = 7;
        const bool is_symmetric = false;
        WHEN("The CachedWindow is instantiated, the samples matched") {
            Fourier::Window::CachedWindow<float> cached_window{function, N, is_symmetric};
            THEN("The correct function flag is set") {
                REQUIRE(function == cached_window.get_function());
            }
            THEN("The length of the cached window is correct") {
                REQUIRE(N == cached_window.get_samples().size());
            }
            THEN("The correct symmetry flag is set") {
                REQUIRE(is_symmetric == cached_window.get_is_symmetric());
            }
            THEN("The samples match the window function call outputs") {
                for (size_t n = 0; n < N; n++)
                    REQUIRE(cached_window[n] == Fourier::Window::window<float>(function, n, N, is_symmetric));
            }
        }
    }
    GIVEN("Parameters for an asymmetric Bartlett window of length 8") {
        const auto function = Fourier::Window::Function::Bartlett;
        const size_t N = 8;
        const bool is_symmetric = false;
        WHEN("The CachedWindow is instantiated, the samples matched") {
            Fourier::Window::CachedWindow<float> cached_window{function, N, is_symmetric};
            THEN("The correct function flag is set") {
                REQUIRE(function == cached_window.get_function());
            }
            THEN("The length of the cached window is correct") {
                REQUIRE(N == cached_window.get_samples().size());
            }
            THEN("The correct symmetry flag is set") {
                REQUIRE(is_symmetric == cached_window.get_is_symmetric());
            }
            THEN("The samples match the window function call outputs") {
                for (size_t n = 0; n < N; n++)
                    REQUIRE(cached_window[n] == Fourier::Window::window<float>(function, n, N, is_symmetric));
            }
        }
    }
    GIVEN("Parameters for a symmetric Bartlett window of length 7") {
        const auto function = Fourier::Window::Function::Bartlett;
        const size_t N = 7;
        const bool is_symmetric = true;
        WHEN("The CachedWindow is instantiated, the samples matched") {
            Fourier::Window::CachedWindow<float> cached_window{function, N, is_symmetric};
            THEN("The correct function flag is set") {
                REQUIRE(function == cached_window.get_function());
            }
            THEN("The length of the cached window is correct") {
                REQUIRE(N == cached_window.get_samples().size());
            }
            THEN("The correct symmetry flag is set") {
                REQUIRE(is_symmetric == cached_window.get_is_symmetric());
            }
            THEN("The samples match the window function call outputs") {
                for (size_t n = 0; n < N; n++)
                    REQUIRE(cached_window[n] == Fourier::Window::window<float>(function, n, N, is_symmetric));
            }
        }
    }
    GIVEN("Parameters for a symmetric Bartlett window of length 8") {
        const auto function = Fourier::Window::Function::Bartlett;
        const size_t N = 8;
        const bool is_symmetric = true;
        WHEN("The CachedWindow is instantiated, the samples matched") {
            Fourier::Window::CachedWindow<float> cached_window{function, N, is_symmetric};
            THEN("The correct function flag is set") {
                REQUIRE(function == cached_window.get_function());
            }
            THEN("The length of the cached window is correct") {
                REQUIRE(N == cached_window.get_samples().size());
            }
            THEN("The correct symmetry flag is set") {
                REQUIRE(is_symmetric == cached_window.get_is_symmetric());
            }
            THEN("The samples match the window function call outputs") {
                for (size_t n = 0; n < N; n++)
                    REQUIRE(cached_window[n] == Fourier::Window::window<float>(function, n, N, is_symmetric));
            }
        }
    }
}

SCENARIO("Cached windows need to be updated on-the-fly") {
    GIVEN("An existing window function") {
        const auto function = Fourier::Window::Function::Bartlett;
        const size_t N = 7;
        const bool is_symmetric = false;
        Fourier::Window::CachedWindow<float> cached_window{function, N, is_symmetric};
        WHEN("The cached window is set with the same parameters") {
            cached_window.set_window(function, N, is_symmetric);
            THEN("The correct function flag is set") {
                REQUIRE(function == cached_window.get_function());
            }
            THEN("The length of the cached window is correct") {
                REQUIRE(N == cached_window.get_samples().size());
            }
            THEN("The correct symmetry flag is set") {
                REQUIRE(is_symmetric == cached_window.get_is_symmetric());
            }
            THEN("The samples match the window function call outputs") {
                for (size_t n = 0; n < N; n++)
                    REQUIRE(cached_window[n] == Fourier::Window::window<float>(function, n, N, is_symmetric));
            }
        }
        WHEN("The cached window is set with a new length") {
            cached_window.set_window(function, N + 1, is_symmetric);
            THEN("The correct function flag is set") {
                REQUIRE(function == cached_window.get_function());
            }
            THEN("The length of the cached window is correct") {
                REQUIRE(N + 1 == cached_window.get_samples().size());
            }
            THEN("The correct symmetry flag is set") {
                REQUIRE(is_symmetric == cached_window.get_is_symmetric());
            }
            THEN("The samples match the window function call outputs") {
                for (size_t n = 0; n < N + 1; n++)
                    REQUIRE(cached_window[n] == Fourier::Window::window<float>(function, n, N + 1, is_symmetric));
            }
        }
        WHEN("The cached window is set with a new window function") {
            cached_window.set_window(Fourier::Window::Function::Hann, N, is_symmetric);
            THEN("The correct function flag is set") {
                REQUIRE(Fourier::Window::Function::Hann == cached_window.get_function());
            }
            THEN("The length of the cached window is correct") {
                REQUIRE(N == cached_window.get_samples().size());
            }
            THEN("The correct symmetry flag is set") {
                REQUIRE(is_symmetric == cached_window.get_is_symmetric());
            }
            THEN("The samples match the window function call outputs") {
                for (size_t n = 0; n < N; n++)
                    REQUIRE(cached_window[n] == Fourier::Window::window<float>(Fourier::Window::Function::Hann, n, N, is_symmetric));
            }
        }
        WHEN("The cached window is set with a new asymmetry flag") {
            cached_window.set_window(function, N, !is_symmetric);
            THEN("The correct function flag is set") {
                REQUIRE(function == cached_window.get_function());
            }
            THEN("The length of the cached window is correct") {
                REQUIRE(N == cached_window.get_samples().size());
            }
            THEN("The correct symmetry flag is set") {
                REQUIRE(!is_symmetric == cached_window.get_is_symmetric());
            }
            THEN("The samples match the window function call outputs") {
                for (size_t n = 0; n < N; n++)
                    REQUIRE(cached_window[n] == Fourier::Window::window<float>(function, n, N, !is_symmetric));
            }
        }
    }
}
