// Basic math functions.
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

#ifndef DSP_FILTER_COMPRESSION_FILTER_HPP
#define DSP_FILTER_COMPRESSION_FILTER_HPP

#include <algorithm>
#include "../math.hpp"
#include "iir.hpp"

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief A class for performing multi-band soft-clipping on an input signal.
template<typename T>
struct MultiBandSoftClipper {
 private:
    /// a filter for extracting the high-end
    IIR::Bilinear::DirectForm2Transpose<T> highpass;
    /// a filter for extracting the low-end
    IIR::Bilinear::DirectForm2Transpose<T> lowpass;

 public:
    /// @brief Set the parameters of the multi-band soft-clipper filters.
    ///
    /// @param cutoffHigh the cut-off frequency of the high band in radians
    /// @param cutoffLow the cut-off frequency of the low band in radians
    ///
    inline void setFilterParameters(T cutoffHigh, T cutoffLow) {
        Filter::IIR::Bilinear::highpass(highpass.b, highpass.a, cutoffHigh);
        Filter::IIR::Bilinear::lowpass(lowpass.b, lowpass.a, cutoffLow);
    }

    /// @brief Process the given signal.
    ///
    /// @param signal the signal to process with the multi-band dynamics
    /// @param gainHigh the gain to apply to the high band
    /// @param gainMid the gain to apply to the mid band
    /// @param gainLow the gain to apply to the low band
    /// @returns the output signal from the multi-band dynamics
    ///
    inline T process(T signal, T gainHigh = 1.f, T gainMid = 1.f, T gainLow = 1.f) {
        // separate the bands using the high-pass, low-pass filter combo
        T high = highpass.process(signal);
        T low = lowpass.process(signal);
        T mid = signal - high - low;
        // apply each gain individually and apply soft-clipping.
        return Math::Clipping::cubic(gainHigh * high) +
               Math::Clipping::cubic(gainMid * mid) +
               Math::Clipping::cubic(gainLow * low);
    }
};

/// @brief A module that performs Sine compression filtering.
/// @details
/// This technique of sine compression consists of: (1) applying multi-band
/// dynamics for band-dependent soft clipping, (2) notch filtering to remove
/// nasty tones and provide a subtractive element, and (3) a global soft
/// clip on the output waveform.
///
template<typename T>
class CompressionFilter {
 private:
    /// the multi-band dynamics processor
    MultiBandSoftClipper<T> dynamics;
    /// a filter for removing specific bands of spectral content
    IIR::Biquadratic::DirectForm2Transpose<T> notch;

 public:
    /// @brief Set the parameters of the multi-band dynamics filters.
    ///
    /// @param cutoff the cut-off frequency of the notch filter in Hz
    /// @param quality the Q value of the notch filter
    /// @param bandHigh the frequency cut-off for the high-band
    /// @param bandLow the frequency cut-off for the low-band
    ///
    inline void setFilterParameters(
        const T& cutoff,
        const T& quality = 1.f,
        const T& bandHigh = 2000.f,
        const T& bandLow = 250.f
    ) {
        dynamics.setFilterParameters(bandHigh, bandLow);
        Filter::IIR::Biquadratic::bainter_bandstop(notch.b, notch.a, cutoff, cutoff, quality);
    }

    /// @brief Process the given signal.
    ///
    /// @param signal the signal to process with the module
    /// @param gainHigh the gain to apply to the high band
    /// @param gainMid the gain to apply to the mid band
    /// @param gainLow the gain to apply to the low band
    /// @param clipOutput the value of the output clip parameter
    /// @returns the output signal from the module
    ///
    inline T process(T signal,
        T gainHigh   = 1.f,
        T gainMid    = 1.f,
        T gainLow    = 1.f,
        T gainOutput = 1.f
    ) {
        // process the signal with the multi-band dynamics
        signal = dynamics.process(signal, gainHigh, gainMid, gainLow);
        // filter the signal using the notch filter to remove harmonics
        signal = notch.process(signal);
        // clip the signal to add additional full-band harmonics
        signal = Math::Clipping::cubic(gainOutput * signal);
        return signal;
    }
};

}  // namespace Filter

#endif  // DSP_FILTER_COMPRESSION_FILTER_HPP
