// Functions for calculating the Fast Fourier Transform (FFT) on-the-fly.
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_FFT_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_FFT_HPP_

#include <cmath>          // floor, ceil, abs, pow, log2
#include <cstddef>        // size_t
#include <algorithm>      // min, fill, copy
#include <complex>        // complex
#include <utility>        // swap
#include <vector>         // vector
#include "constants.hpp"  // M_PI, j<T>, etc.
#include "functions.hpp"  // complex_multiply, etc.
#include "window.hpp"     // window_function

/// @brief Basic mathematical functions.
namespace Math {

/// A type for DFT coefficient buffers.
typedef std::vector<std::complex<float>> DFTCoefficients;

/// A type for STFT coefficient matrices.
typedef std::vector<DFTCoefficients> STFTCoefficients;

/// @brief Linearly interpolate between coefficients.
/// @param coeff The coefficients to interpolate between.
/// @param index The floating point index to sample from the coefficients.
/// @returns The linearly interpolated coefficient at the given index.
static inline std::complex<float> interpolate_coefficients(
    const Math::DFTCoefficients& coeff,
    float index
) {
    int y0 = floorf(index);
    int y1 = ceilf(index);
    float alpha = index - y0;
    return (1 - alpha) * coeff[y0] + alpha * coeff[y1];
}

/// @brief Utility class for pre-computing twiddle factors for a radix-2 FFT.
/// @tparam T The type for the twiddle factors.
/// @details
/// This class pre-computes the complex exponential coefficients (twiddle
/// factors) used in the computation of the Fast Fourier Transform (FFT). For
/// an FFT of length \f$ N \f$, the twiddle factors are defined as:
/// \f[ W_k = e^{-i \frac{2\pi k}{N}}, \quad k = 0, 1, \dots, \frac{N}{2}-1. \f]
///
/// Only \f$ N/2 \f$ factors are stored due to the symmetry properties of the
/// FFT. Pre-computing these factors significantly improves the performance of
/// the FFT by avoiding repeated calls to expensive transcendental functions
/// during the transform computation.
template<typename T>
class TwiddleFactors {
 private:
    /// @brief Pre-computed twiddle factors.
    /// @details
    /// Only half the number of factors are stored because the FFT algorithm
    /// can exploit the symmetry in the complex exponentials, where the full
    /// set for an N-point FFT is:
    /// \f[ \{ W_0, W_1, \ldots, W_{(N/2)-1} \} \f]
    std::vector<std::complex<T>> factors;

 public:
    /// @brief Construct a twiddle factor table for an N-point FFT.
    /// @param n The length of the FFT. Ideally, n should be a power of 2.
    explicit TwiddleFactors(const size_t& n) { resize(n); }

    /// @brief Pre-compute the twiddle factor buffer for a new FFT length.
    /// @param n The new FFT length. Ideally, n should be a power of 2.
    inline void resize(const size_t& n) {
        // Resize the vector to store half the number of FFT points.
        factors.resize(n >> 1);
        // Calculate the angular step theta = -2*pi/n.
        const T theta = T(-2.0) * M_PI / n;
        for (size_t i = 0; i < factors.size(); ++i) {
            const T angle = theta * static_cast<T>(i);
            factors[i] = std::complex<T>(cos(angle), sin(angle));
        }
    }

    /// @brief Return the FFT size corresponding to the stored twiddle factors.
    /// @return The FFT length \f$N\f$, i.e., twice the number of factors.
    /// @details
    /// Since only \f$ N/2 \f$ twiddle factors are stored (to leverage
    /// symmetry), this method returns \f$ N = 2 \times (\text{number of
    /// stored factors}) \f$.
    inline size_t size() const { return factors.size() << 1; }

    /// @brief Accesse a pre-computed twiddle factor by index.
    /// @param i The index of the desired twiddle factor (0-based index).
    /// @return A constant reference to the \f$ i \f$th twiddle factor.
    /// @details
    /// The accessed twiddle factor is given by:
    /// \f[ W_i = e^{-i \frac{2\pi i}{N}}, \f]
    /// where \f$ N \f$ is the FFT length (returned by size()). It is
    /// important to note that valid indices range from \f$0\f$ to
    /// \f$ \frac{N}{2} - 1 \f$. Accessing an index outside this range leads
    /// to undefined behavior.
    const std::complex<T>& operator[](const size_t& i) const {
        return factors[i];
    }
};

/// @brief Pre-computed bit-reversal table for radix-2 FFT.
/// @details
/// This class pre-computes a table of bit-reversed indices for an FFT of
/// length \f$ N \f$. Bit-reversal is a crucial step in the radix-2 FFT
/// algorithm, where the input data is reordered according to the bit-reversed
/// order of their indices. This reordering allows the FFT algorithm to
/// access data in a cache-friendly pattern and perform the butterfly
/// computations efficiently.
///
/// For an index \f$ i \f$ (with \f$ 0 \leq i < N \f$), the bit-reversed index
/// is obtained by reversing the binary representation of \f$ i \f$. For
/// example, if \f$ N = 8 \f$ (i.e., using 3 bits), the bit reversal of
/// \f$ i = 3 \f$ (binary \f$ 011 \f$) is \f$ 110 \f$ (binary), which is 6 in
/// decimal.
class BitReversalTable {
 private:
    /// The pre-computed bit-reversal table.
    std::vector<size_t> table;

 public:
    /// @brief Construct a BitReversalTable for an \f$ N \f$-point FFT.
    /// @param n The length of the FFT (assumed that \f$n\f$ is a power of 2.)
    explicit BitReversalTable(const size_t& n) { resize(n); }

    /// @brief Pre-compute the bit-reversal table for an \f$ N \f$-point FFT.
    /// @param n The new FFT length (assumed that \f$n\f$ is a power of 2.)
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

    /// @brief Return the size of the FFT (number of indices in the table).
    /// @return The number of entries in the bit-reversal table.
    inline size_t size() const { return table.size(); }

    /// @brief Access the bit-reversed index at a given position.
    /// @param idx The original index (0-based) for which to retrieve the
    /// bit-reversed value.
    /// @return The bit-reversed index corresponding to \f$ idx \f$.
    /// @details
    /// The returned value is the bit-reversed representation of \f$ idx \f$
    /// for an FFT of the current size.
    const size_t& operator[](size_t idx) const { return table[idx]; }
};

/// @brief An on-the-fly implementation of the Cooley-Tukey iterative FFT.
/// @tparam T The type for the complex coefficients.
/// @details
/// The OnTheFlyFFT class provides an efficient implementation of the radix-2
/// Fast Fourier Transform (FFT) using pre-computed bit-reversal indices and
/// twiddle factors. It is designed for on-the-fly computation, allowing the
/// FFT to be computed incrementally (step-by-step) rather than in one
/// complete pass. This is particularly useful in streaming or real-time
/// applications.
///
/// The class encapsulates the state required for the Cooley-Tukey FFT
/// algorithm, including:
/// -   A pre-computed bit-reversal table to reorder the input samples.
/// -   Pre-computed twiddle factors for the butterfly computations.
/// -   Internal state variables (`step_`, `group`, and `pair`) to manage the
///     iterative FFT processing.
/// -   An internal coefficients buffer that holds both the input samples
///     (after windowing and bit-reversal) and the intermediate/final FFT
///     results.
template<typename T>
class OnTheFlyFFT {
 private:
    /// Pre-computed bit-reversal table for an N-point FFT.
    BitReversalTable bit_reversal;

    /// Pre-computed twiddle factors for an N-point FFT.
    TwiddleFactors<T> twiddles;

    /// The current step size in the Cooley-Tukey algorithm. Initially set to
    /// \f$2\f$, this variable doubles after completing each stage of the FFT.
    size_t step_ = 2;

    /// The current group offset within the coefficients buffer. Groups
    /// define the starting index for a set of butterfly computations in the
    /// current stage.
    size_t group = 0;

    /// The current pair offset within a group. This index tracks the
    /// position within a group for the current butterfly operation.
    size_t pair = 0;

    /// @brief Total number of steps needed to complete the FFT computation.
    /// @details
    /// Computed as:
    /// \f[
    /// \text{total\_steps} = \frac{N}{2} \times \log_2(N)
    /// \f]
    /// where \f$ N \f$ is the FFT length.
    size_t total_steps = 0;

 public:
    /// The coefficients buffer for the FFT computation.
    std::vector<std::complex<T>> coefficients;

    /// @brief Construct an OnTheFlyFFT object for an N-point FFT.
    /// @param n The length of the FFT. Must be a power of 2.
    explicit OnTheFlyFFT(const size_t& n) :
        bit_reversal(1), twiddles(1), coefficients(1) { resize(n); }

    /// @brief Resize and initialize the FFT computation structures.
    /// @param n The new FFT length. Must be a power of 2.
    inline void resize(const size_t& n) {
        bit_reversal.resize(n);
        twiddles.resize(n);
        coefficients.resize(n);
        total_steps = (n >> 1) * static_cast<size_t>(log2f(n));
        std::fill(coefficients.begin(), coefficients.end(), 0.f);
    }

    /// @brief Return the FFT length.
    /// @return The number of samples in the FFT.
    inline size_t size() const { return twiddles.size(); }

    /// @brief Return the number of steps required for the FFT computation.
    /// @return The total number of Cooley-Tukey steps needed.
    /// @details
    /// The total steps are computed as:
    /// \f[
    /// \text{total\_steps} = \frac{N}{2} \times \log_2(N)
    /// \f]
    inline size_t get_total_steps() const { return total_steps; }

    /// @brief Buffer input samples and prepare the FFT for computation.
    /// @param samples The input sample buffer of \f$N\f$ complex samples.
    /// @param window The window function samples to apply to the signal. May
    /// be an empty vector to indicate no window (i.e., a rectangular window.)
    inline void buffer(const std::complex<T>* x, const std::vector<T>& w = {}) {
        // Copy the samples into the coefficients buffer.
        std::copy(x, x + coefficients.size(), coefficients.begin());
        // Apply the window function to each sample.
        if (w.size() > 0) {
            for (size_t n = 0; n < coefficients.size(); ++n)
                coefficients[n] *= w[n];
        }
        // Perform bit-reversal permutation using the pre-computed table.
        for (size_t n = 0; n < coefficients.size(); ++n) {
            if (n < bit_reversal[n])
                std::swap(coefficients[n], coefficients[bit_reversal[n]]);
        }
        // Reset FFT state variables.
        step_ = 2;
        group = 0;
        pair = 0;
    }

    /// @brief Perform a single FFT computation step (butterfly operation.)
    inline void step() {
        if (is_done_computing()) return;
        // Calculate the half-step size and determine the twiddle factor stride.
        size_t half_step = step_ >> 1;
        size_t twiddle_stride = coefficients.size() / step_;
        // Retrieve the appropriate twiddle factor.
        auto w = twiddles[pair * twiddle_stride];
        // Perform the butterfly operation.
        auto even = coefficients[group + pair];
        auto odd = complex_multiply(coefficients[group + pair + half_step], w);
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

    /// @brief Perform a batch of FFT steps targeting a specified hop length.
    /// @param hop_lenth The number of samples between FFT computations.
    /// @details
    /// This method calculates the number of FFT steps to perform based on the
    /// hop length and the total number of steps required. It then iteratively
    /// calls the single-step() method, allowing the FFT computation to be
    /// spread across multiple processing intervals.
    inline void step(const size_t& hop_lenth) {
        auto steps = ceilf(get_total_steps() / static_cast<float>(hop_lenth));
        for (size_t i = 0; i < steps; i++) step();
    }

    /// @brief Checks whether the FFT computation has been completed.
    /// @return True if the FFT has been fully computed; false otherwise.
    /// @details
    /// The FFT computation is considered complete when the current step size
    /// exceeds the length of the coefficients buffer.
    inline bool is_done_computing() const {
        return step_ > coefficients.size();
    }

    /// @brief Complete the computation schedule of the FFT.
    inline void compute() { while (!is_done_computing()) step(); }
};

/// @brief An on-the-fly implementation of the Cooley-Tukey iterative RFFT.
/// @tparam T The type for the complex coefficients.
/// @details
/// The OnTheFlyRFFT class implements a Real FFT (RFFT) by leveraging an
/// underlying complex FFT of half the size (N/2) and pre-computed twiddle
/// factors. This approach packs the real input samples into a complex array,
/// computes the FFT on the packed data, and then reconstructs the full
/// N-point FFT using the symmetry properties of real signals.
template<typename T>
class OnTheFlyRFFT {
 private:
    /// The underlying N/2-point FFT used for the RFFT computation.
    OnTheFlyFFT<T> fft;
    /// Pre-computed twiddle factors for reconstructing the full N-point FFT.
    TwiddleFactors<T> twiddles;

    /// @brief Finalize the reconstruction of the N-point FFT from the
    /// underlying N/2-point FFT.
    /// @details
    /// This method reconstructs the full FFT coefficients for a real input
    /// signal by combining the results from the underlying complex FFT with
    /// pre-computed twiddle factors. It handles the special cases of the DC
    /// and Nyquist bins separately and reconstructs the remaining bins using
    /// symmetry properties.
    inline void finalize() {
        const auto N = size();    // Full FFT length (N)
        const size_t M = N >> 1;  // Half FFT length (M = N/2)
        // Handle DC (k = 0) and Nyquist (k = M) bins separately.
        T re0 = fft.coefficients[0].real();
        T im0 = fft.coefficients[0].imag();
        coefficients[0] = std::complex<T>(re0 + im0, T(0.0));
        coefficients[M] = std::complex<T>(re0 - im0, T(0.0));
        // Reconstruct FFT bins for 1 <= k < M.
        for (size_t k = 1; k < M; k++) {
            auto A = fft.coefficients[k];
            auto B = std::conj(fft.coefficients[M - k]);
            auto Wk = twiddles[k];  // Wk = exp(-j*2pi*k/N)
            // The below work-around addresses an incompatibility between
            // `std::complex` and SIMD primitive types. We replace:
            // auto Xk = T(0.5) * (A + B - j<T>() * Wk * (A - B));
            // with the broken down version:
            auto Xk = complex_multiply<T>(Wk, A - B);
                 Xk = complex_multiply<T>(Xk, j<T>());
                 Xk = A + B - Xk;
                 Xk = complex_multiply<T>(Xk, T(0.5));
            coefficients[k]     = Xk;
            coefficients[N - k] = std::conj(Xk);
        }
    }

 public:
    /// The output coefficients buffer containing the final FFT result.
    std::vector<std::complex<T>> coefficients;

    /// @brief Construct an OnTheFlyRFFT object for an N-point RFFT.
    /// @param n The length of the RFFT. Must be a power of 2.
    explicit OnTheFlyRFFT(const size_t& n) :
        fft(1), twiddles(1), coefficients(1) { resize(n); }

    /// @brief Resize and re-initialize the RFFT computation structures.
    /// @param n The new length of the RFFT. Must be a power of 2.
    inline void resize(const size_t& n) {
        fft.resize(n >> 1);
        twiddles.resize(n);
        coefficients.resize(n);
        std::fill(coefficients.begin(), coefficients.end(), 0.f);
    }

    /// @brief Return the length of the RFFT.
    /// @return The number of samples (N) in the RFFT.
    inline size_t size() const { return coefficients.size(); }

    /// @brief Return the total number of steps required to compute the FFT.
    /// @return The total number of butterfly operations needed by the FFT.
    inline size_t get_total_steps() const { return fft.get_total_steps(); }

    /// @brief Checks whether the RFFT computation has been completed.
    /// @returns True if the underlying FFT has been fully computed and the
    /// RFFT reconstruction is complete; false otherwise.
    inline bool is_done_computing() const { return fft.is_done_computing(); }

    /// @brief Buffer input samples and prepare the RFFT for computation.
    /// @param x A pointer to the real input sample buffer of length N.
    /// @param w A vector representing the window function to be applied
    /// to the input samples.
    inline void buffer(const T* x, const std::vector<float>& w) {
        // Create an array to interleave real samples into complex numbers.
        std::vector<std::complex<T>> packed(fft.size());
        for (size_t k = 0; k < packed.size(); ++k)
            packed[k] = {x[2 * k] * w[2 * k], x[2 * k + 1] * w[2 * k + 1]};
        // Since windowing is applied during packing, use no window for FFT.
        fft.buffer(packed.data());
    }

    /// @brief Perform a single RFFT computation step (butterfly operation.)
    /// @details
    /// This method advances the underlying FFT computation by one butterfly
    /// operation. Once the FFT computation is complete, it finalizes the
    /// reconstruction of the full FFT spectrum.
    inline void step() {
        if (is_done_computing()) return;
        fft.step();
        if (is_done_computing()) finalize();
    }

    /// @brief Perform a batch of FFT steps targeting a specified hop length.
    /// @param hop_length The number of samples between FFT computations.
    /// @details
    /// This method calculates the number of FFT steps to perform based on the
    /// hop length and the total number of steps required. It then iteratively
    /// calls the single-step() method, allowing the FFT computation to be
    /// spread across multiple processing intervals.
    inline void step(const size_t& hop_length) {
        auto steps = ceilf(get_total_steps() / static_cast<float>(hop_length));
        for (size_t i = 0; i < steps; i++) step();
    }

    /// @brief Complete the computation schedule of the FFT.
    inline void compute() { while (!is_done_computing()) step(); }

    /// @brief Perform in-place smoothing of the magnitude coefficients.
    /// @param sample_rate The sample rate (measured in Hz.)
    /// @param fraction_of_octave Fraction-of-an-octave for smoothing (e.g.,
    /// \f$1 = 1\mathrm{Oct.}\f$, \f$\frac{1}{6} = \frac{1}{6}\mathrm{Oct.}\f$).
    /// @details
    /// This method smooths the FFT magnitude spectrum over octave-based
    /// frequency bands. Instead of returning a new vector, it modifies the
    /// internal `coefficients` buffer directly, replacing each FFT
    /// coefficient with its smoothed magnitude (stored in the real part, with
    /// zero imaginary part).
    inline void smooth(float sample_rate, float fraction_of_octave) {
        // Ensure FFT coefficients exist.
        const size_t N = coefficients.size();
        if (N == 0) return;
        // Define the frequency range: from 0 Hz to the Nyquist frequency.
        const float f_max = sample_rate / 2.f;
        const float bin_width = sample_rate / static_cast<float>(N);
        // Build prefix sum of magnitudes.
        std::vector<T> cumsum(N + 1, 0.0);
        for (size_t n = 0; n < N; ++n)
            cumsum[n + 1] = cumsum[n] + abs(coefficients[n]);
        // Apply octave-based smoothing in-place.
        const float half_band_factor = powf(2.f, fraction_of_octave / 2.f);
        const float desired_ratio = powf(2.f, fraction_of_octave);
        for (size_t n = 0; n < N; ++n) {
            const float f_center = n * bin_width;
            // Skip bins outside the allowed frequency range.
            if (f_center > f_max) {
                coefficients[n] = std::complex<float>(0.f, 0.f);
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
            // Map frequencies to FFT bin indices.
            size_t low_idx  = std::floor(f_low / bin_width);
            size_t high_idx = std::floor(f_high / bin_width);
            if (low_idx >= N) {
                coefficients[n] = std::complex<float>(0.f, 0.f);
                continue;
            }
            high_idx = std::min(high_idx, N - 1);
            const size_t count = high_idx - low_idx + 1;
            const T sum = cumsum[high_idx + 1] - cumsum[low_idx];
            // Replace the coefficient with the smoothed magnitude.
            coefficients[n] = std::complex<T>(sum / count, T(0.0));
        }
    }
};

}  // namespace Math

#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_FFT_HPP_
