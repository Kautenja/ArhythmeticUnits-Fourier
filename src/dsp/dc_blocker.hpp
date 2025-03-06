// A DC Blocking filter.
//
// Copyright 2025 Arhythmetic Units
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_DC_BLOCKER_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_DC_BLOCKER_HPP_

/// @brief Structure for digital filtering.
namespace Filter {

/// @brief A DC blocking filter comprised of a digital differentiator and a
/// leaky integrator in cascade.
/// @details
///
/// ## Digital Differentiator
///
/// DC can be perfectly blocked using a _digital differentiator_ that results
/// in infinite attenuation of DC, but has a logarithmic roll-off into the
/// higher order frequency components that is undesirable. The digital
/// differentiator has a transfer function in the \f$z\f$-domain with a zero at
/// \f$z = 1\f$:
///
/// \f$H_{\text{differentiator}}(z) = 1 - z^{-1}\f$
///
/// In the time domain, this becomes simply:
///
/// \f$y[n] = x[n] - x[n - 1]\f$
///
/// ## Leaky Integrator
///
/// To counteract the effect on the higher frequency components, a pole can be
/// placed near the zero to flatten out the pass-band and sharpen the
/// transition band. A coefficient \f$p \in (0, 1)\f$ controls the proximity of
/// the pole to \f$z = 1\f$. As \f$p \to 1\f$, the transition width approaches
/// \f$0\f$. The transfer function of the one-pole filter can be written as:
///
/// \f$H_{\text{integrator}}(z) = \frac{1}{1 - z^{-1}}\f$
///
/// which has a time domain representation of:
///
/// \f$y[n] - p y[n - 1] = x[n]\f$
///
/// ## DC Blocker
///
/// A DC blocking filter can be formed as the cascade of the digital
/// differentiator with a leaky integrator where \f$p \in (0, 1)\f$ controls
/// the transition band width.
///
/// \f$H(z) = H_{\text{differentiator}}(z)H_{\text{integrator}}(z)\f$
///
/// In the time domain, this becomes:
///
/// \f$y[n] - p y[n - 1] = x[n] - x[n - 1]\f$
///
template<typename T>
struct DCBlocker {
 private:
    /// the coefficient for the feedback line that controls the transition
    /// width. The default of 0.999 produces a transition width of 10Hz for
    /// signals sampled at rates of 44100Hz.
    T p;
    /// the delayed input signal for the digital differentiator
    T last_input;
    /// the delayed output signal for the leaky integrator
    T output;

 public:
    DCBlocker() : p(0.999), last_input(0.0), output(0.0) { }

    /// @brief Reset the state of the filter.
    inline void reset() { last_input = output = 0.0; }

    /// @brief Set the transition width in \f$Hz\f$.
    ///
    /// @param width the width of the transition band in \f$Hz\f$
    /// @param sample_rate the sample rate of the external processing loop
    ///
    inline void setTransitionWidth(const T& width, const T& sample_rate) {
        p = T(1.0) - (T(2.0) * width / sample_rate);
    }

    /// @brief Set the transition width in \f$Hz\f$.
    ///
    /// @param sample_rate the sample rate of the external processing loop
    /// @returns the transition width measured in \f$Hz\f$
    ///
    inline T getTransitionWidth(const T& sample_rate) const {
        return sample_rate * (T(1.0) - p) / T(2.0);
    }

    /// @brief Process a sample using the filter.
    ///
    /// @param input the input sample to filter
    /// @details
    /// the output sample is calculated as:
    ///
    /// \f$y[n] = g(x[n] - x[n - 1]) + p y[n - 1]\f$
    ///
    /// where \f$g\f$ is a gain correction coefficient calculated as
    ///
    /// \f$g = \frac{1}{H(-1)} = \frac{1 + p}{2}\f$
    ///
    inline const T& process(const T& input) {
        // Calculate the output from the filter and store the value as the last
        // output for the next call to `process`, i.e., y[n - 1]
        output = ((T(1.0) + p) / T(2.0)) * (input - last_input) + p * output;
        // Store the current input to be the last input for the next call to
        // `process`, i.e., x[n - 1]
        last_input = input;
        return output;
    }

    /// @brief Return the output value from the filter.
    inline const T& getValue() const { return output; }
};

}  // namespace Filter

#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_DC_BLOCKER_HPP_
