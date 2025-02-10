// A structure for calculating a moving average.
// Copyright 2020 Christian Kauten
//
// Author: Christian Kauten (kautenja@auburn.edu)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef DSP_FILTER_MOVING_AVERAGE_HPP
#define DSP_FILTER_MOVING_AVERAGE_HPP

#include "../math/circular_buffer.hpp"

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief A simple IIR moving average filter.
struct MovingAverageIIR {
 private:
    /// the buffer to store samples in for the moving average
    Math::CircularBuffer<float> buffer;
    /// the feedback path
    float feedback = 0.f;

 public:
    /// @brief Initialize a new moving average filter.
    ///
    /// @param samples the number of samples to use for calculating the average
    ///
    explicit MovingAverageIIR(std::size_t samples = 64) {
        buffer.resize(samples);
    }

    /// @brief Reset the moving average filter to its default state.
    void reset() {
        buffer.clear();
        feedback = 0.f;
    }

    /// @brief Set the delay time on the input delay line.
    ///
    /// @param time the delay time for the input signal in seconds
    /// @param sample_rate the sample rate of the encompassing process
    ///
    inline void set_time(const float& time, const float& sample_rate) {
        // Calculate the size based on the look-ahead time and sample rate. Add
        // 1 to ensure the minimum is 1 and that the 1 sample delay is accounted
        // for, i.e., from inserting the input before querying the delay line
        // in the process function.
        const unsigned num_samples = time * sample_rate + 1.f;
        // Return if the size has not changed.
        if (buffer.size() == num_samples) return;
        // Clear the delay line and update the size.
        buffer.resize(num_samples);
        reset();
    }

    /// @brief Process the input value.
    ///
    /// @param value the value to process with the filter
    /// @returns the current value of the moving average
    ///
    float process(float value) {
        const float delay_output = buffer.at(1);
        buffer.insert(value);
        value -= delay_output;
        value += feedback;
        feedback = value;
        value *= 1.f / static_cast<float>(buffer.size());
        return value;
    }
};

/// @brief A simple FIR moving average filter.
struct MovingAverageFIR {
 private:
    /// the buffer to store samples in for the moving average
    Math::CircularBuffer<float> buffer;
    /// the moving average
    float average = 0;

 public:
    /// @brief Initialize a new moving average filter.
    ///
    /// @param samples the number of samples to use for calculating the average
    ///
    explicit MovingAverageFIR(std::size_t samples = 64) {
        buffer.resize(samples);
    }

    /// @brief Reset the moving average filter to its default state.
    void reset() {
        buffer.clear();
        average = 0;
    }

    /// @brief Set the delay time on the input delay line.
    ///
    /// @param time the delay time for the input signal in seconds
    /// @param sample_rate the sample rate of the encompassing process
    ///
    inline void set_time(const float& time, const float& sample_rate) {
        // Calculate the size based on the look-ahead time and sample rate. Add
        // 1 to ensure the minimum is 1 and that the 1 sample delay is accounted
        // for, i.e., from inserting the input before querying the delay line
        // in the process function.
        const unsigned num_samples = time * sample_rate + 1.f;
        // Return if the size has not changed.
        if (buffer.size() == num_samples) return;
        // Clear the delay line and update the size.
        buffer.resize(num_samples);
        reset();
    }

    /// @brief Recalculate the moving average from the delay line.
    /// @details
    /// This should be called occasionally in real-time environments to
    /// prevent numerical instability propagating through the moving average.
    void recalculate() {
        average = 0.f;
        for (unsigned i = 0; i < buffer.size(); i++) average += buffer.at(i);
    }

    /// @brief Process the input value.
    ///
    /// @param value the value to process with the filter
    /// @returns the current value of the moving average
    ///
    float process(float value) {
        // Remove the oldest sample from the moving average.
        average -= buffer.at(1);
        // Divide the incoming sample by the total number of samples to pre-
        // average the value and insert into the delay line.
        value /= static_cast<float>(buffer.size());
        buffer.insert(value);
        // Update the moving average by adding the current value.
        average += value;
        return average;
    }
};

}  // namespace Filter

#endif  // DSP_FILTER_MOVING_AVERAGE_HPP
