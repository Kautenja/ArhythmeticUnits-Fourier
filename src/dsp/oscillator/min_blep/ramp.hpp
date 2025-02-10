// An oscillator that generates a saw wave using rack::MinBlep.
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

#ifndef DSP_OSCILLATORS_MINBLEP_RAMP_HPP
#define DSP_OSCILLATORS_MINBLEP_RAMP_HPP

#include "rack.hpp"
#include "../../math.hpp"
#include "_common.hpp"

/// @brief Structures for generating oscillating signals.
namespace Oscillator {

/// @brief Oscillators based on the VCV Rack MinBlepGenerator.
namespace MinBlep {

/// @brief the different directions for the ramps
enum class RampDirection { Up = 0, Down = 1 };

/// @brief Calculate the ramp waveform.
///
/// @param phase the current phase of the wave
///
template<RampDirection>
float ramp(float phase);
// ramp up
template<>
inline float ramp<RampDirection::Up>(float phase) {
    return 2 * phase - 1.f;
}
// ramp down (ramp up with inverted phase)
template<>
inline float ramp<RampDirection::Down>(float phase) {
    return ramp<RampDirection::Up>(1.f - phase);
}

/// @brief Return the discontinuity value for the ramp direction.
///
/// @param phase the current phase of the wave
///
template<RampDirection>
float jump_discontinuity();
template<>
// ramp up goes from 1 to -1, jump of -2
inline float jump_discontinuity<RampDirection::Up>() { return -2.0; }
template<>
// ramp up goes from -1 to 1, jump of 2
inline float jump_discontinuity<RampDirection::Down>() { return 2.0; }

/// @brief An oscillator that generates a saw wave.
template<RampDirection direction>
struct Ramp {
 private:
    /// the Min BLEP generator for anti-aliasing the saw wave
    rack::dsp::MinBlepGenerator<16, 32, float> minBLEP;

    /// the current phase of the oscillator
    float phase = 0.0;
    /// the current frequency of the oscillator
    float freq = 440.f;
    /// the value from the last oscillator synchronization
    float lastSyncValue = 0.0;
    /// the output value of the oscillator
    float value = 0.0;

 public:
    /// whether the oscillator is synced to another oscillator
    bool syncEnabled = false;

    /// @brief Set the frequency of the oscillator.
    ///
    /// @param frequency the frequency of the oscillator in Hertz
    ///
    inline void setFrequency(float frequency) { freq = frequency; }

    /// @brief Return the frequency of the oscillator.
    ///
    /// @returns the frequency of the oscillator in Hertz
    ///
    inline float getFrequency() { return freq; }

    /// @brief Return the phase of the oscillator.
    ///
    /// @returns the phase of the oscillator \f$\in [0.0, 1.0]\f$
    ///
    inline float getPhase() const { return phase; }

    /// @brief Return the value from the oscillator.
    ///
    /// @returns the output from the oscillator \f$\in [-1.0, 1.0]\f$
    ///
    inline float getValue() const { return value; }

    /// @brief Process a sample for given sample time and sync value.
    ///
    /// @param deltaTime the change in time between samples
    /// @param syncValue the value of the oscillator to sync to
    ///
    void process(float deltaTime, float syncValue = 0) {
        // Advance phase
        const float deltaPhase = Math::clip(freq * deltaTime, 1e-6f, 0.5f);
        phase += deltaPhase;
        if (phase >= 1.0) {  // end of cycle, wrap phase around max phase (1.0)
            phase -= 1.0;
            const float crossing = -phase / deltaPhase;
            minBLEP.insertDiscontinuity(crossing, jump_discontinuity<direction>());
        }
        if (syncEnabled)  // sync the oscillator to the input oscillator
            hardSync(syncValue, lastSyncValue, phase, deltaPhase, [](float x){ return ramp<direction>(x); }, minBLEP);
        // set the value of the oscillator
        value = ramp<direction>(phase);
        value += minBLEP.process();
    }
};

// typedef shortcuts for the ramp down and ramp up classes
typedef Ramp<RampDirection::Down> RampDown;
typedef Ramp<RampDirection::Up> RampUp;

}  // namespace MinBlep

}  // namespace Oscillator

#endif  // DSP_OSCILLATORS_MINBLEP_RAMP_HPP
