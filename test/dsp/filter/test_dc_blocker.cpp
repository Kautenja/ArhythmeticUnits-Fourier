// Test cases for Filter::DCBlocker structure.
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

#include <vector>
#include "dsp/math.hpp"
#include "dsp/filter/dc_blocker.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

template<typename T>
inline std::vector<T> operator+(const std::vector<T>& input, const T& scalar) {
    std::vector<T> output(input.size());
    std::transform(input.begin(), input.end(), output.begin(), [&scalar](const float& value){
        return value + scalar;
    });
    return output;
}

template<typename T>
inline std::vector<T> operator+(const T& scalar, const std::vector<T>& input) {
    std::vector<T> output(input.size());
    std::transform(input.begin(), input.end(), output.begin(), [&scalar](const float& value){
        return value + scalar;
    });
    return output;
}

template<typename T>
inline std::vector<T> operator-(const std::vector<T>& input, const T& scalar) {
    std::vector<T> output(input.size());
    std::transform(input.begin(), input.end(), output.begin(), [&scalar](const float& value){
        return value - scalar;
    });
    return output;
}

template<typename T>
inline std::vector<T> operator-(const T& scalar, const std::vector<T>& input) {
    std::vector<T> output(input.size());
    std::transform(input.begin(), input.end(), output.begin(), [&scalar](const float& value){
        return value - scalar;
    });
    return output;
}

template<typename T>
inline std::vector<T> operator*(const std::vector<T>& input, const T& scalar) {
    std::vector<T> output(input.size());
    std::transform(input.begin(), input.end(), output.begin(), [&scalar](const float& value){
        return value * scalar;
    });
    return output;
}

template<typename T>
inline std::vector<T> operator*(const T& scalar, const std::vector<T>& input) {
    std::vector<T> output(input.size());
    std::transform(input.begin(), input.end(), output.begin(), [&scalar](const float& value){
        return value * scalar;
    });
    return output;
}

template<typename T>
inline std::vector<T> operator/(const std::vector<T>& input, const T& scalar) {
    std::vector<T> output(input.size());
    std::transform(input.begin(), input.end(), output.begin(), [&scalar](const float& value){
        return value / scalar;
    });
    return output;
}

template<typename T>
inline std::vector<T> operator/(const T& scalar, const std::vector<T>& input) {
    std::vector<T> output(input.size());
    std::transform(input.begin(), input.end(), output.begin(), [&scalar](const float& value){
        return scalar / value;
    });
    return output;
}

template<typename T>
inline T mean(const std::vector<T>& elements) {
    T sum = 0;
    for (const auto& element : elements)
        sum += element;
    sum /= elements.size();
    return sum;
}

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
std::vector<T> applyFilter(const std::vector<T>& wave, Filter::DCBlocker<T>& filter) {
    // Create an output vector to store the filtered samples in
    std::vector<T> output;
    output.resize(wave.size());
    for (unsigned i = 0; i < wave.size(); i++)  // Iterate over the samples
        output[i] = filter.process(wave[i]);
    // Remove the filter N samples where the filter delay line is populating
    // output.erase(output.begin(), output.begin() + filter.size());
    return output;
}

// ---------------------------------------------------------------------------
// MARK: Filter::DCBlocker<float>
// ---------------------------------------------------------------------------

SCENARIO("The DC offset needs to be removed from a sinusoid that has no DC offset") {
    GIVEN("a DC Blocking filter with narrow transition band") {
        Filter::DCBlocker<float> filter;
        WHEN("a sinusoid at 0.1pi is filtered") {
            const auto angular_frequency = 0.1f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate);
            const auto output = applyFilter(input, filter);
            THEN("the signal is not effected") {
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.2pi is filtered") {
            const auto angular_frequency = 0.2f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate);
            const auto output = applyFilter(input, filter);
            THEN("the signal is not effected") {
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.5pi is filtered") {
            const auto angular_frequency = 0.5f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate);
            const auto output = applyFilter(input, filter);
            THEN("the signal is not effected") {
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.8pi is filtered") {
            const auto angular_frequency = 0.8f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate);
            const auto output = applyFilter(input, filter);
            THEN("the signal is not effected") {
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 1pi is filtered") {
            const auto angular_frequency = 1.f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate);
            const auto output = applyFilter(input, filter);
            THEN("the signal is not effected") {
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                REQUIRE(Math::IEEE754::approx_equal(-50.3f, Math::get_rms(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-50.3f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                REQUIRE(Math::IEEE754::approx_equal(-36.6f, Math::get_peak(input), 1e-1f));
                REQUIRE(Math::IEEE754::approx_equal(-36.6f, Math::get_peak(output), 1e-1f));
            }
        }
    }
}

SCENARIO("The DC offset needs to be removed from a sinusoid that has a negative DC offset") {
    GIVEN("a DC Blocking filter with narrow transition band") {
        Filter::DCBlocker<float> filter;
        WHEN("a sinusoid at 0.1pi is filtered") {
            const auto angular_frequency = 0.1f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid<float>(frequency, sample_rate, sample_rate) - 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(-0.999f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(-0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.2pi is filtered") {
            const auto angular_frequency = 0.2f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) - 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(-0.999f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(-0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.5pi is filtered") {
            const auto angular_frequency = 0.5f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) - 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(-0.999f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(-0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.8pi is filtered") {
            const auto angular_frequency = 0.8f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) - 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(-0.999f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(-0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 1pi is filtered") {
            const auto angular_frequency = 1.f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) - 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(-1.f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(-0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-50.3f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-50.3f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-36.6f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-36.6f, Math::get_peak(output), 1e-1f));
            }
        }
    }
}

SCENARIO("The DC offset needs to be removed from a sinusoid that has a positive DC offset") {
    GIVEN("a DC Blocking filter with narrow transition band") {
        Filter::DCBlocker<float> filter;
        WHEN("a sinusoid at 0.1pi is filtered") {
            const auto angular_frequency = 0.1f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) + 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(1.f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.2pi is filtered") {
            const auto angular_frequency = 0.2f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) + 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(1.f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.5pi is filtered") {
            const auto angular_frequency = 0.5f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) + 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(1.f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-0.f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 0.8pi is filtered") {
            const auto angular_frequency = 0.8f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) + 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(1.f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-3.0f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-0.4f, Math::get_peak(output), 1e-1f));
            }
        }
        WHEN("a sinusoid at 1pi is filtered") {
            const auto angular_frequency = 1.f;
            const auto sample_rate = 44100.f;
            const auto frequency = angular_frequency * sample_rate / 2.f;
            const auto input = Math::generate_sinusoid(frequency, sample_rate, sample_rate) + 1.f;
            const auto output = applyFilter(input, filter);
            THEN("the output signal has no DC offset") {
                // std::cout << Math::get_rms(input) << " " << Math::get_rms(output) << std::endl;
                // std::cout << Math::get_peak(input) << " " << Math::get_peak(output) << std::endl;
                // check the DC coefficient
                REQUIRE(Math::IEEE754::approx_equal(1.f, mean(input), 1e-3f));
                REQUIRE(Math::IEEE754::approx_equal(0.0f, mean(output), 1e-1f));
                // check the RMS levels
                // REQUIRE(Math::IEEE754::approx_equal(-50.3f, Math::get_rms(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-50.3f, Math::get_rms(output), 1e-1f));
                // check the peak levels
                // REQUIRE(Math::IEEE754::approx_equal(-36.6f, Math::get_peak(input), 1e-1f));
                // REQUIRE(Math::IEEE754::approx_equal(-36.6f, Math::get_peak(output), 1e-1f));
            }
        }
    }
}
