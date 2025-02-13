// Functions for calculating the Fast Fourier Transform (FFT.)
//
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

#ifndef DSP_MATH_FFT_HPP
#define DSP_MATH_FFT_HPP

#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <vector>
#include "constants.hpp"
#include "window.hpp"

/// @brief Basic mathematical functions.
namespace Math {

/// A type for DFT coefficient buffers.
typedef std::vector<std::complex<float>> DFTCoefficients;

/// A type for STFT coefficient matrices.
typedef std::vector<DFTCoefficients> STFTCoefficients;

/// @brief A utility class for pre-computing and storing twiddle factors for a radix-2 FFT.
///
/// This class precomputes the complex exponential coefficients (twiddle factors)
/// used in the computation of the Fast Fourier Transform (FFT). For an FFT of length \f$ N \f$,
/// the twiddle factors are defined as:
/// \f[ W_k = e^{-i \frac{2\pi k}{N}}, \quad k = 0, 1, \dots, \frac{N}{2}-1. \f]
///
/// Only \f$ N/2 \f$ factors are stored due to the symmetry properties of the FFT.
/// Precomputing these factors significantly improves the performance of the FFT by avoiding
/// repeated calls to expensive transcendental functions during the transform computation.
///
/// @note The FFT length \f$ N \f$ should ideally be a power of 2 to work with radix-2 FFT algorithms.
class TwiddleFactors {
 private:
    /// @brief Pre-computed twiddle factors stored as a vector of std::complex<float>.
    ///
    /// Only half the number of factors are stored because the FFT algorithm can exploit
    /// the symmetry in the complex exponentials, where the full set for an N-point FFT is:
    /// \f[ \{ W_0, W_1, \ldots, W_{(N/2)-1} \} \f]
    std::vector<std::complex<float>> factors;

 public:
    /// @brief Constructs a TwiddleFactors object for an N-point FFT.
    ///
    /// @param n The length of the FFT. Ideally, n should be a power of 2.
    ///
    /// The constructor precomputes the twiddle factors by calling the resize() method.
    explicit TwiddleFactors(const size_t& n) {
        resize(n);
    }

    /// @brief Resizes and precomputes the twiddle factor buffer for a new FFT length.
    ///
    /// @param n The new FFT length. Ideally, n should be a power of 2.
    ///
    /// This method performs the following steps:
    /// 1. Resizes the internal vector to hold \f$ N/2 \f$ twiddle factors.
    /// 2. Computes the constant multiplier \f$ e^{-i \frac{2\pi}{N}} \f$.
    /// 3. Uses an iterative approach to compute each factor:
    ///    \f[ \text{current} = \text{multiplier}^i \quad \text{for} \quad i = 0, 1, \ldots, \frac{N}{2}-1, \f]
    ///    where \f$ \text{current} \f$ is initially set to 1.
    inline void resize(const size_t& n) {
        // Resize the vector to store half the number of FFT points.
        factors.resize(n >> 1);  // Equivalent to factors.resize(n / 2)
        // Calculate the angular step theta = -2*pi/n.
        const float theta = -2.0f * M_PI / n;
        // Compute the multiplier: e^{-i 2pi/n} = cos(theta) + i*sin(theta)
        const std::complex<float> multiplier(std::cos(theta), std::sin(theta));
        // Initialize the first twiddle factor to 1 (i.e., e^{0}).
        std::complex<float> current(1.0f, 0.0f);
        // Iteratively compute and store each twiddle factor.
        for (size_t i = 0; i < factors.size(); ++i) {
            factors[i] = current;
            current *= multiplier;
        }
    }

    /// @brief Returns the FFT size corresponding to the stored twiddle factors.
    ///
    /// @return The FFT length \f$ N \f$, computed as twice the number of stored factors.
    ///
    /// Since only \f$ N/2 \f$ twiddle factors are stored (to leverage symmetry), this method
    /// returns \f$ N = 2 \times (\text{number of stored factors}) \f$.
    inline size_t size() const {
        return factors.size() << 1;  // Equivalent to factors.size() * 2.
    }

    /// @brief Accesses a pre-computed twiddle factor by its index.
    ///
    /// @param i The index of the desired twiddle factor (0-based index).
    /// @return A constant reference to the \f$ i \f$th twiddle factor.
    ///
    /// The accessed twiddle factor is given by:
    /// \f[ W_i = e^{-i \frac{2\pi i}{N}}, \f]
    /// where \f$ N \f$ is the FFT length (returned by size()). It is important to note
    /// that valid indices range from 0 to \f$ \frac{N}{2} - 1 \f$. Accessing an index outside
    /// this range leads to undefined behavior.
    const std::complex<float>& operator[](const size_t& i) const {
        return factors[i];
    }
};

/// @brief Pre-computed bit-reversal table for radix-2 FFT.
class BitReversalTable {
 private:
    /// The pre-computed bit-reversal table.
    std::vector<size_t> table;

 public:
    /// @brief Initialize a bit-reversal table for an N-point FFT.
    /// @param n The length of the FFT.
    explicit BitReversalTable(const size_t& n) { resize(n); }

    /// @brief Resize the bit-reversal table.
    /// @param n The length of the FFT.
    inline void resize(const size_t& n) {
        table.resize(n);
        size_t log2n = static_cast<size_t>(log2f(n));
        for (size_t i = 0; i < n; ++i) {
            size_t y = 0;
            size_t x = i;
            for (size_t j = 0; j < log2n; ++j) {
                y = (y << 1) | (x & 1);
                x >>= 1;
            }
            table[i] = y;
        }
    }

    /// @brief Return the length of the FFT.
    inline size_t size() const { return table.size(); }

    /// @brief Return the bit-reversal index at the given index.
    /// @param idx The index of the bit-reversal index to access.
    /// @returns The bit-reversal index at the given index.
    const size_t& operator[](size_t idx) const { return table[idx]; }
};

/// @brief An FFT computation utility based on pre-computed Twiddle factors.
class OnTheFlyFFT {
 private:
    /// @brief Pre-computed bit-reversal table for an N-point FFT.
    BitReversalTable bit_reversal;
    /// @brief Pre-computed twiddle factors for an N-point FFT.
    TwiddleFactors twiddles;

    /// The current step of the Cooley-Tukey algorithm.
    size_t step_ = 2;
    /// The current group of the Cooley-Tukey algorithm.
    size_t group = 0;
    /// The current pair of the Cooley-Tukey algorithm.
    size_t pair = 0;
    /// The total number of steps needed to compute the FFT.
    size_t total_steps = 0;

 public:
    /// @brief The coefficients to compute on-the-fly
    std::vector<std::complex<float>> coefficients;

    /// @brief Initialize the FFT
    /// @param n The length of the FFT.
    OnTheFlyFFT(const size_t& n) : bit_reversal(n), twiddles(n), coefficients(n) { }

    /// @brief Resize the FFT.
    /// @param n The length of the FFT.
    inline void resize(const size_t& n) {
        bit_reversal.resize(n);
        twiddles.resize(n);
        coefficients.resize(n);
        total_steps = (n >> 1) * static_cast<size_t>(log2f(n));
        std::fill(coefficients.begin(), coefficients.end(), 0.f);
    }

    /// @brief Return the length of the FFT.
    inline size_t size() const { return twiddles.size(); }

    /// @brief Return the total number of steps needed to compute the FFT.
    inline size_t get_total_steps() const { return total_steps; }

    /// @brief Buffer input samples for on-the-fly computation.
    /// @param samples The sample buffer of length N.
    /// @param window The window function to use before computing the DFT.
    inline void buffer(
        const std::complex<float>* samples,
        const std::vector<float>& window
    ) {
        const auto N = coefficients.size();
        // Copy the coefficients into the buffer.
        memcpy(coefficients.data(), samples, N * sizeof(std::complex<float>));
        // Window the local copy of the coefficients.
        for (size_t n = 0; n < N; ++n) coefficients[n] *= window[n];
        // Bit-reversal Permutation using Pre-computed Table.
        for (size_t n = 0; n < N; ++n)
            if (n < bit_reversal[n])
                std::swap(coefficients[n], coefficients[bit_reversal[n]]);
        // Reset the FFT computation iterator variables.
        step_ = 2;
        group = 0;
        pair = 0;
    }

    /// @brief Perform a single step of the FFT computation.
    inline void step() {
        if (is_done_computing()) return;
        // Determine the half step and twiddle stride.
        const size_t half_step = step_ >> 1;
        const size_t twiddle_stride = coefficients.size() / step_;
        // Compute the inner logic of the Cooley-Tukey algorithm.
        const auto w = twiddles[pair * twiddle_stride];
        const auto even = coefficients[group + pair];
        const auto odd = coefficients[group + pair + half_step] * w;
        coefficients[group + pair] = even + odd;
        coefficients[group + pair + half_step] = even - odd;
        // Update the loop iterators.
        pair = pair + 1;
        if (pair >= half_step) {
            pair = 0;
            group = group + step_;
            if (group >= coefficients.size()) {
                group = 0;
                step_ <<= 1;
            }
        }
    }

    /// @brief Perform a batch of steps targeting a known hop-rate.
    /// @param hop_length The number of samples between FFT computations.
    /// @details
    /// This call to `step` performs the number of steps required to compute
    /// the FFT in `hop_length` total samples.
    inline void step(const size_t& hop_length) {
        const auto num_steps = ceilf(get_total_steps() / static_cast<float>(hop_length));
        for (size_t i = 0; i < num_steps; i++) step();
    }

    /// @brief Return a flag determining whether the computation is complete.
    /// @returns True if the FFT is done computing, False otherwise.
    inline bool is_done_computing() const {
        return step_ > coefficients.size();
    }
};

/// @brief An RFFT computation utility based on pre-computed Twiddle factors.
class OnTheFlyRFFT {
 private:
    /// @brief The N/2 point FFT.
    OnTheFlyFFT fft;
    /// @brief Pre-computed twiddle factors for reconstructing an N-point FFT.
    TwiddleFactors twiddles;

 public:
    /// @brief The coefficients to compute on-the-fly
    std::vector<std::complex<float>> coefficients;

    /// @brief Initialize the RFFT
    /// @param n The length of the RFFT.
    OnTheFlyRFFT(const size_t& n) : fft(n >> 1), twiddles(n), coefficients(n) { }

    /// @brief Resize the RFFT.
    /// @param n The length of the RFFT.
    inline void resize(const size_t& n) {
        fft.resize(n >> 1);
        twiddles.resize(n);
        coefficients.resize(n);
        std::fill(coefficients.begin(), coefficients.end(), 0.f);
    }

    /// @brief Return the length of the RFFT.
    inline size_t size() const {
        return coefficients.size();
    }

    /// @brief Return the total number of steps needed to compute the FFT.
    inline size_t get_total_steps() const { return fft.get_total_steps(); }

    /// @brief Buffer input samples for on-the-fly computation.
    /// @param samples The sample buffer of length N.
    /// @param window The window function to use before computing the DFT.
    inline void buffer(const float* samples, const std::vector<float>& window) {
        const size_t M = size() >> 1;
        // Create a temporary packed array.
        std::vector<std::complex<float>> packed(M);
        for (size_t k = 0; k < M; ++k) {
            float r = samples[2 * k]     * window[2 * k];
            float i = samples[2 * k + 1] * window[2 * k + 1];
            packed[k] = std::complex<float>(r, i);
        }
        // Since the window has already been applied, pass a unity window.
        std::vector<float> unity(M, 1.0f);
        fft.buffer(packed.data(), unity);
    }

    /// @brief Perform a single step of the FFT computation.
    inline void step() {
        if (fft.is_done_computing())
            return;
        fft.step();
        if (fft.is_done_computing())
            finalize();
    }

    /// @brief Perform a batch of steps targeting a known hop-rate.
    /// @param hop_length The number of samples between FFT computations.
    /// @details
    /// This call to `step` performs the number of steps required to compute
    /// the FFT in `hop_length` total samples.
    inline void step(const size_t& hop_length) {
        const auto num_steps = ceilf(get_total_steps() / static_cast<float>(hop_length));
        for (size_t i = 0; i < num_steps; i++) step();
    }

    /// @brief Return a flag determining whether the computation is complete.
    /// @returns True if the FFT is done computing, False otherwise.
    inline bool is_done_computing() const {
        return fft.is_done_computing();
    }

    /// @brief Finalize interleaved N/2-point FFT coefficients.
    inline void finalize() {
        const auto N = size();    // full FFT length.
        const size_t M = N >> 1;  // M = N/2.
        // Handle DC and Nyquist bins.
        float re0 = fft.coefficients[0].real();
        float im0 = fft.coefficients[0].imag();
        coefficients[0] = std::complex<float>(re0 + im0, 0.0f);
        coefficients[M] = std::complex<float>(re0 - im0, 0.0f);
        // Reconstruct bins 1 .. M-1.
        for (size_t k = 1; k < M; k++) {
            std::complex<float> A = fft.coefficients[k];
            std::complex<float> B = std::conj(fft.coefficients[M - k]);
            std::complex<float> Wk = twiddles[k]; // Wk = exp(-j*2pi*k/N)
            std::complex<float> Xk = 0.5f * (A + B - std::complex<float>(0, 1) * Wk * (A - B));
            coefficients[k]     = Xk;
            coefficients[N - k] = std::conj(Xk);
        }
    }
};

/// @brief Smooth the FFT magnitudes over octave-based bandwidths.
/// @param fft_coeffs         Full FFT array, size = N.
/// @param sample_rate        Sampling frequency in Hz.
/// @param fraction_of_octave Fraction-of-an-octave for smoothing (e.g. 1.0 = 1 octave, 1/6 = 1/6 octave).
/// @param f_min              Minimum frequency for the smoothing window (Hz).
/// @param f_max              Maximum frequency for the smoothing window (Hz). Defaults to sample_rate/2 if <= 0.
/// @returns A vector of size N where each element is the smoothed magnitude for the corresponding FFT bin.
/// @details
/// This function smooths the FFT magnitude spectrum by averaging the magnitudes within a logarithmically
/// spaced frequency window corresponding to the given fraction-of-an-octave. For each FFT bin:
///   - The center frequency is computed as f_center = i * (sample_rate / N).
///   - An initial smoothing window is defined as:
///         f_low  = f_center / (2^(fraction_of_octave/2))
///         f_high = f_center * (2^(fraction_of_octave/2))
///   - If the window exceeds the allowed frequency range [f_min, f_max], the boundaries are adjusted
///     to maintain the desired octave span (i.e. f_high / f_low ≈ 2^(fraction_of_octave)).
///   - A prefix sum of the FFT magnitudes is used to quickly compute the average magnitude over the window.
/// The output is a vector of the same size as the input FFT array, preserving the linear bin spacing.
inline Math::DFTCoefficients smooth_fft(const Math::DFTCoefficients &fft_coeffs, float sample_rate, float fraction_of_octave, float f_min = 20.0f, float f_max = 0.0f) {
    const size_t n = fft_coeffs.size();
    if (n == 0) { return {}; }

    // Default f_max to the Nyquist frequency if not specified.
    if (f_max <= 0.0f) { f_max = sample_rate / 2.0f; }

    // Compute the frequency resolution (bin width).
    const float bin_width = sample_rate / static_cast<float>(n);

    //--------------------------------------------------------------------------
    // 1) Compute the magnitude array.
    //    For each FFT coefficient, calculate:
    //       mag[i] = sqrt( real^2 + imag^2 )
    //--------------------------------------------------------------------------
    std::vector<float> mag(n);
    for (size_t i = 0; i < n; ++i) {
        float re = fft_coeffs[i].real();
        float im = fft_coeffs[i].imag();
        mag[i] = std::sqrt(re * re + im * im);
    }

    //--------------------------------------------------------------------------
    // 2) Build a prefix sum of magnitudes.
    //    Let prefix_mag[k] = sum(mag[0] ... mag[k-1]) with prefix_mag[0] = 0.
    //--------------------------------------------------------------------------
    std::vector<double> prefix_mag(n + 1, 0.0);
    for (size_t i = 0; i < n; ++i) {
        prefix_mag[i + 1] = prefix_mag[i] + mag[i];
    }

    //--------------------------------------------------------------------------
    // 3) For each FFT bin, define a log-based smoothing window and compute
    //    the average magnitude using the prefix sum.
    //--------------------------------------------------------------------------
    Math::DFTCoefficients smoothed(n, 0.f);

    // half_band_factor defines the half-width of the smoothing window:
    //   half_band_factor = 2^(fraction_of_octave/2)
    const float half_band_factor = std::pow(2.0f, fraction_of_octave / 2.0f);
    // desired_ratio is the overall frequency span of the window:
    //   desired_ratio = 2^(fraction_of_octave)
    const float desired_ratio = std::pow(2.0f, fraction_of_octave);

    for (size_t i = 0; i < n; ++i) {
        // Compute the center frequency for bin i.
        const float f_center = i * bin_width;

        // Skip bins outside the allowed frequency range.
        if (f_center < f_min || f_center > f_max) {
            smoothed[i] = 0.f;
            continue;
        }

        // Define the initial smoothing window.
        float f_low  = f_center / half_band_factor;
        float f_high = f_center * half_band_factor;

        // Adjust the window if it exceeds the allowed frequency range.
        if (f_high > f_max) {
            f_high = f_max;
            f_low  = f_high / desired_ratio;
        }
        if (f_low < f_min) {
            f_low  = f_min;
            f_high = f_low * desired_ratio;
            if (f_high > f_max) { f_high = f_max; }
        }

        // Map the window frequencies to FFT bin indices.
        size_t low_idx  = static_cast<size_t>(std::floor(f_low / bin_width));
        size_t high_idx = static_cast<size_t>(std::floor(f_high / bin_width));
        if (low_idx >= n) {
            smoothed[i] = 0.f;
            continue;
        }
        high_idx = std::min(high_idx, n - 1UL);

        // Compute the average magnitude in the window using the prefix sum.
        const size_t count = high_idx - low_idx + 1;
        const double sum_in_window = prefix_mag[high_idx + 1] - prefix_mag[low_idx];
        smoothed[i] = (count > 0) ? static_cast<float>(sum_in_window / count) : 0.f;
    }

    return smoothed;
}


}  // namespace Math

#endif  // DSP_MATH_FFT_HPP
