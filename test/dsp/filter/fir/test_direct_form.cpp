// Test cases for Filter::FIR::DirectForm structure.
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

#include <algorithm>
#include "dsp/filter/fir/direct_form.hpp"
#include "dsp/math.hpp"
#include "dsp/io.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("initial values should be null") {
    auto filter = Filter::FIR::DirectForm<float>();
    REQUIRE(1 == filter.size());
    REQUIRE(0.f == filter.coefficient(0));
}

TEST_CASE("initial values should be null when initializing with valid size") {
    auto filter = Filter::FIR::DirectForm<float>(10);
    REQUIRE(10 == filter.size());
    for (unsigned i = 0; i < 10; i++)
        REQUIRE(0.f == filter.coefficient(i));
}

TEST_CASE("initializer should throw for invalid size") {
    REQUIRE_THROWS(Filter::FIR::DirectForm<float>(0));
}

TEST_CASE("resize should throw for invalid size") {
    auto filter = Filter::FIR::DirectForm<float>();
    REQUIRE_THROWS(filter.resize(0));
}

SCENARIO("the filter needs to be resized") {
    GIVEN("a filter in default state") {
        auto filter = Filter::FIR::DirectForm<float>();
        WHEN("the filter is resized to 10 coefficients") {
            filter.resize(10);
            THEN("the size is updated") {
                REQUIRE(10 == filter.size());
            }
            THEN("the coefficients are 0") {
                for (unsigned i = 0; i < 10; i++)
                    REQUIRE(0.f == filter.coefficient(i));
            }
        }
    }
    GIVEN("a filter with 10 coefficients") {
        auto filter = Filter::FIR::DirectForm<int>();
        filter.resize(10);
        for (unsigned i = 0; i < 10; i++) filter.coefficient(i) = i;
        WHEN("the filter is resized to 5 coefficients") {
            filter.resize(5);
            THEN("the size is updated") {
                REQUIRE(5 == filter.size());
            }
            THEN("the coefficients are correct") {
                for (int i = 0; i < 5; i++)
                    REQUIRE(i == filter.coefficient(i));
            }
        }
        WHEN("the filter is resized to 5 coefficients then back to 10") {
            filter.resize(5);
            filter.resize(10);
            THEN("the size is updated") {
                REQUIRE(10 == filter.size());
            }
            THEN("the coefficients are correct") {
                for (int i = 0; i < 5; i++)
                    REQUIRE(i == filter.coefficient(i));
                for (int i = 5; i < 10; i++)
                    REQUIRE(0 == filter.coefficient(i));
            }
        }
    }
}

TEST_CASE("coefficient should set and return") {
    auto filter = Filter::FIR::DirectForm<float>();
    filter.coefficient(0) = 1.f;
    REQUIRE(1.f == filter.coefficient(0));
}

SCENARIO("values are filtered") {
    GIVEN("a filter with a single coefficient") {
        auto filter = Filter::FIR::DirectForm<float>();
        WHEN("an arbitrary value (1) is processed by the filter with 0th coefficient of 0") {
            THEN("then the output is 0") {
                for (unsigned i = 0; i < 10; i++)
                    REQUIRE(0.f == filter.process(1.f));
            }
        }
        WHEN("an arbitrary value (1) is processed by the filter with 0th coefficient of 1") {
            filter.coefficient(0) = 1.f;
            THEN("then the output is 1") {
                for (unsigned i = 0; i < 10; i++)
                    REQUIRE(1.f == filter.process(1.f));
            }
        }
    }
    GIVEN("a filter with two coefficients") {
        auto filter = Filter::FIR::DirectForm<float>();
        filter.resize(2);
        WHEN("an arbitrary value (1) is processed by the filter with 0th and 1th coefficient of 0") {
            THEN("then the output is 0") {
                for (unsigned i = 0; i < 10; i++)
                    REQUIRE(0.f == filter.process(1.f));
            }
        }
        WHEN("an arbitrary value (1) is processed by the filter with 0th coefficient of 1 and 1th coefficient of 0") {
            filter.coefficient(0) = 1.f;
            THEN("then the output is 1") {
                for (unsigned i = 0; i < 10; i++)
                    REQUIRE(1.f == filter.process(1.f));
            }
        }
        WHEN("an arbitrary value (1) is processed by the filter with 0th coefficient of 0 and 1th coefficient of 1") {
            filter.coefficient(1) = 1.f;
            THEN("then the output is 0 then 1") {
                REQUIRE(0.f == filter.process(1.f));
                for (unsigned i = 0; i < 10; i++)
                    REQUIRE(1.f == filter.process(1.f));
            }
        }
        WHEN("an arbitrary value (1) is processed by the filter with 0th and 1th coefficient of 1") {
            filter.coefficient(0) = 1.f;
            filter.coefficient(1) = 1.f;
            THEN("then the output is 1 then 2") {
                REQUIRE(1.f == filter.process(1.f));
                for (unsigned i = 0; i < 10; i++)
                    REQUIRE(2.f == filter.process(1.f));
            }
        }
        WHEN("an arbitrary value (1) is processed by the filter with 0th coefficient of 1 and 1th coefficient of -1") {
            filter.coefficient(0) = 1.f;
            filter.coefficient(1) = -1.f;
            THEN("then the output is 1 then 0") {
                REQUIRE(1.f == filter.process(1.f));
                for (unsigned i = 0; i < 10; i++)
                    REQUIRE(0.f == filter.process(1.f));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: setCoefficients
// ---------------------------------------------------------------------------

/// @brief Filter a waveform given a filter.
///
/// @tparam the data type for representing samples
/// @param wave the wave to filter
/// @param filter the filter to apply to the waveform
/// @returns the result of filtering the input waveform
/// @details
/// This will introduce a delay equal to the order of the filter in order to
/// fully populate the filter's delay line with waveform data.
///
template<typename T>
std::vector<T> filterWave(const std::vector<T>& wave, Filter::FIR::DirectForm<T>& filter) {
    // Create an output vector to store the filtered samples in
    std::vector<T> output;
    output.resize(wave.size());
    for (unsigned i = 0; i < wave.size(); i++)  // Iterate over the samples
        output[i] = filter.process(wave[i]);
    // Remove the filter N samples where the filter delay line is populating
    output.erase(output.begin(), output.begin() + filter.size());
    return output;
}

SCENARIO("lowpass coefficients are calculated") {
    static constexpr float sample_rate = 44100;
    static constexpr float nyquist_rate = sample_rate / 2;
    static constexpr int num_samples = sample_rate;
    static constexpr float frequency = 0.5;
    // Calculate the initial waveform
    static const auto wave = Math::generate_sinusoid(frequency * nyquist_rate, sample_rate, num_samples);
    // IO::writeWAV("raw.wav", wave, sample_rate, 1);
    REQUIRE(Math::IEEE754::approx_equal<float>(Math::maxRMS<float>(), Math::get_rms(wave), 1e-3));
    REQUIRE(Math::IEEE754::epsilon_equal<float>(0, Math::get_peak(wave)));

    GIVEN("an odd filter with a Hamming window") {
        Filter::FIR::DirectForm<float> filter(255);
        WHEN("The cutoff frequency is set to 0.0") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.0);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The sinusoid drops below measurable decibel levels") {
                const auto peak = Math::amplitude2decibels(std::numeric_limits<float>::epsilon());
                REQUIRE(peak == Math::get_peak(output));
                const auto rms = Math::amplitude2decibels(std::numeric_limits<float>::epsilon())/2;
                REQUIRE(rms == Math::get_rms(output));
            }
        }
        WHEN("The cutoff frequency is set to 0.25") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.25);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The sinusoid drops below audible decibel levels") {
                REQUIRE(Math::get_peak(output) < -40);
                REQUIRE(Math::get_rms(output) < -60);
            }
        }
        WHEN("The cutoff frequency is set to 0.4") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.4);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The sinusoid drops below audible decibel levels") {
                REQUIRE(Math::get_peak(output) < -40);
                REQUIRE(Math::get_rms(output) < -60);
            }
        }
        WHEN("The cutoff frequency is set to 0.5") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.5);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The sinusoid drops below -3dB") {
                REQUIRE(Math::get_peak(output) < -3);
                REQUIRE(Math::get_rms(output) < -3);
            }
        }
        WHEN("The cutoff frequency is set to 0.6") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.6);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The RMS of the sinusoid is not affected") {
                REQUIRE(Math::IEEE754::approx_equal<float>(Math::maxRMS<float>(), Math::get_rms(output), 1e-2));
            }
        }
        WHEN("The cutoff frequency is set to 0.75") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.75);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The RMS of the sinusoid is not affected") {
                REQUIRE(Math::IEEE754::approx_equal<float>(Math::maxRMS<float>(), Math::get_rms(output), 1e-2));
            }
        }
        WHEN("The cutoff frequency is set to 1.0") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 1.0);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The RMS of the sinusoid is not affected") {
                REQUIRE(Math::IEEE754::approx_equal<float>(Math::maxRMS<float>(), Math::get_rms(output), 1e-2));
            }
        }
    }
    GIVEN("an even filter with a Hamming window") {
        Filter::FIR::DirectForm<float> filter(256);
        WHEN("The cutoff frequency is set to 0.0") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.0);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The sinusoid drops below measurable decibel levels") {
                const auto peak = Math::amplitude2decibels(std::numeric_limits<float>::epsilon());
                REQUIRE(peak == Math::get_peak(output));
                const auto rms = Math::amplitude2decibels(std::numeric_limits<float>::epsilon())/2;
                REQUIRE(rms == Math::get_rms(output));
            }
        }
        WHEN("The cutoff frequency is set to 0.25") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.25);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The sinusoid drops below audible decibel levels") {
                REQUIRE(Math::get_peak(output) < -40);
                REQUIRE(Math::get_rms(output) < -60);
            }
        }
        WHEN("The cutoff frequency is set to 0.4") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.4);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The sinusoid drops below audible decibel levels") {
                REQUIRE(Math::get_peak(output) < -40);
                REQUIRE(Math::get_rms(output) < -60);
            }
        }
        WHEN("The cutoff frequency is set to 0.5") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.5);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The sinusoid drops below -3dB") {
                REQUIRE(Math::get_peak(output) < -3);
                REQUIRE(Math::get_rms(output) < -3);
            }
        }
        WHEN("The cutoff frequency is set to 0.6") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.6);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The RMS of the sinusoid is not affected") {
                REQUIRE(Math::IEEE754::approx_equal<float>(Math::maxRMS<float>(), Math::get_rms(output), 1e-2));
            }
        }
        WHEN("The cutoff frequency is set to 0.75") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 0.75);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The RMS of the sinusoid is not affected") {
                REQUIRE(Math::IEEE754::approx_equal<float>(Math::maxRMS<float>(), Math::get_rms(output), 1e-2));
            }
        }
        WHEN("The cutoff frequency is set to 1.0") {
            filter.setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::hamming<float>, 1.0);
            auto output = filterWave(wave, filter);
            // IO::writeWAV("filtered.wav", output, sample_rate, 1);
            THEN("The RMS of the sinusoid is not affected") {
                REQUIRE(Math::IEEE754::approx_equal<float>(Math::maxRMS<float>(), Math::get_rms(output), 1e-2));
            }
        }
    }
}
