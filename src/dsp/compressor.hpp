// A simple compressor.
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

#ifndef DSP_COMPRESSOR
#define DSP_COMPRESSOR

#include <algorithm>
#include "function_generator/integrator.hpp"
#include "filter/root_mean_square.hpp"

namespace DSP {

/// @brief A simple compressor.
/// @details
///
/// | is_expander | is_upward | Mode       | Description                       |
/// |:------------|:----------|:-----------|:----------------------------------|
/// | false       | false     | Compressor | sounds louder than threshold are reduced.
/// | false       | true      | Upwards    | sounds louder than threshold are increased.
/// | true        | false     | Expander   | sounds quieter than threshold are reduced.
/// | true        | true      | Sausage    | sounds quieter than the threshold are increased.
///
struct Compressor {
 private:
    /// the meter for calculating the RMS of the input signal
    Filter::RootMeanSquare meter;
    /// the function generator for responding to amplitude events
    FunctionGenerator::Integrator<float> integrator;
    /// the look-ahead delay line
    Math::CircularBuffer<float> look_ahead;
    /// the ratio of amplitude to amplitude past the threshold
    float ratio = 1.f;
    /// the threshold for generating amplitude events
    float threshold = 1.f;
    /// the amount of makeup gain
    float makeup = 1.f;
    /// the output of the envelope
    float envelope = 0.f;
    /// Whether the compress is an expander. When false, the compressor will
    /// reduce the level of signals that are louder than the threshold. When
    /// true the compressor will reduce the level of signals that are quieter
    /// than the threshold.
    bool is_expander = false;
    /// Whether the compressor is an upward expander. When false, the
    /// compressor will reduce the level of signals that are louder than the
    /// threshold. When true, the compressor will increase the level of signals
    /// that are louder than the threshold.
    bool is_upward = false;
    /// Whether the knee is soft (true) or hard (false.
    bool is_soft_knee = false;
    /// Whether to meter the peak meter instead of RMS
    bool is_peak_metering = false;

 public:
    /// Initialize a new compressor.
    Compressor() {
        integrator.setRiseShape(0.5);
        integrator.setFallShape(0.5);
    }

    /// @brief Set the sample rate to resize the RMS meter delay line.
    ///
    /// @param time the delay line length for the RMS meter in seconds
    /// @param sample_rate the sample rate of the encompassing process
    ///
    inline void set_meter_time(const float& time, const float& sample_rate) {
        meter.set_time(time, sample_rate);
    }

    /// @brief Reset the internal state of the compressor.
    inline void reset() {
        meter.reset();
        integrator.reset();
        look_ahead.clear();
    }

    /// @brief Set the attack length to a new value.
    ///
    /// @param seconds the number of seconds in the attack stage
    ///
    inline void setAttack(const float& seconds) { integrator.setRise(seconds); }

    /// @brief Return the attack stage length in seconds.
    inline float getAttack() const { return integrator.getRise(); }

    /// @brief Set the attack shape to a new value.
    ///
    /// @param shape the new value for the exponential attack shape
    ///
    inline void setAttackShape(const float& shape) { integrator.setRiseShape(shape); }

    /// @brief Return the attack shape exponent.
    inline float getAttackShape() const { return integrator.getRiseShape(); }

    /// @brief Set the release length to a new value.
    ///
    /// @param seconds the number of seconds in the release stage
    ///
    inline void setRelease(const float& seconds) { integrator.setFall(seconds); }

    /// @brief Return the release stage length in seconds.
    inline float getRelease() const { return integrator.getFall(); }

    /// @brief Set the release shape to a new value.
    ///
    /// @param shape the new value for the exponential release shape
    ///
    inline void setReleaseShape(const float& shape) { integrator.setFallShape(shape); }

    /// @brief Return the release shape exponent.
    inline float getReleaseShape() const { return integrator.getFallShape(); }

    /// @brief Set the look-ahead time on the input delay line.
    ///
    /// @param time the look-ahead time for the input signal in seconds
    /// @param sample_rate the sample rate of the encompassing process
    ///
    inline void setLookAhead(const float& time, const float& sample_rate) {
        // Calculate the size based on the look-ahead time and sample rate. Add
        // 1 to ensure the minimum is 1 and that the 1 sample delay is accounted
        // for, i.e., from inserting the input before querying the delay line
        // in the process function.
        const unsigned num_samples = time * sample_rate + 1.f;
        // Return if the size has not changed.
        if (look_ahead.size() == num_samples) return;
        // Clear the delay line and update the size.
        look_ahead.clear();
        look_ahead.resize(num_samples);
    }

    /// @brief Return the look-ahead time based on the sampling rate.
    ///
    /// @param sample_rate the sample rate of the encompassing process
    /// @returns the look-ahead time in seconds
    ///
    inline float getLookAhead(const float& sample_rate) const {
        return (look_ahead.size() - 1.f) / sample_rate;
    }

    /// @brief Set the ratio.
    ///
    /// @param value the ratio of amplitude to amplitude past the threshold
    ///
    inline void setRatio(const float& value) { ratio = std::max(value, 1.f); }

    /// @brief Return the ratio in units.
    inline float getRatio() const { return ratio; }

    /// @brief Return the slope based on the ratio.
    inline float getSlope() const { return 1.f - (1.f / ratio); }

    /// @brief Set the amplitude threshold in decibels (\f$db\f$).
    ///
    /// @param value the amplitude threshold for generating compression
    ///
    inline void setThreshold(const float& value) {
        threshold = Math::clip(value, -60.f, 12.f);
    }

    /// @brief Return the amplitude threshold in units.
    inline float getThreshold() const { return threshold; }

    /// @brief Set the makeup gain as an amplitude scale factor.
    ///
    /// @param value the multiplicative makeup gain applied after compression
    /// @details
    /// The minimal makeup gain is 1.
    ///
    inline void setMakeup(const float& value) { makeup = std::max(value, 1.f); }

    /// @brief Return the makeup gain in units.
    inline float getMakeup() const { return makeup; }

    /// @brief Set the compressor to expander mode.
    ///
    /// @param value true to enable expander mode, false to disable
    ///
    inline void setExpander(const bool& value) { is_expander = value; }

    /// @brief Return true if the compressor is in expander mode.
    inline bool isExpander() const { return is_expander; }

    /// @brief Set the compressor to upward mode.
    ///
    /// @param value true to enable upward mode, false to disable
    ///
    inline void setUpward(const bool& value) { is_upward = value; }

    /// @brief Return true if the compressor is in upward mode.
    inline bool isUpward() const { return is_upward; }

    /// @brief Set the compressor to soft-knee mode.
    ///
    /// @param value true to enable soft-knee mode, false to disable
    ///
    inline void setSoftKnee(const bool& value) { is_soft_knee = value; }

    /// @brief Return true if the compressor is in soft-knee mode.
    inline bool isSoftKnee() const { return is_soft_knee; }

    /// @brief Set the compressor to peak metering mode.
    ///
    /// @param value true to enable peak metering mode, false to disable
    ///
    inline void setPeakMetering(const bool& value) { is_peak_metering = value; }

    /// @brief Return true if the compressor is in peak metering mode.
    inline bool isPeakMetering() const { return is_peak_metering; }

    /// @brief Return the value of the envelope generator.
    inline float getEnvelope() const { return envelope; }

    /// @brief Return the value of the envelope generator as direct current.
    inline float getEnvelopeDirect() const {
        return is_upward ? envelope - 1.f : 1.f - envelope;
    }

    /// @brief Process a sample through the compressor.
    ///
    /// @param input the input signal to compress
    /// @param side the signal to analyze for dynamics events
    /// @param deltaTime the amount of time between this sample and the last
    /// @returns the signal after processing with the compressor
    ///
    float process(const float& input, const float& side, const float& deltaTime) {
        // Put the input signal sample into the look-ahead delay line.
        look_ahead.insert(input);
        // Calculate the current RMS and peak of the signal in decibels (dB).
        const float level = is_peak_metering ?
            Math::amplitude2decibels(side) : meter.process(side);
        // Calculate the gain of the signal relative to the threshold. Determine
        // the relativity based on the mode of the compressor
        const float diff = getSlope() * (is_expander ? level - threshold : threshold - level);
        // If knee is soft, use a hyperbolic tangent to smooth the transition.
        const float gain = is_soft_knee ?
            Math::Knee::hyperbolic_tangent(diff) : Math::Knee::hard(diff);
        // Convert the gain back to amplitude through the decibel scale.
        const float amplitude = std::pow(10.f, gain / 20.f);
        // Invert the gain about 0 for the integrator and process it.
        integrator.process(1.f - amplitude, deltaTime);
        // Determine the direction of the envelope based on the compressor mode.
        envelope = is_upward ?
            1.f + integrator.getValue() : 1.f - integrator.getValue();
        // Modulate the input signal by the compressor envelope and makeup gain.
        // Get the input signal from the back of the delay line.
        return makeup * look_ahead.at(1) * envelope;
    }
};

}  // namespace DSP

#endif  // DSP_COMPRESSOR
