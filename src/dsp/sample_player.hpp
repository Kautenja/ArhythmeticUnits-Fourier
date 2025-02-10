// A sample playback engine.
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

#ifndef DSP_SAMPLE_PLAYER
#define DSP_SAMPLE_PLAYER

#include <iostream>
#include "./filter/fir/direct_form.hpp"
#include "./io.hpp"

namespace DSP {

/// @brief a 32-bit sample player.
struct SamplePlayer {
 public:
    /// the number of taps in the anti-aliasing filter
    static constexpr unsigned ANTI_ALIAS_TAPS = 64;
    /// whether the sample playback should loop
    bool is_looping = false;

 private:
    /// the sample being played
    IO::AudioBuffer sample;

    /// @brief The playback rate of the sample player.
    /// @details
    /// When the rate is positive (negative), the sample is played
    /// forwards (backwards). At 0, sample playback is halted. When the
    /// absolute value of the rate is above 1, the sample is played back faster
    /// via decimation. When the absolute value of the rate is below 1, the
    /// sample is played slower via interpolation.
    float playback_rate = 0;
    /// @brief The normalized position of the first sample \f$\in [0, 1]\f$.
    float start = 0;
    /// @brief The normalized position of the last sample \f$\in [0, 1]\f$.
    float end = 1;

    /// the index of the current sample
    int64_t sample_index = 0;
    /// the fractional component of the current sample index
    float sample_fractional = 0;

    /// low-pass filters for up/down-sampling
    Filter::FIR::DirectForm<float> filters[2];

    /// the gain of the sample player
    float gain = 1;
    /// the panning of the sample player between left and right channels
    float panning = 0;

    /// @brief Increment the position of the playback head.
    ///
    /// @param deltaTime the amount of time between the last and current sample
    /// @returns The number of discrete samples that elapsed.
    ///
    int64_t increment_position(const float& deltaTime) {
        // Increment the floating point position based on the playback rate
        sample_fractional += playback_rate * sample.getSampleRate() * deltaTime;
        int64_t num_samples = static_cast<int64_t>(sample_fractional);
        if (abs(sample_fractional) >= 1) {
            // Update sample index with integer component of the change
            sample_index += num_samples;
            // Remove integer component from fractional integrator.
            sample_fractional -= num_samples;
        }
        // play-head advanced past the end
        if (sample_index > getEndSample() && is_looping && playback_rate > 0)
            // subtract the length of the sample to reset to the start
            sample_index -= getNumPlaybackSamples();
        // play-head recessed before the start
        else if (sample_index < getStartSample() && is_looping && playback_rate < 0)
            // add the length of the sample to reset to the end
            sample_index += getNumPlaybackSamples();
        // Clamp the index to the correct range
        sample_index = Math::clip(sample_index, getStartSample(), getEndSample());
        return num_samples;
    }

 public:
    /// @brief Initialize a new sample player.
    SamplePlayer() { for (auto& filter : filters) filter.resize(ANTI_ALIAS_TAPS); }

    /// @brief Return the audio buffer that represents the sample being played.
    inline IO::AudioBuffer& getBuffer() { return sample; }

    /// @brief Set the starting position of the sample.
    ///
    /// @param start the starting position of the sample
    ///
    inline void setStart(const float& point) {
        start = Math::clip(point, 0.f, end);
    }

    /// @brief Return the normalized starting position of the sample.
    inline float getStart() const { return start; }

    /// @brief Return the starting sample of the sample.
    inline int64_t getStartSample() const { return start * (sample.getNumSamples() - 1); }

    /// @brief Set the ending position of the sample.
    ///
    /// @param end the ending position of the sample
    ///
    inline void setEnd(const float& point) {
        end = Math::clip(point, start, 1.f);
    }

    /// @brief Return the normalized ending position of the sample.
    inline float getEnd() const { return end; }

    /// @brief Return the ending sample of the sample.
    inline int64_t getEndSample() const { return end * (sample.getNumSamples() - 1); }

    /// @brief Return the duration of the sample relative to start and end.
    inline float getDuration() const { return end - start; }

    /// @brief Return the playback length of the sample in samples.
    inline float getNumPlaybackSamples() const {
        return getDuration() * sample.getNumSamples();
    }

    /// @brief Set the playback rate.
    ///
    /// @param rate the rate to play the sample at relative to the sample rate
    ///
    inline void setPlaybackRate(const float& rate) {
        // Calculate the magnitude of the playback rate bounded above 0
        const float magnitude = std::max(abs(rate), std::numeric_limits<float>::epsilon());
        const float new_playback_rate = Math::sgn(rate) * magnitude;
        // If the magnitude has not changed, return to reduce calculations
        if (new_playback_rate == playback_rate) return;
        // Update the playback rate and calculate new filter coefficients.
        playback_rate = new_playback_rate;
        // Set the cut-off frequency of the anti-aliasing filter
        const float cutoff = (magnitude > 1 ? 1.f / magnitude : magnitude);
        filters[0].setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::blackman<float>, cutoff);
        filters[1].setCoefficients(&Filter::FIR::lowpass<float>, &Math::Window::blackman<float>, cutoff);
    }

    /// @brief Return the playback rate relative to the sample rate.
    inline float getPlaybackRate() const { return playback_rate; }

    /// @brief Get the frequency of the play-back engine.
    ///
    /// @returns the number of sample completions per second
    ///
    float getFrequency() const {
        return abs(playback_rate) * sample.getSampleRate() / sample.getNumSamples();
    }

    /// @brief Return the current discrete position of the sample play-head.
    inline float getSampleIndex() const { return sample_index; }

    /// @brief Return the current relative position of the sample play-head.
    inline float getPhase() const { return sample_index / sample.getNumSamples(); }

    /// @brief Set the gain to a new level.
    ///
    /// @param value the new multiplicative gain factor
    ///
    inline void setGain(const float& value) { gain = value; }

    /// @brief Return the gain applied to both channels.
    inline float getGain() const { return gain; }

    /// @brief Set the panning to a new level.
    ///
    /// @param value the new panning value
    /// @details
    /// When \f$value = 0\f$ panning is centered. When \f$value > 0\f$, the
    /// audio pans right and when value < 0, the audio pans right. Full panning
    /// right(left) is achieved at \f$value = 1\f$(\f$value = -1\f$).
    ///
    inline void setPanning(const float& value) { panning = value; }

    /// @brief Return the panning control.
    inline float getPanning() const { return panning; }

    /// @brief Return the sample for the given channel and index.
    ///
    /// @param channel the stereo channel to get a sample from
    /// @param index the index of the sample to return
    ///
    float getSample(const IO::AudioBuffer::Channel& channel, const unsigned& index) const {
        const float panGain = (channel == IO::AudioBuffer::Channel::Left ?
            (panning > 0 ? 1 - panning : 1) : (panning > 0 ? 1 : 1 + panning));
        return gain * panGain * sample.getSample(channel, index);
    }

    /// @brief Return the sample for the given channel.
    ///
    /// @param channel the stereo channel to get the sample for
    /// @returns a vector of samples for the given channel
    ///
    // std::vector<float> getSamples(IO::AudioBuffer::Channel channel) {
    //     // Get the gain placed on the channel, panel setting, and global gain.
    //     const float GAIN = gain * (channel == 0 ?
    //         (panning > 0 ? 1 - panning : 1) : (panning > 0 ? 1 : 1 + panning));
    //     // Create a placeholder vector with the given number of samples.
    //     std::vector<float> result(sample.getNumSamples());
    //     for (unsigned i = 0; i < sample.getNumSamples(); i++)
    //         result[i] = GAIN * sample.getSample(channel, i);
    //     return result;
    // }

    /// @brief Reset the internal state of the sample player.
    ///
    /// @param to_terminal whether to reset to the terminal position
    ///
    inline void reset(bool to_terminal = false) {
        if (to_terminal)  // reset index to the back of the playback window
            sample_index = playback_rate > 0 ? getEndSample() : getStartSample();
        else  // reset index to the front of the playback window
            sample_index = playback_rate > 0 ? getStartSample() : getEndSample();
        sample_fractional = 0;
        // reset the interpolation / decimation filter delay lines
        for (auto& filter : filters) filter.reset();
    }

    /// @brief A stereo sample.
    struct StereoSample {
        /// The left and right samples in the stereo sample.
        float samples[2] = {0, 0};
    } __attribute__((packed));

    /// @brief Process a sample from the sample player.
    ///
    /// @param deltaTime the amount of time between the last and current sample
    /// @returns the output stereo sample
    ///
    StereoSample process(const float& deltaTime) {
        // playback_rate = 1;
        int64_t last_sample_index = sample_index;
        // Update the position of the play-head.
        float samples = increment_position(deltaTime);
        // Create a placeholder for the output sample.
        StereoSample result;

        if (abs(playback_rate * sample.getSampleRate() * deltaTime) > 1) {  // decimate
            int64_t begin_index = last_sample_index - (playback_rate > 0 ? ANTI_ALIAS_TAPS : -static_cast<int64_t>(ANTI_ALIAS_TAPS));
            // play-head advanced past the end
            if (begin_index > getEndSample())// && playback_rate > 0)
                // subtract the length of the sample to reset to the start
                begin_index -= getNumPlaybackSamples();
            // play-head recessed before the start
            else if (begin_index < getStartSample())// && playback_rate < 0)
                // add the length of the sample to reset to the end
                begin_index += getNumPlaybackSamples();
            begin_index = Math::clip(begin_index, getStartSample(), getEndSample());

            while (begin_index != sample_index) {
                filters[0].process_delay(sample.getSample(IO::AudioBuffer::Channel::Left, begin_index));
                filters[1].process_delay(sample.getSample(IO::AudioBuffer::Channel::Right, begin_index));
                begin_index += playback_rate > 0 ? 1 : -1;
                // play-head advanced past the end
                if (begin_index > getEndSample() && playback_rate > 0)
                    // subtract the length of the sample to reset to the start
                    begin_index -= getNumPlaybackSamples();
                // play-head recessed before the start
                else if (begin_index < getStartSample() && playback_rate < 0)
                    // add the length of the sample to reset to the end
                    begin_index += getNumPlaybackSamples();
            }
            int64_t next_index = sample_index + (playback_rate > 0 ? 1 : -1);
            // play-head advanced past the end
            if (next_index > getEndSample() && is_looping && playback_rate > 0)
                // subtract the length of the sample to reset to the start
                next_index -= getNumPlaybackSamples();
            // play-head recessed before the start
            else if (next_index < getStartSample() && is_looping && playback_rate < 0)
                // add the length of the sample to reset to the end
                next_index += getNumPlaybackSamples();
            next_index = Math::clip(next_index, getStartSample(), getEndSample());
            result.samples[0] = Math::interpolate(
                filters[0].process(sample.getSample(IO::AudioBuffer::Channel::Left, sample_index)),
                filters[0].process(sample.getSample(IO::AudioBuffer::Channel::Left, next_index)),
                playback_rate > 0 ? sample_fractional : -sample_fractional
            );
            if (sample.isStereo()) {
                result.samples[1] = Math::interpolate(
                    sample.getSample(IO::AudioBuffer::Channel::Right, sample_index),
                    sample.getSample(IO::AudioBuffer::Channel::Right, next_index),
                    playback_rate > 0 ? sample_fractional : -sample_fractional
                );
            } else {
                result.samples[1] = result.samples[0];
            }
        } else if (abs(playback_rate * sample.getSampleRate() * deltaTime) < 1) {  // interpolate
            // TODO: cleanup this logic
            int64_t next_index = sample_index + (playback_rate > 0 ? 1 : -1);
            // play-head advanced past the end
            if (next_index > getEndSample() && is_looping && playback_rate > 0)
                // subtract the length of the sample to reset to the start
                next_index -= getNumPlaybackSamples();
            // play-head recessed before the start
            else if (next_index < getStartSample() && is_looping && playback_rate < 0)
                // add the length of the sample to reset to the end
                next_index += getNumPlaybackSamples();
            next_index = Math::clip(next_index, getStartSample(), getEndSample());

            result.samples[0] = filters[0].process(Math::interpolate(
                sample.getSample(IO::AudioBuffer::Channel::Left, sample_index),
                sample.getSample(IO::AudioBuffer::Channel::Left, next_index),
                playback_rate > 0 ? sample_fractional : -sample_fractional
            ));
            if (sample.isStereo()) {
                result.samples[1] = filters[1].process(Math::interpolate(
                    sample.getSample(IO::AudioBuffer::Channel::Right, sample_index),
                    sample.getSample(IO::AudioBuffer::Channel::Right, next_index),
                    playback_rate > 0 ? sample_fractional : -sample_fractional
                ));
            } else {
                result.samples[1] = result.samples[0];
            }
        } else {  // standard playback rate
            if (sample.isStereo()) {  // Stereo, process both left and right channels
                result.samples[0] = filters[0].process(sample.getSample(IO::AudioBuffer::Channel::Left, sample_index));
                result.samples[1] = filters[1].process(sample.getSample(IO::AudioBuffer::Channel::Right, sample_index));
            } else {  // Mono, process and copy left to right channel
                result.samples[0] = filters[0].process(sample.getSample(IO::AudioBuffer::Channel::Mono, sample_index));
                result.samples[1] = result.samples[0];
            }
        }

        // Apply the gain and panning settings
        result.samples[0] *= gain * (panning > 0 ? 1 - panning : 1);
        result.samples[1] *= gain * (panning > 0 ? 1 : 1 + panning);

        return result;
    }
};

}  // namespace DSP

#endif  // DSP_SAMPLE_PLAYER
