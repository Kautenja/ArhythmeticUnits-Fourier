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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>  // memcpy
#include <algorithm>
#include <complex>
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
///
/// This class precomputes a table of bit-reversed indices for an FFT of length \f$ N \f$.
/// Bit-reversal is a crucial step in the radix-2 FFT algorithm, where the input data is
/// reordered according to the bit-reversed order of their indices. This reordering allows
/// the FFT algorithm to access data in a cache-friendly pattern and perform the butterfly
/// computations efficiently.
///
/// For an index \f$ i \f$ (with \f$ 0 \leq i < N \f$), the bit-reversed index is obtained by
/// reversing the binary representation of \f$ i \f$. For example, if \f$ N = 8 \f$ (i.e., using 3 bits),
/// the bit reversal of \f$ i = 3 \f$ (binary \f$ 011 \f$) is \f$ 110 \f$ (binary), which is 6 in decimal.
///
/// @note It is assumed that \f$ N \f$ is a power of 2.
class BitReversalTable {
 private:
    /// @brief The pre-computed bit-reversal table.
    ///
    /// Each entry in this vector holds the bit-reversed index corresponding to its position.
    std::vector<size_t> table;

 public:
    /// @brief Constructs a BitReversalTable for an \f$ N \f$-point FFT.
    ///
    /// @param n The length of the FFT. It is assumed that \f$ n \f$ is a power of 2.
    ///
    /// The constructor precomputes the bit-reversal table by invoking the resize() method.
    explicit BitReversalTable(const size_t& n) {
        resize(n);
    }

    /// @brief Resizes and precomputes the bit-reversal table for an \f$ N \f$-point FFT.
    ///
    /// @param n The new FFT length. It is assumed that \f$ n \f$ is a power of 2.
    ///
    /// This method performs the following steps:
    /// 1. Resizes the internal vector to hold \f$ n \f$ entries.
    /// 2. Computes the number of bits required to represent the indices, which is
    ///    \f$ \log_2(n) \f$.
    /// 3. For each index \f$ i \f$ in the range [0, \f$ n-1 \f$]:
    ///    - Initializes a temporary variable \f$ y \f$ to 0.
    ///    - Iteratively extracts the least significant bit of \f$ i \f$ and
    ///      appends it to \f$ y \f$, effectively reversing the bit order.
    ///    - Stores the computed bit-reversed value in the table.
    inline void resize(const size_t& n) {
        table.resize(n);
        // Determine the number of bits required to represent indices 0 to n-1.
        size_t log2n = static_cast<size_t>(log2f(n));
        for (size_t i = 0; i < n; ++i) {
            size_t y = 0;
            size_t x = i;
            // Reverse the bits of i.
            for (size_t j = 0; j < log2n; ++j) {
                y = (y << 1) | (x & 1);
                x >>= 1;
            }
            table[i] = y;
        }
    }

    /// @brief Returns the size of the FFT (i.e., the number of indices in the table).
    ///
    /// @return The FFT length, which is also the number of entries in the bit-reversal table.
    inline size_t size() const { return table.size(); }

    /// @brief Accesses the bit-reversed index at a given position.
    ///
    /// @param idx The original index (0-based) for which to retrieve the bit-reversed value.
    /// @return A constant reference to the bit-reversed index corresponding to \f$ idx \f$.
    ///
    /// The returned value is the bit-reversed representation of \f$ idx \f$ for an FFT of
    /// the current size.
    const size_t& operator[](size_t idx) const { return table[idx]; }
};

/// @brief An FFT computation utility based on pre-computed twiddle factors.
///
/// The OnTheFlyFFT class provides an efficient implementation of the radix-2 Fast Fourier Transform (FFT)
/// using pre-computed bit-reversal indices and twiddle factors. It is designed for on-the-fly computation,
/// allowing the FFT to be computed incrementally (step-by-step) rather than in one complete pass. This is
/// particularly useful in streaming or real-time applications.
///
/// The class encapsulates the state required for the Cooley-Tukey FFT algorithm, including:
/// - A pre-computed bit-reversal table to reorder the input samples.
/// - Pre-computed twiddle factors for the butterfly computations.
/// - Internal state variables (`step_`, `group`, and `pair`) to manage the iterative FFT processing.
/// - An internal coefficients buffer that holds both the input samples (after windowing and bit-reversal)
///   and the intermediate/final FFT results.
///
/// @note The FFT length (number of samples) must be a power of 2.
class OnTheFlyFFT {
 private:
    /// @brief Pre-computed bit-reversal table for an N-point FFT.
    ///
    /// This table is used to rearrange the input samples into bit-reversed order prior to
    /// performing the FFT computation.
    BitReversalTable bit_reversal{1};

    /// @brief Pre-computed twiddle factors for an N-point FFT.
    ///
    /// These are the complex coefficients used in the butterfly operations of the FFT.
    TwiddleFactors twiddles{1};

    /// @brief The current step size in the Cooley-Tukey algorithm.
    ///
    /// Initially set to 2, this variable doubles after completing each stage of the FFT.
    size_t step_ = 2;

    /// @brief The current group offset within the coefficients buffer.
    ///
    /// Groups define the starting index for a set of butterfly computations in the current stage.
    size_t group = 0;

    /// @brief The current pair offset within a group.
    ///
    /// This index tracks the position within a group for the current butterfly operation.
    size_t pair = 0;

    /// @brief The total number of steps needed to complete the FFT computation.
    ///
    /// Computed as:
    /// \f[
    /// \text{total\_steps} = \frac{N}{2} \times \log_2(N)
    /// \f]
    /// where \f$ N \f$ is the FFT length.
    size_t total_steps = 0;

 public:
    /// @brief The coefficients buffer for the FFT computation.
    ///
    /// This vector holds the input samples (after windowing and bit-reversal) and is updated in-place
    /// during the FFT computation. Once the computation is complete, this buffer contains the frequency
    /// domain representation of the input signal.
    std::vector<std::complex<float>> coefficients{1};

    /// @brief Constructs an OnTheFlyFFT object for an N-point FFT.
    ///
    /// @param n The length of the FFT. Must be a power of 2.
    ///
    /// The constructor initializes the pre-computed bit-reversal table, twiddle factors,
    /// and the coefficients buffer to accommodate \f$ N \f$ samples.
    explicit OnTheFlyFFT(const size_t& n) { resize(n); }

    /// @brief Resizes and initializes the FFT computation structures.
    ///
    /// @param n The new FFT length. Must be a power of 2.
    ///
    /// This method resizes the bit-reversal table, twiddle factors, and coefficients buffer.
    /// It also recalculates the total number of FFT computation steps and resets the coefficients
    /// buffer to zero.
    inline void resize(const size_t& n) {
        bit_reversal.resize(n);
        twiddles.resize(n);
        coefficients.resize(n);
        total_steps = (n >> 1) * static_cast<size_t>(log2f(n));
        std::fill(coefficients.begin(), coefficients.end(), 0.f);
    }

    /// @brief Returns the FFT length.
    ///
    /// @return The number of samples in the FFT.
    ///
    /// This value is determined by the size of the twiddle factors (which are pre-computed for an N-point FFT).
    inline size_t size() const { return twiddles.size(); }

    /// @brief Returns the total number of steps required for the FFT computation.
    ///
    /// @return The total number of Cooley-Tukey steps needed.
    ///
    /// The total steps are computed as:
    /// \f[
    /// \text{total\_steps} = \frac{N}{2} \times \log_2(N)
    /// \f]
    inline size_t get_total_steps() const { return total_steps; }

    /// @brief Buffers input samples and prepares the FFT for on-the-fly computation.
    ///
    /// @param samples A pointer to the input sample buffer containing \f$ N \f$ complex samples.
    /// @param window A vector representing the window function to be applied to the samples.
    ///
    /// This method performs the following operations:
    /// 1. Copies the input samples into the internal coefficients buffer.
    /// 2. Applies the provided window function to each sample.
    /// 3. Reorders the samples using a bit-reversal permutation based on the pre-computed table.
    /// 4. Resets the internal state variables (`step_`, `group`, and `pair`) for the FFT computation.
    inline void buffer(
        const std::complex<float>* samples,
        const std::vector<float>& window
    ) {
        const auto N = coefficients.size();
        // Copy the samples into the coefficients buffer.
        memcpy(coefficients.data(), samples, N * sizeof(std::complex<float>));
        // Apply the window function to each sample.
        for (size_t n = 0; n < N; ++n)
            coefficients[n] *= window[n];
        // Perform bit-reversal permutation using the pre-computed bit-reversal table.
        for (size_t n = 0; n < N; ++n)
            if (n < bit_reversal[n])
                std::swap(coefficients[n], coefficients[bit_reversal[n]]);
        // Reset FFT state variables.
        step_ = 2;
        group = 0;
        pair = 0;
    }

    /// @brief Performs a single FFT computation step (butterfly operation).
    ///
    /// This method executes one butterfly operation of the Cooley-Tukey FFT algorithm. It:
    /// 1. Determines the current half-step size and twiddle factor stride.
    /// 2. Retrieves the appropriate twiddle factor from the pre-computed table.
    /// 3. Computes the butterfly (combining an even-indexed element with an odd-indexed element).
    /// 4. Updates the internal coefficients buffer in-place.
    /// 5. Advances the internal state (`pair`, `group`, and `step_`) for the next computation.
    inline void step() {
        if (is_done_computing()) return;
        // Calculate the half-step size and determine the twiddle factor stride.
        const size_t half_step = step_ >> 1;
        const size_t twiddle_stride = coefficients.size() / step_;
        // Retrieve the appropriate twiddle factor.
        const auto w = twiddles[pair * twiddle_stride];
        // Perform the butterfly operation.
        const auto even = coefficients[group + pair];
        const auto odd = coefficients[group + pair + half_step] * w;
        coefficients[group + pair] = even + odd;
        coefficients[group + pair + half_step] = even - odd;
        // Update the FFT state variables.
        pair = pair + 1;
        if (pair >= half_step) {
            pair = 0;
            group = group + step_;
            if (group >= coefficients.size()) {
                group = 0;
                step_ <<= 1;  // Double the step size for the next stage.
            }
        }
    }

    /// @brief Performs a batch of FFT steps targeting a specified hop length.
    ///
    /// @param hop_length The number of samples between FFT computations.
    ///
    /// This method calculates the number of FFT steps to perform based on the hop length and
    /// the total number of steps required. It then iteratively calls the single-step() method,
    /// allowing the FFT computation to be spread across multiple processing intervals.
    inline void step(const size_t& hop_length) {
        const auto num_steps = ceilf(get_total_steps() / static_cast<float>(hop_length));
        for (size_t i = 0; i < num_steps; i++) {
            step();
        }
    }

    /// @brief Checks whether the FFT computation has been completed.
    ///
    /// @return True if the FFT has been fully computed; false otherwise.
    ///
    /// The FFT computation is considered complete when the current step size exceeds
    /// the length of the coefficients buffer.
    inline bool is_done_computing() const {
        return step_ > coefficients.size();
    }
};

/// @brief An RFFT computation utility based on pre-computed twiddle factors.
///
/// The OnTheFlyRFFT class implements a Real FFT (RFFT) by leveraging an underlying
/// complex FFT of half the size (N/2) and pre-computed twiddle factors. This approach
/// packs the real input samples into a complex array, computes the FFT on the packed data,
/// and then reconstructs the full N-point FFT using the symmetry properties of real signals.
///
/// @note The RFFT length (N) must be a power of 2.
class OnTheFlyRFFT {
 private:
    /// @brief The underlying N/2-point FFT used for the RFFT computation.
    ///
    /// The complex FFT is applied on a packed representation of the real input,
    /// reducing the computation by half while leveraging the symmetry in real signals.
    OnTheFlyFFT fft{1};

    /// @brief Pre-computed twiddle factors used for reconstructing the full N-point FFT.
    ///
    /// These twiddle factors, defined as \f$ W_k = e^{-j\frac{2\pi k}{N}} \f$, are used in the
    /// reconstruction process to combine the FFT results of the even and odd parts of the signal.
    TwiddleFactors twiddles{1};

 public:
    /// @brief The output coefficients buffer containing the final FFT result.
    ///
    /// This vector holds the frequency-domain representation (complex spectrum) of the real input
    /// signal after the reconstruction process. Its size is N.
    std::vector<std::complex<float>> coefficients{1};

    /// @brief Constructs an OnTheFlyRFFT object for an N-point RFFT.
    ///
    /// @param n The length of the RFFT. Must be a power of 2.
    ///
    /// The constructor initializes the underlying N/2-point FFT, the twiddle factors for
    /// reconstruction, and the output coefficients buffer.
    explicit OnTheFlyRFFT(const size_t& n) { resize(n); }

    /// @brief Resizes and reinitializes the RFFT computation structures.
    ///
    /// @param n The new length of the RFFT. Must be a power of 2.
    ///
    /// This method resizes the underlying FFT, twiddle factors, and the coefficients buffer.
    /// It also resets the coefficients to zero.
    inline void resize(const size_t& n) {
        fft.resize(n >> 1);
        twiddles.resize(n);
        coefficients.resize(n);
        std::fill(coefficients.begin(), coefficients.end(), 0.f);
    }

    /// @brief Returns the length of the RFFT.
    ///
    /// @return The number of samples (N) in the RFFT.
    inline size_t size() const {
        return coefficients.size();
    }

    /// @brief Returns the total number of steps required to compute the underlying FFT.
    ///
    /// @return The total number of butterfly operations needed by the underlying FFT.
    inline size_t get_total_steps() const {
        return fft.get_total_steps();
    }

    /// @brief Buffers input samples and prepares the RFFT for on-the-fly computation.
    ///
    /// @param samples A pointer to the real input sample buffer of length N.
    /// @param window A vector representing the window function to be applied to the input samples.
    ///
    /// This method performs the following operations:
    /// 1. Packs the real input samples into a temporary complex array. Two consecutive samples
    ///    (one for the real part and one for the imaginary part) form a complex number.
    /// 2. Applies the provided window function to the samples during the packing process.
    /// 3. Buffers the packed data into the underlying FFT using a unity window since the window
    ///    has already been applied.
    inline void buffer(const float* samples, const std::vector<float>& window) {
        const size_t M = size() >> 1;  // M = N/2, the size of the underlying FFT.
        // Create a temporary array to pack the samples into complex numbers.
        std::vector<std::complex<float>> packed(M);
        for (size_t k = 0; k < M; ++k) {
            float r = samples[2 * k]     * window[2 * k];
            float i = samples[2 * k + 1] * window[2 * k + 1];
            packed[k] = std::complex<float>(r, i);
        }
        // Since the window function has been applied during packing, use a unity window for FFT.
        std::vector<float> unity(M, 1.0f);
        fft.buffer(packed.data(), unity);
    }

    /// @brief Performs a single step of the RFFT computation.
    ///
    /// This method advances the underlying FFT computation by one butterfly operation. Once the
    /// FFT computation is complete, it finalizes the reconstruction of the full FFT spectrum.
    inline void step() {
        if (fft.is_done_computing())
            return;
        fft.step();
        if (fft.is_done_computing())
            finalize();
    }

    /// @brief Performs a batch of FFT steps targeting a specified hop length.
    ///
    /// @param hop_length The number of samples between FFT computations.
    ///
    /// This method calculates the number of FFT steps to perform based on the hop length and then
    /// iteratively calls the single-step method, allowing the RFFT computation to be distributed
    /// across multiple processing intervals.
    inline void step(const size_t& hop_length) {
        const auto num_steps = ceilf(get_total_steps() / static_cast<float>(hop_length));
        for (size_t i = 0; i < num_steps; i++) {
            step();
        }
    }

    /// @brief Checks whether the RFFT computation has been completed.
    ///
    /// @return True if the underlying FFT has been fully computed and the RFFT reconstruction
    ///         is complete; false otherwise.
    inline bool is_done_computing() const {
        return fft.is_done_computing();
    }

    /// @brief Finalizes the reconstruction of the N-point FFT from the underlying N/2-point FFT.
    ///
    /// This method reconstructs the full FFT coefficients for a real input signal by combining
    /// the results from the underlying complex FFT with pre-computed twiddle factors.
    /// It handles the special cases of the DC and Nyquist bins separately and reconstructs the
    /// remaining bins using symmetry properties.
    inline void finalize() {
        const auto N = size();    // Full FFT length (N)
        const size_t M = N >> 1;  // Half FFT length (M = N/2)
        // Handle DC (k = 0) and Nyquist (k = M) bins separately.
        float re0 = fft.coefficients[0].real();
        float im0 = fft.coefficients[0].imag();
        coefficients[0] = std::complex<float>(re0 + im0, 0.0f);
        coefficients[M] = std::complex<float>(re0 - im0, 0.0f);
        // Reconstruct FFT bins for 1 <= k < M.
        for (size_t k = 1; k < M; k++) {
            std::complex<float> A = fft.coefficients[k];
            std::complex<float> B = std::conj(fft.coefficients[M - k]);
            std::complex<float> Wk = twiddles[k]; // Wk = exp(-j*2pi*k/N)
            std::complex<float> Xk = 0.5f * (A + B - std::complex<float>(0, 1) * Wk * (A - B));
            coefficients[k]     = Xk;
            coefficients[N - k] = std::conj(Xk);
        }
    }

    /**
     * @brief Perform in-place smoothing of the FFT magnitudes.
     *
     * This method smooths the FFT magnitude spectrum over octave-based frequency bands.
     * Instead of returning a new vector, it modifies the internal `coefficients` buffer directly,
     * replacing each FFT coefficient with its smoothed magnitude (stored in the real part, with zero imaginary part).
     *
     * @param sample_rate        Sampling frequency in Hz.
     * @param fraction_of_octave Fraction-of-an-octave for smoothing (e.g. 1.0 = 1 octave, 1/6 = 1/6 octave).
     * @param f_min              Minimum frequency for the smoothing window (Hz).
     * @param f_max              Maximum frequency for the smoothing window (Hz). Defaults to sample_rate/2 if <= 0.
     */
    inline void smooth(float sample_rate, float fraction_of_octave, float f_min = 20.0f, float f_max = 0.0f) {
        // Ensure FFT coefficients exist.
        const size_t n = coefficients.size();
        if (n == 0) { return; }
        if (f_max <= 0.0f) { f_max = sample_rate / 2.0f; }
        const float bin_width = sample_rate / static_cast<float>(n);

        // 1) Compute magnitude array (temporary storage).
        std::vector<float> mag(n);
        for (size_t i = 0; i < n; ++i) {
            float re = coefficients[i].real();
            float im = coefficients[i].imag();
            mag[i] = std::sqrt(re * re + im * im);
        }

        // 2) Build prefix sum of magnitudes.
        std::vector<double> prefix_mag(n + 1, 0.0);
        for (size_t i = 0; i < n; ++i) {
            prefix_mag[i + 1] = prefix_mag[i] + mag[i];
        }

        // 3) Apply octave-based smoothing in-place.
        const float half_band_factor = std::pow(2.0f, fraction_of_octave / 2.0f);
        const float desired_ratio = std::pow(2.0f, fraction_of_octave);

        for (size_t i = 0; i < n; ++i) {
            const float f_center = i * bin_width;
            // Skip bins outside the allowed frequency range.
            if (f_center < f_min || f_center > f_max) {
                coefficients[i] = std::complex<float>(0.0f, 0.0f);
                continue;
            }
            // Define the initial smoothing window.
            float f_low  = f_center / half_band_factor;
            float f_high = f_center * half_band_factor;
            // Adjust window boundaries if necessary.
            if (f_high > f_max) {
                f_high = f_max;
                f_low  = f_high / desired_ratio;
            }
            if (f_low < f_min) {
                f_low  = f_min;
                f_high = f_low * desired_ratio;
                if (f_high > f_max) { f_high = f_max; }
            }
            // Map frequencies to FFT bin indices.
            size_t low_idx  = static_cast<size_t>(std::floor(f_low / bin_width));
            size_t high_idx = static_cast<size_t>(std::floor(f_high / bin_width));
            if (low_idx >= n) {
                coefficients[i] = std::complex<float>(0.0f, 0.0f);
                continue;
            }
            high_idx = std::min(high_idx, n - 1UL);
            const size_t count = high_idx - low_idx + 1;
            const double sum_in_window = prefix_mag[high_idx + 1] - prefix_mag[low_idx];
            float avg = (count > 0) ? static_cast<float>(sum_in_window / count) : 0.0f;
            // Replace the FFT coefficient with the smoothed magnitude.
            coefficients[i] = std::complex<float>(avg, 0.0f);
        }
    }
};

}  // namespace Math

#endif  // DSP_MATH_FFT_HPP
