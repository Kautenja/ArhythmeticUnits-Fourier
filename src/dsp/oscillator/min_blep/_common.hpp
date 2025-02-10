// Common code for structures in the Oscillator::MinBlep package.
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

#ifndef DSP_OSCILLATORS_MINBLEP_COMMON_HPP
#define DSP_OSCILLATORS_MINBLEP_COMMON_HPP

#include <functional>
#include "rack.hpp"
#include "../../math.hpp"

/// @brief Structures for generating oscillating signals.
namespace Oscillator {

/// @brief Oscillators based on the VCV Rack MinBlepGenerator.
namespace MinBlep {

/// @brief Hard sync the phase of an oscillator given an input waveform.
///
/// @param sync the input signal to sync to
/// @param lastSync a reference to a variable containing the last sync value
/// @param phase the current phase of the oscillator
/// @param deltaPhase the change in phase for the current sample
/// @param waveform the function mapping phase to the waveform output
/// @param minBLEP the MinBlepGenerator for the waveform generator
/// @returns true if a sync event occurred
///
template<int Z, int O, typename T = float>
bool hardSync(
    const float& sync,
    float& lastSync,
    float& phase,
    const float& deltaPhase,
    std::function<float(float)> waveform,
    rack::dsp::MinBlepGenerator<Z, O, T>& minBLEP
) {
    const float deltaSync = sync - lastSync;
    const float crossing = -lastSync / deltaSync;
    lastSync = sync;
    const bool isSync = (0 < crossing) && (crossing <= 1.f) && (sync >= 0.f);
    const float newPhase = isSync ? (1.f - crossing) * deltaPhase : phase;
    const float p = crossing - 1.f;
    const float x0 = waveform(phase);
    const float x1 = waveform(newPhase);
    minBLEP.insertDiscontinuity(p, x1 - x0);
    phase = newPhase;
    return isSync;
}

}  // namespace MinBlep

}  // namespace Oscillator

#endif  // DSP_OSCILLATORS_MINBLEP_COMMON_HPP
