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

/// @brief Compute the Fast Fourier Transform (FTT) in-place.
///
/// @tparam N the length of the input sequence.
/// @param input the input vector to compute the FFT of, \f$x[n]\f$
/// @param window a windowing function to use when calculating coefficients.
///
template<int N>
void fft_(
    std::vector<std::complex<float>>& input,
    const Window::Function& window = Window::Function::Boxcar
) {
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");
    std::vector<std::complex<float>> even(N / 2), odds(N / 2);
    for (int i = 0; i < N / 2; ++i) {
        even[i] = Window::window<float>(window, 2*i, N, false) * input[2*i] / Window::coherent_gain(window);
        odds[i] = Window::window<float>(window, 2*i+1, N, false) * input[2*i+1] / Window::coherent_gain(window);
    }
    fft_<N / 2>(even);
    fft_<N / 2>(odds);
    for (int i = 0; i < N / 2; ++i) {
        std::complex<float> w = exp(std::complex<float>(0, -2 * pi<float>() * i / N)) * odds[i];
        input[i] = even[i] + w;
        input[i + N / 2] = even[i] - w;
    }
}

// The base case of a 1-point FFT is a NoOp.
template<>
inline void fft_<1>(std::vector<std::complex<float>>&, const Window::Function&) { }

/// @brief Pre-computed twiddle factors for radix-2 FFT.
class TwiddleFactors {
 private:
    /// The pre-computed twiddle factors.
    std::vector<std::complex<float>> factors;

 public:
    /// @brief Initialize a twiddle factor set for an N-point FFT.
    /// @param n The length of the FFT.
    explicit TwiddleFactors(const size_t& n) { resize(n); }

    /// @brief Resize the twiddle factor buffer.
    /// @param n The length of the FFT.
    inline void resize(const size_t& n) {
        factors.resize(n >> 1);  // bit-wise "/ 2"
        // NOTE: This old implementation results in many calls to exp.
        // for (size_t i = 0; i < factors.size(); ++i)
        //     factors[i] = std::exp(std::complex<float>(0, -2.0f * M_PI * i / n));
        // Pre-compute the multiplier: e^{-i * 2pi/n}.
        const float theta = -2.0f * M_PI / n;
        const std::complex<float> multiplier(std::cos(theta), std::sin(theta));
        std::complex<float> current(1.0f, 0.0f);
        for (size_t i = 0; i < factors.size(); ++i) {
            factors[i] = current;
            current *= multiplier;
        }
    }

    /// @brief Return the length of the FFT.
    inline size_t size() const {
        return factors.size() << 1;  // bit-wise "* 2"
    }

    /// @brief Return the twiddle factor at the given index.
    /// @param i The index of the twiddle factor to access.
    /// @returns The twiddle factor at the given index.
    /// @details
    /// Twiddle factors are computed as
    /// \f$W_k = e^{\frac{-i * 2 * \pi * k}{N}}\f$
    const std::complex<float>& operator[](const size_t& i) const {
        return factors[i];
    }
};

/// @brief Precomputed bit-reversal table for radix-2 FFT.
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
    /// @param i The index of the bit-reversal index to access.
    /// @returns The bit-reversal index at the given index.
    const size_t& operator[](size_t i) const { return table[i]; }
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
/// @param fftCoeffs        Full FFT array, size=N.
/// @param sampleRate       Sampling frequency (Hz).
/// @param fractionOfOctave Fraction-of-an-octave (e.g. 1.0 = 1-octave, 1/6 = 1/6-octave, etc.).
/// @param fMin             Minimum frequency for smoothing window (Hz).
/// @param fMax             Maximum frequency for smoothing window (Hz). Defaults to sampleRate if <=0.
/// @returns A vector of size N, each element is the smoothed magnitude for that bin.
/// @details
/// This function preserves linear spacing in the output (size = N).
///
/// This version:
///   - Expects a full FFT array of size N (complex).
///   - f[i] = i*(sampleRate/N) for i=0..N-1 (unshifted).
///   - Uses prefix sums + binary search => O(N log N).
///
inline Math::DFTCoefficients smooth_fft(
    const Math::DFTCoefficients &fftCoeffs,
    float sampleRate,
    float fractionOfOctave,
    float fMin = 20.0f,
    float fMax = 0.0f)
{
    const size_t N = fftCoeffs.size();
    if (N == 0)
        return {};

    // Set fMax to sampleRate if not specified.
    if (fMax <= 0.0f)
        fMax = sampleRate;

    // Each FFT bin spans this much frequency.
    const float binWidth = sampleRate / static_cast<float>(N);

    // Compute magnitudes.
    std::vector<float> mag(N);
    for (size_t i = 0; i < N; ++i) {
        float re = fftCoeffs[i].real();
        float im = fftCoeffs[i].imag();
        mag[i] = std::sqrt(re * re + im * im);
    }

    // Build prefix sum: prefixMag[k] = sum_{i=0}^{k-1} mag[i]
    std::vector<double> prefixMag(N + 1, 0.0);
    for (size_t i = 0; i < N; ++i)
        prefixMag[i + 1] = prefixMag[i] + mag[i];

    // Prepare output vector.
    Math::DFTCoefficients smoothed(N, 0.f);

    // halfBandFactor = 2^(fractionOfOctave/2)
    const float halfBandFactor = std::pow(2.0f, fractionOfOctave / 2.0f);

    // Helper lambda to clamp a frequency to [fMin, fMax]
    auto clampFreq = [fMin, fMax](float f) -> float {
        return std::max(fMin, std::min(f, fMax));
    };

    // Process each bin.
    for (size_t i = 0; i < N; ++i) {
        // Compute center frequency directly from bin index.
        float fCenter = i * binWidth;

        // Skip bins outside the desired frequency range.
        if (fCenter < fMin || fCenter > fMax) {
            smoothed[i] = 0.f;
            continue;
        }

        // Define the smoothing window (logarithmic scale).
        float fLow  = clampFreq(fCenter / halfBandFactor);
        float fHigh = clampFreq(fCenter * halfBandFactor);

        // Map the window edges to FFT bin indices.
        size_t lowIdx  = static_cast<size_t>(std::floor(fLow  / binWidth));
        size_t highIdx = static_cast<size_t>(std::floor(fHigh / binWidth));
        if (lowIdx >= N) {
            smoothed[i] = 0.f;
            continue;
        }
        highIdx = std::min(highIdx, N - 1UL);

        // Average the magnitudes in [lowIdx, highIdx] using the prefix sum.
        const size_t count = highIdx - lowIdx + 1;
        const double sum = prefixMag[highIdx + 1] - prefixMag[lowIdx];
        smoothed[i] = static_cast<float>(sum / count);
    }

    return smoothed;
}

}  // namespace Math

#endif  // DSP_MATH_FFT_HPP
