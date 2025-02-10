// An oscillator that generates a sine wave using rack::MinBlep.
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

#ifndef DSP_OSCILLATORS_MINBLEP_SINE_HPP
#define DSP_OSCILLATORS_MINBLEP_SINE_HPP

#include "rack.hpp"
#include "../../math.hpp"
#include "_common.hpp"

/// @brief Structures for generating oscillating signals.
namespace Oscillator {

/// @brief Oscillators based on the VCV Rack MinBlepGenerator.
namespace MinBlep {

/// @brief An oscillator that generates a sine wave.
struct Sine {
 private:
    /// the Min BLEP generator for anti-aliasing the sine wave
    rack::dsp::MinBlepGenerator<16, 32, float> minBLEP;

    /// the current phase of the oscillator
    float phase = 0.0;
    /// the current frequency of the oscillator
    float freq = 440.f;
    /// the value from the last oscillator synchronization
    float lastSyncValue = 0.0;
    /// the output value of the oscillator
    float value = 0.0;

    /// @brief Return the value of a sine function based on unit phase.
    ///
    /// @param phase the current phase in the wave
    /// @returns the sine wave value for given phase
    ///
    static inline float sine(float phase) {
        return -std::cos(2 * M_PI * phase);
    }

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
        const float deltaPhase = Math::clip(freq * deltaTime, 1e-6f, 0.5f);
        phase += deltaPhase;
        if (phase >= 1.0) phase -= 1.0;
        if (syncEnabled)  // sync the oscillator to the input oscillator
            hardSync(syncValue, lastSyncValue, phase, deltaPhase, [](float x){ return sin(x); }, minBLEP);
        value = sine(phase);
        value += minBLEP.process();
    }
};

}  // namespace MinBlep

}  // namespace Oscillator

#endif  // DSP_OSCILLATORS_MINBLEP_SINE_HPP
