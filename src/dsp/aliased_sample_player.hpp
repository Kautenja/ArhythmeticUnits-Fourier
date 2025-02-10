// An aliased sample playback engine.
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

#ifndef DSP_ALIASED_SAMPLE_PLAYER
#define DSP_ALIASED_SAMPLE_PLAYER

#include "./io.hpp"

namespace DSP {

/// @brief a 32-bit sample player.
struct AliasedSamplePlayer {
 public:
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

    /// the gain of the sample player
    float gain = 1;
    /// the panning of the sample player between left and right channels
    float panning = 0;

    /// @brief Increment the position of the playback head.
    ///
    /// @param deltaTime the amount of time between the last and current sample
    ///
    void increment_position(const float& deltaTime) {
        // Increment the floating point position based on the playback rate
        sample_fractional += playback_rate * sample.getSampleRate() * deltaTime;
        if (abs(sample_fractional) >= 1) {
            // Update sample index with integer component of the change
            sample_index += static_cast<int64_t>(sample_fractional);
            // Remove integer component from fractional integrator.
            sample_fractional -= static_cast<int64_t>(sample_fractional);
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
    }

 public:
    /// @brief Initialize a new sample player.
    AliasedSamplePlayer() { }

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
        // Update the position of the play-head.
        increment_position(deltaTime);
        // Create a placeholder for the output sample.
        StereoSample result;
        if (sample.isStereo()) {  // Stereo, process both left and right channels
            result.samples[0] = sample.getSample(IO::AudioBuffer::Channel::Left, sample_index);
            result.samples[1] = sample.getSample(IO::AudioBuffer::Channel::Right, sample_index);
        } else {  // Mono, process and copy left to right channel
            result.samples[0] = sample.getSample(IO::AudioBuffer::Channel::Mono, sample_index);
            result.samples[1] = result.samples[0];
        }
        // Apply the gain and panning settings
        result.samples[0] *= gain * (panning > 0 ? 1 - panning : 1);
        result.samples[1] *= gain * (panning > 0 ? 1 : 1 + panning);
        return result;
    }
};

}  // namespace DSP

#endif  // DSP_ALIASED_SAMPLE_PLAYER
