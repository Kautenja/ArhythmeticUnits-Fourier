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

template<typename T>
std::vector<std::complex<T>> real_to_complex(const std::vector<T>& real_vec) {
    std::vector<std::complex<T>> complex_vec(real_vec.size());
    std::transform(real_vec.begin(), real_vec.end(), complex_vec.begin(), [](T x) { return std::complex<T>(x, 0); });
    return complex_vec;
}

template<typename T>
std::vector<T> complex_to_real(const std::vector<std::complex<T>>& complex_vec) {
    std::vector<T> real_vec(complex_vec.size());
    std::transform(complex_vec.begin(), complex_vec.end(), real_vec.begin(), [](const std::complex<T>& c) { return c.real(); });
    return real_vec;
}

// ---------------------------------------------------------------------------
// MARK: FFT
// ---------------------------------------------------------------------------

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

/// @brief Compute the Fast Fourier Transform (FFT).
///
/// @tparam N the length of the input sequence / number of coefficients.
/// @param input the input vector to compute the FFT of, \f$x[n]\f$
/// @param window a windowing function to use when calculating coefficients.
/// @returns a vector containing the DFT coefficients, \f$X[k]\f$
///
template<int N>
std::vector<std::complex<float>> fft(
    const std::vector<float>& input,
    const Window::Function& window = Window::Function::Boxcar
) {
    auto output = real_to_complex(input);
    fft_<N>(output, window);
    return output;
}

/// @brief Compute the Fast Fourier Transform (FTT) in-place.
///
/// @param input the input vector to compute the FFT of, \f$x[n]\f$
/// @param n the length of the input sequence.
/// @param window a windowing function to use when calculating coefficients.
///
inline void fft_n(
    std::vector<std::complex<float>>& input,
    const size_t& n,
    const Window::Function& window = Window::Function::Boxcar
) {
    if (n == 1) return;
    std::vector<std::complex<float>> even(n / 2), odds(n / 2);
    for (size_t i = 0; i < n / 2; ++i) {
        even[i] = Window::window<float>(window, 2*i, n, false) * input[2*i] / Window::coherent_gain(window);
        odds[i] = Window::window<float>(window, 2*i+1, n, false) * input[2*i+1] / Window::coherent_gain(window);
    }
    fft_n(even, n / 2);
    fft_n(odds, n / 2);
    for (size_t i = 0; i < n / 2; ++i) {
        std::complex<float> w = exp(std::complex<float>(0, -2 * pi<float>() * i / static_cast<float>(n))) * odds[i];
        input[i] = even[i] + w;
        input[i + n / 2] = even[i] - w;
    }
}

// /// @brief Compute the Fast Fourier Transform (FFT).
// ///
// /// @param input the input vector to compute the FFT of, \f$x[n]\f$
// /// @tparam n the length of the input sequence / number of coefficients.
// /// @param window a windowing function to use when calculating coefficients.
// /// @returns a vector containing the DFT coefficients, \f$X[k]\f$
// ///
// std::vector<std::complex<float>> fft(
//     const std::vector<float>& input,
//     const size_t& n,
//     const Window::Function& window = Window::Function::Boxcar
// ) {
//     auto output = real_to_complex(input);
//     fft_n(output, n, window);
//     return output;
// }




/// @brief Compute the Fast Fourier Transform (FTT) in-place.
///
/// @param input the input vector to compute the FFT of, \f$x[n]\f$
/// @param n the length of the input sequence.
/// @param window a windowing function to use when calculating coefficients.
///
inline void fft_inplace(
    std::complex<float>* input,
    const size_t& n,
    const Window::Function& window = Window::Function::Boxcar
) {
    // Input signal windowing.
    for (size_t i = 0; i < n; ++i)
        input[i] *= Window::window<float>(window, i, n, false) / Window::coherent_gain(window);
    // Bit-reversal permutation.
    size_t j = 0;
    for (size_t i = 0; i < n; ++i) {
        if (i < j) {
            std::swap(input[i], input[j]);
        }
        size_t mask = n >> 1;
        while (j >= mask && mask > 0) {
            j -= mask;
            mask >>= 1;
        }
        j += mask;
    }
    // Iterative Cooley-Tukey FFT.
    for (size_t step = 2; step <= n; step <<= 1) {
        size_t half_step = step / 2;
        std::complex<float> w_step = std::exp(std::complex<float>(0, -2.0f * M_PI / step));
        for (size_t group = 0; group < n; group += step) {
            std::complex<float> w = 1.0f;
            for (size_t pair = 0; pair < half_step; ++pair) {
                std::complex<float> even = input[group + pair];
                std::complex<float> odd = input[group + pair + half_step] * w;
                input[group + pair] = even + odd;
                input[group + pair + half_step] = even - odd;
                w *= w_step;
            }
        }
    }
}




/// @brief Precomputed twiddle factors for radix-2 FFT.
class TwiddleFactors {
 private:
    /// The pre-computed twiddle factors.
    std::vector<std::complex<float>> factors;

 public:
    /// @brief Initialize a twiddle factor set for an N-point FFT.
    /// @param n The length of the FFT.
    explicit TwiddleFactors(const size_t& n) { resize(n); }

    // /// @brief Resize the twiddle factor buffer.
    // /// @param n The length of the FFT.
    // inline void resize(const size_t& n) {
    //     factors.resize(n >> 1);  // n / 2
    //     for (size_t i = 0; i < factors.size(); ++i)
    //         factors[i] = std::exp(std::complex<float>(0, -2.0f * M_PI * i / n));
    // }

    /// @brief Resize the twiddle factor buffer.
    /// @param n The length of the FFT.
    inline void resize(const size_t& n) {
        factors.resize(n >> 1);  // bit-wise "/ 2"
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
    /// @brief Pre-computed twiddle factors for an N-point FFT.
    TwiddleFactors twiddles;
    /// @brief Pre-computed bit-reversal table for an N-point FFT.
    BitReversalTable bit_reversal;

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
    OnTheFlyFFT(const size_t& n) : twiddles(n), bit_reversal(n), coefficients(n) { }

    /// @brief Resize the FFT.
    /// @param n The length of the FFT.
    inline void resize(const size_t& n) {
        twiddles.resize(n);
        bit_reversal.resize(n);
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


















/// @brief An FFT computation utility based on pre-computed Twiddle factors.
class FFT {
 private:
    /// @brief Pre-computed twiddle factors for an N-point FFT.
    TwiddleFactors twiddles;

    // std::vector<size_t> bit_reversal_table;

    // /// @brief Generate the bit-reversal table for the current FFT size.
    // void generate_bit_reversal_table(const size_t& n) {
    //     bit_reversal_table.resize(n);
    //     size_t log2n = static_cast<size_t>(log2f(n));
    //     for (size_t i = 0; i < n; ++i) {
    //         size_t y = 0;
    //         size_t x = i;
    //         for (size_t j = 0; j < log2n; ++j) {
    //             y = (y << 1) | (x & 1);
    //             x >>= 1;
    //         }
    //         bit_reversal_table[i] = y;
    //     }
    // }

 public:
    /// @brief Initialize the FFT
    /// @param n The length of the FFT.
    explicit FFT(const size_t& n) : twiddles(n) {
        // generate_bit_reversal_table(n);
    }

    /// @brief Resize the FFT.
    /// @param n The length of the FFT.
    inline void resize(const size_t& n) {
        twiddles.resize(n);
        // generate_bit_reversal_table(n);
    }

    /// @brief Return the length of the FFT.
    inline size_t size() const { return twiddles.size(); }

    /// @brief Compute the DFT using the iterative Cooley-Tukey algorithm.
    /// @param input The input signal to compute the DFT coefficients of.
    /// @param window The window function to use before computing the DFT.
    inline void operator()(std::complex<float>* input,
        const Window::Function& window = Window::Function::Boxcar
    ) {
        // Determine the length of the FFT from the Twiddle factor set.
        const auto n = twiddles.size();
        // Input signal windowing.
        for (size_t i = 0; i < n; ++i)
            input[i] *= Window::window<float>(window, i, n, false) / Window::coherent_gain(window);

        // // Bit-reversal Permutation using Pre-computed Table
        // for (size_t i = 0; i < n; ++i) {
        //     size_t j = bit_reversal_table[i];
        //     if (i < j) {
        //         std::swap(input[i], input[j]);
        //     }
        // }

        // Bit-reversal permutation
        size_t j = 0;
        for (size_t i = 0; i < n; ++i) {
            if (i < j) {
                std::swap(input[i], input[j]);
            }
            size_t mask = n >> 1;
            while (j >= mask && mask > 0) {
                j -= mask;
                mask >>= 1;
            }
            j += mask;
        }
        // Iterative Cooley-Tukey FFT
        for (size_t step = 2; step <= n; step <<= 1) {
            size_t half_step = step / 2;
            size_t twiddle_stride = n / step;
            for (size_t group = 0; group < n; group += step) {
                for (size_t pair = 0; pair < half_step; ++pair) {
                    std::complex<float> w = twiddles[pair * twiddle_stride];
                    std::complex<float> even = input[group + pair];
                    std::complex<float> odd = input[group + pair + half_step] * w;
                    input[group + pair] = even + odd;
                    input[group + pair + half_step] = even - odd;
                }
            }
        }
    }
};

/// @brief An RFFT computation utility based on pre-computed Twiddle factors.
class RFFT {
 private:
    /// @brief The N/2 point FFT.
    FFT fft;
    /// @brief Pre-computed twiddle factors for reconstructing an N-point FFT.
    TwiddleFactors twiddles;

 public:
    /// @brief Initialize the RFFT
    /// @param n The length of the RFFT.
    explicit RFFT(const size_t& n) : fft(n / 2), twiddles(n) { }

    /// @brief Resize the RFFT.
    /// @param n The length of the RFFT.
    inline void resize(const size_t& n) {
        fft.resize(n / 2);
        twiddles.resize(n);
    }

    /// @brief Return the length of the RFFT.
    inline size_t size() const {
        return twiddles.size();
    }

    /// @brief Compute the coefficients.
    /// @param input The input signal to compute the DFT coefficients of.
    /// @param window The window function to use before computing the DFT.
    inline std::vector<std::complex<float>> operator()(float* input,
        const Window::Function& window = Window::Function::Boxcar
    ) {
        // Determine the length of the FFT, i.e., N.
        const auto N = size();
        // Apply windowing to the input signal (expected to have length N.)
        for (size_t n = 0; n < N; ++n)
            input[n] *= Window::window<float>(window, n, N, false) / Window::coherent_gain(window);
        // Interleave the even and odd components using a clever casting based
        // approach. If we can guarantee that even and odd tuples are evenly
        // spaced and contiguous, then we can simply cast each (even,odd)
        // tuple directly to a complex float. This reduces the complexity of
        // interleaving from O(N) to O(1) and encumbers no memory overhead.
        auto interleaved = reinterpret_cast<std::complex<float>*>(input);
        // Take the N/2 point FFT of the interleaved signal.
        fft(interleaved);

        std::vector<std::complex<float>> output(N);

        // Handle k = 0 and k = N/2 (the DC bin and Nyquist bin)
        // Typically:
        //   X[0] = real( interleaved[0] ) + imag( interleaved[0] )
        //   X[N/2] = real( interleaved[0] ) - imag( interleaved[0] )
        float re0 = interleaved[0].real();
        float im0 = interleaved[0].imag();

        output[0]          = std::complex<float>(re0 + im0, 0.0f);
        output[N / 2]      = std::complex<float>(re0 - im0, 0.0f);

        // For k = 1..(N/2 - 1), we need to use the known RFFT reconstruction formula
        for (size_t k = 1; k < N / 2; k++) {
            // Let Xk = interleaved[k]
            // Let XNk = interleaved[N/2 - k] (the "mirrored" index)
            std::complex<float> Xk  = interleaved[k];
            std::complex<float> XNk = std::conj(interleaved[N / 2 - k]); // conjugate because of Hermitian symmetry

            // Twiddle factor for k
            auto Wk = twiddles[k];

            // The typical RFFT combination:
            // output[k]   = 0.5 * [ (Xk + XNk) - j*Wk*(Xk - XNk) ]
            // output[N-k] = conj(output[k])

            auto half = 0.5f;

            auto sum     = Xk + XNk;     // (Xk + XNk)
            auto diff    = Xk - XNk;     // (Xk - XNk)
            // multiply diff by -j = j * (XNk - Xk), or simply diff * ( -i )
            // but it's easier to do Wk * ( diff * -i ).
            // In many references, you'll see a re-labeling that Wk has also a j factor built in.

            // This is one place where you have to be careful about
            // how you incorporate the "e^{-j 2 pi k / N}" factor.

            auto magic   = std::complex<float>(0.0f, -1.0f) * diff;  // multiply by -j
            auto twisted = Wk * magic;                                // then by the twiddle

            output[N / 2 - k]         = half * (sum - twisted);
            output[N / 2 + k]     = std::conj(output[k]);
        }

        return output;
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
    if (N == 0) {
        return {};
    }

    // Default fMax to sampleRate if not specified or <= 0
    if (fMax <= 0.0f) {
        fMax = sampleRate;
    }

    //--------------------------------------------------------------------------
    // 1) Build a frequency array (linearly spaced)
    //    freq[i] = i * (sampleRate / N)
    //--------------------------------------------------------------------------
    std::vector<float> freqs(N);
    const float binWidth = sampleRate / static_cast<float>(N);
    for (size_t i = 0; i < N; ++i) {
        freqs[i] = i * binWidth;
    }

    //--------------------------------------------------------------------------
    // 2) Build a magnitude array and a prefix sum for quick averaging
    //
    //    mag[i] = sqrt( real^2 + imag^2 )
    //    prefixMag[i] = sum of mag[0..i-1], with prefixMag[0] = 0 for convenience
    //--------------------------------------------------------------------------
    std::vector<float> mag(N);
    mag.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        float re = fftCoeffs[i].real();
        float im = fftCoeffs[i].imag();
        mag[i] = std::sqrt(re * re + im * im);
    }

    std::vector<double> prefixMag(N + 1, 0.0);
    for (size_t i = 1; i <= N; ++i) {
        prefixMag[i] = prefixMag[i-1] + static_cast<double>(mag[i-1]);
    }

    // Helper lambda: sum of magnitudes in range [start, end] (inclusive of end).
    // We'll clamp the indices to [0, N-1].
    auto rangeSum = [&](size_t start, size_t end){
        if (end < start) return 0.0;
        start = std::min(start, N-1UL);
        end   = std::min(end,   N-1UL);
        return prefixMag[end + 1] - prefixMag[start];
    };

    //--------------------------------------------------------------------------
    // 3) For each bin i, define log-based window around freqs[i],
    //    then find the bin indices [lowIdx, highIdx] via binary search,
    //    and compute the average magnitude using prefix sums.
    //--------------------------------------------------------------------------
    Math::DFTCoefficients smoothed(N, 0.f);

    // halfBandFactor = 2^(fractionOfOctave/2)
    // e.g. fractionOfOctave=1 => halfBandFactor=~1.4142 => 1-octave total
    const float halfBandFactor = std::pow(2.0f, fractionOfOctave / 2.0f);

    // We’ll do a quick lambda to help clamp a frequency to [fMin, fMax].
    auto clampFreq = [&](float f) {
        return std::max(fMin, std::min(f, fMax));
    };

    for (size_t i = 0; i < N; ++i)
    {
        float fCenter = freqs[i];
        // If outside [fMin, fMax], we skip or set to 0
        if (fCenter < fMin || fCenter > fMax) {
            smoothed[i] = 0.0f;
            continue;
        }

        // Define log-based window
        float fLow  = clampFreq(fCenter / halfBandFactor);
        float fHigh = clampFreq(fCenter * halfBandFactor);

        // If fHigh < fLow, skip
        if (fHigh < fLow) {
            smoothed[i] = 0.0f;
            continue;
        }

        // ---------------------------------------------------------------
        // Find the bin indices that correspond to fLow and fHigh.
        // freq[idx] = idx * binWidth.
        // => idx = freq / binWidth.
        //
        // We'll do an integer cast, but let's be safer and do:
        //    idx = floor( freq / binWidth ) for fLow
        //    idx = floor( freq / binWidth ) for fHigh
        // Or we could do binary search, but a direct formula is simpler
        // since freq[i] is linear with i.
        // ---------------------------------------------------------------
        size_t lowIdx  = static_cast<size_t>(std::floor(fLow  / binWidth));
        size_t highIdx = static_cast<size_t>(std::floor(fHigh / binWidth));

        if (lowIdx > highIdx || lowIdx >= N) {
            smoothed[i] = 0.0f;
            continue;
        }
        if (highIdx >= N) {
            highIdx = N - 1;
        }

        // sum of magnitudes in [lowIdx .. highIdx]
        double sumInRange = rangeSum(lowIdx, highIdx);
        size_t count      = (highIdx - lowIdx + 1);

        // arithmetic mean
        smoothed[i] = (count > 0) ? static_cast<float>(sumInRange / count) : 0.0f;
    }

    return smoothed;
}



inline void fft_optimized(
    std::complex<float>* input,
    const size_t& n,
    const Window::Function& window = Window::Function::Boxcar
) {
    // Input signal windowing.
    for (size_t i = 0; i < n; ++i)
        input[i] *= Window::window<float>(window, i, n, false) / Window::coherent_gain(window);
    // Bit-reversal permutation
    size_t j = 0;
    for (size_t i = 0; i < n; ++i) {
        if (i < j) {
            std::swap(input[i], input[j]);
        }
        size_t mask = n >> 1;
        while (j >= mask && mask > 0) {
            j -= mask;
            mask >>= 1;
        }
        j += mask;
    }
    // Precompute twiddle factors
    TwiddleFactors twiddles(n);
    // Iterative Cooley-Tukey FFT
    for (size_t step = 2; step <= n; step <<= 1) {
        size_t half_step = step / 2;
        size_t twiddle_stride = n / step;
        for (size_t group = 0; group < n; group += step) {
            for (size_t pair = 0; pair < half_step; ++pair) {
                std::complex<float> w = twiddles[pair * twiddle_stride];
                std::complex<float> even = input[group + pair];
                std::complex<float> odd = input[group + pair + half_step] * w;
                input[group + pair] = even + odd;
                input[group + pair + half_step] = even - odd;
            }
        }
    }
}




// ---------------------------------------------------------------------------
// MARK: IFFT
// ---------------------------------------------------------------------------

/// @brief Compute the Inverse Fast Fourier Transform (IFFT) in-place.
///
/// @tparam N the length of the input coefficients.
/// @param input the input coefficients to compute the IFFT of, \f$h[n]\f$
///
template<int N>
void ifft_(std::vector<std::complex<float>>& input) {
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");
    std::vector<std::complex<float>> even(N / 2), odds(N / 2);
    for (int i = 0; i < N / 2; ++i) {
        even[i] = input[2*i];
        odds[i] = input[2*i+1];
    }
    ifft_<N / 2>(even);
    ifft_<N / 2>(odds);
    for (int i = 0; i < N / 2; ++i) {
        std::complex<float> w = exp(std::complex<float>(0, 2 * pi<float>() * i / N)) * odds[i];
        input[i] = (even[i] + w) / 2.f;
        input[i + N / 2] = (even[i] - w) / 2.f;
    }
}

// The base case of a 1-point IFFT is a NoOp.
template<>
inline void ifft_<1>(std::vector<std::complex<float>>&) { }

/// @brief Compute the Inverse Fast Fourier Transform (IFFT).
///
/// @tparam N the length of the input coefficients.
/// @param input the input coefficients to compute the IFFT of, \f$h[n]\f$
/// @returns The real-valued components of the reconstructed signal.
///
template<int N>
std::vector<float> ifft(std::vector<std::complex<float>>& input) {
    ifft_<N>(input);
    return complex_to_real(input);
}

}  // namespace Math

#endif  // DSP_MATH_FFT_HPP
