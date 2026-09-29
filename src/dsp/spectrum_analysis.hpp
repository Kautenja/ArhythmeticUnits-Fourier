// Bounded, one-hop spectral analysis for scalar or SIMD samples.
//
// Copyright 2026 Arhythmetic Units
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_SPECTRUM_ANALYSIS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_SPECTRUM_ANALYSIS_HPP_

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "fft.hpp"
#include "window.hpp"

namespace Fourier {

/// @brief Frame-latched spectral analysis settings; magnitudes are unscaled FFT units.
struct SpectrumSettings {
    size_t length = 2048;
    size_t hop = 1024;
    Window::Function window = Window::Function::Boxcar;
    float sample_rate = 48000.f;
    float octave = 0.f;
    float alpha = 0.f;
};

/// @brief Windowed real FFT, positive-bin smoothing and output within one hop.
/// @tparam T Scalar or SIMD real arithmetic with ADL abs support.
/// @tparam OutputWeight Scheduling credit per output bin (1 or 2).
/// @details One producer owns this object. Storage is prepared at construction.
/// For M=N/2, K=M+1 and B=(M/2)log2(M), each frame has W=pM+B+(1+o)K units,
/// where o=OutputWeight (default 1), allowing for a more costly output callback,
/// and p=4 while rebuilding the window cache or p=1 otherwise. Each pair
/// executes at its first preparation unit; the remaining p-1 units are credit.
/// Output bins similarly execute at the first of their o units.
/// Call s executes floor((s+1)W/H)-floor(sW/H), at most ceil(W/H) units.
/// Preparation, butterflies, reconstruction/prefix sums and per-bin output
/// execute in dependency order. A frame ending at sample jH publishes at
/// jH+H-1, with zero padding before the first input. Units have unequal costs.
/// The output callback runs once per positive bin in the last stage; it must
/// not allocate, block, or retain references. Only publish its destination
/// after process() returns true. Input is assumed finite.
template<typename T, size_t OutputWeight = 1>
class SpectrumAnalysis {
    static_assert(OutputWeight == 1 || OutputWeight == 2,
        "Spectrum output weight must be one or two");
    /// Maximum-size plans also serve smaller transforms by index/stride scaling.
    const size_t maximum_length;
    BitReversalTable reversal;
    // SIMD lanes share scalar twiddles: avoid lane-wise approximate trig and
    // redundant plan storage. Native scalar precision is retained for double.
    using Scalar = typename std::conditional<std::is_floating_point<T>::value, T, float>::type;
    TwiddleFactors<Scalar> twiddles;
    std::vector<T> input, magnitude, prefix, average;
    std::vector<std::complex<T>> packed;
    std::vector<float> window;
    std::vector<size_t> band_low, band_high;
    SpectrumSettings settings;
    /// Next input write, retained sample count and current frame's oldest index.
    size_t head = 0, available = 0, origin = 0, frame_available = 0;
    /// Hop phase, dependency stage and unit index; butterfly traversal state.
    size_t phase = 0, stage = 0, cursor = 0, span = 2, group = 0, pair = 0;
    size_t butterflies = 0, reversal_shift = 0, preparation_units = 0;
    /// Quotient/remainder accumulator realizes the balanced quota without division per sample.
    size_t quota_base = 0, quota_remainder = 0, quota_error = 0;
    bool window_dirty = true, bands_dirty = true, clear_average = true;
    float window_gain = 1.f, half_band = 1.f, band_ratio = 1.f;

    /// @brief Refresh the next frame's weighted schedule at a cache/configuration boundary.
    void update_schedule() {
        preparation_units = (window_dirty ? 4 : 1) * (settings.length / 2);
        const size_t work = work_per_frame();
        quota_base = work / settings.hop;
        quota_remainder = work % settings.hop;
    }

    /// @brief Reject invalid capacities before constructing any FFT plan.
    static size_t checked_length(size_t length, size_t hop) {
        if (length < 4 || length > 16384 || (length & (length - 1))
            || hop < 1 || hop > (1u << 20))
            throw std::invalid_argument("Invalid spectrum analysis capacity");
        return length;
    }

    /// @brief Read a retained frame sample, respecting logical zero padding.
    T sample(size_t index) const {
        if (index + frame_available < settings.length) return T(0.f);
        return input[(origin + index) % input.size()];
    }

    /// @brief Fuse window preparation/application, real packing and permutation.
    template<bool Rebuild>
    void prepare_pair(size_t k) {
        if (Rebuild) {
            for (size_t i = 2*k; i < 2*k+2; ++i)
                window[i] = window_gain * Window::window<float>(settings.window,
                    static_cast<float>(i), static_cast<float>(settings.length), false);
        }
        packed[reversal[k] >> reversal_shift] = {
            sample(2*k) * window[2*k], sample(2*k+1) * window[2*k+1]};
    }

    /// @brief Execute one radix-2 butterfly with the maximum-size twiddle table.
    void butterfly() {
        const size_t half = span / 2;
        const std::complex<T> w(twiddles[pair * (maximum_length / span)]);
        const auto even = packed[group + pair];
        const auto odd = complex_multiply(packed[group + pair + half], w);
        packed[group + pair] = even + odd;
        packed[group + pair + half] = even - odd;
        if (++pair == half) {
            pair = 0;
            group += span;
            if (group == settings.length / 2) { group = 0; span *= 2; }
        }
    }

    /// @brief Resume SIMD butterflies within the current quota, splitting at group ends.
    void butterfly_segment(size_t count) {
        while (count) {
            const size_t half = span / 2;
            const size_t end = pair + std::min(count, half - pair);
            const size_t stride = maximum_length / span;
            size_t twiddle = pair * stride;
            count -= end - pair;
            for (; pair < end; ++pair, twiddle += stride) {
                const std::complex<T> w(twiddles[twiddle]);
                const auto even = packed[group + pair];
                const auto odd = complex_multiply(packed[group + pair + half], w);
                packed[group + pair] = even + odd;
                packed[group + pair + half] = even - odd;
            }
            if (pair == half) {
                pair = 0;
                group += span;
                if (group == settings.length / 2) { group = 0; span *= 2; }
            }
        }
    }

    /// @brief Reconstruct one positive bin and extend the magnitude prefix sum.
    void reconstruct(size_t k) {
        const size_t m = settings.length / 2;
        std::complex<T> value;
        if (k == 0 || k == m) {
            const auto z = packed[0];
            value = {z.real() + (k == 0 ? z.imag() : -z.imag()), T(0.f)};
        } else {
            const auto a = packed[k];
            const auto b = std::conj(packed[m-k]);
            value = complex_multiply(std::complex<T>(
                twiddles[k * (maximum_length / settings.length)]), a-b);
            value = complex_multiply(value, j<T>());
            value = complex_multiply<T>(a + b - value, T(0.5f));
        }
        using std::abs;
        magnitude[k] = abs(value);
        prefix[k+1] = prefix[k] + magnitude[k];
    }

    /// @brief Cache one original octave interval, including Nyquist adjustment.
    void prepare_band(size_t k) {
        const float width = settings.sample_rate / settings.length;
        const float maximum = settings.sample_rate / 2.f;
        float low = k * width / half_band, high = k * width * half_band;
        if (high > maximum) { high = maximum; low = high / band_ratio; }
        band_low[k] = static_cast<size_t>(std::floor(low / width));
        band_high[k] = std::min(settings.length / 2,
            static_cast<size_t>(std::floor(high / width)));
    }

    /// @brief Smooth and emit one bin, preserving the frame's output order.
    template<typename Output>
    void output_bin(size_t k, Output& emit) {
        if (bands_dirty) prepare_band(k);
        const size_t low = band_low[k], high = band_high[k];
        const T value = settings.octave == 0.f ? magnitude[k]
            : (prefix[high+1] - prefix[low]) / T(high-low+1);
        using std::abs;
        const T previous = clear_average ? T(0.f) : abs(average[k]);
        average[k] = settings.alpha * previous + (1.f-settings.alpha) * value;
        emit(k, average[k]);
    }

 public:
    /// @brief Prepare maximum transform and retained-input capacity off the sample path.
    explicit SpectrumAnalysis(size_t length, size_t maximum_hop) :
        maximum_length(checked_length(length, maximum_hop)),
        reversal(maximum_length / 2), twiddles(maximum_length),
        input(maximum_length + maximum_hop, T(0.f)), magnitude(maximum_length / 2 + 1),
        prefix(maximum_length / 2 + 2), average(maximum_length / 2 + 1),
        packed(maximum_length / 2), window(maximum_length),
        band_low(maximum_length / 2 + 1), band_high(maximum_length / 2 + 1) {
        settings.length = maximum_length;
        settings.hop = maximum_hop;
        configure(settings);
    }

    /// @brief Grow retained-input capacity only during host reconfiguration.
    /// @details May allocate. Call with no concurrent processing; growth cancels
    /// the current frame. No operation for already prepared capacities.
    void reserve_hop(size_t hop) {
        checked_length(maximum_length, hop);
        if (hop <= input.size() - maximum_length) return;
        input.resize(maximum_length + hop, T(0.f));
        reset();
    }

    /// @brief Cancel pending work and logically clear input/averaging in constant time.
    void reset() {
        phase = 0;
        available = 0;
        clear_average = true;
        // A cancelled cache rebuild may have written only some entries.
        window_dirty = bands_dirty = true;
        update_schedule();
    }

    /// @brief Whether settings can be latched before the next input sample.
    bool is_frame_start() const { return phase == 0; }
    size_t size() const { return settings.length; }
    size_t hop_length() const { return settings.hop; }
    /// @brief Scheduling units for the active frame, or the next frame when idle.
    size_t work_per_frame() const {
        return preparation_units + butterflies + (1+OutputWeight)*(size()/2+1);
    }

    /// @brief Latch settings at a frame boundary; reject invalid values without mutation.
    /// @returns false for a mid-frame call or out-of-capacity/invalid settings.
    /// @details Window and band cache work is deferred to scheduled units.
    /// A length change clears input and EMA logically; other controls preserve them.
    bool configure(const SpectrumSettings& value) {
        if (phase != 0 || value.length < 4 || value.length > maximum_length
            || (value.length & (value.length-1)) || value.hop < 1
            || value.hop > input.size() - maximum_length
            || value.window < Window::Function::Boxcar || value.window > Window::Function::Flattop
            || !std::isfinite(value.alpha) || value.alpha < 0 || value.alpha > 1
            || !std::isfinite(value.octave) || value.octave < 0 || value.octave > 2.5f
            || !std::isfinite(value.sample_rate) || value.sample_rate < 1
            || value.sample_rate > 1000000.f) return false;
        const bool resized = settings.length != value.length;
        if (resized) { available = 0; clear_average = true; }
        window_dirty = window_dirty || resized || settings.window != value.window;
        bands_dirty = bands_dirty || resized || settings.octave != value.octave
            || settings.sample_rate != value.sample_rate;
        settings = value;
        window_gain = 1.f / Window::coherent_gain(settings.window);
        half_band = std::pow(2.f, settings.octave / 2.f);
        band_ratio = std::pow(2.f, settings.octave);
        butterflies = 0;
        for (size_t m = settings.length / 2; m > 1; m /= 2) butterflies += settings.length / 4;
        reversal_shift = 0;
        for (size_t n = settings.length; n < maximum_length; n *= 2) ++reversal_shift;
        update_schedule();
        return true;
    }

    /// @brief Accept input and spend this sample's quota; true publishes a complete frame.
    /// @param emit Called as emit(bin, magnitude) once per bin in DC..Nyquist.
    /// @param capture false retains the input history while continuing analysis.
    template<typename Output>
    bool process(const T& value, Output emit, bool capture = true) {
        if (capture) {
            input[head] = value;
            head = (head + 1) % input.size();
            available = std::min(maximum_length, available + 1);
        }
        if (phase == 0) {
            origin = (head + input.size() - size()) % input.size();
            frame_available = available;
            stage = cursor = group = pair = quota_error = 0;
            span = 2;
            prefix[0] = T(0.f);
        }
        size_t quota = quota_base;
        quota_error += quota_remainder;
        if (quota_error >= settings.hop) { quota_error -= settings.hop; ++quota; }
        const size_t lengths[] = {preparation_units, butterflies,
            size()/2+1, OutputWeight*(size()/2+1)};
        // Retain the simple loop when calls do at most two units. Segment
        // setup costs more than it saves for these sparse schedules.
        if (quota_base <= 1) {
            for (size_t i = 0; i < quota; ++i) {
                switch (stage) {
                case 0:
                    if (!window_dirty) prepare_pair<false>(cursor);
                    else if (cursor % 4 == 0) prepare_pair<true>(cursor / 4);
                    break;
                case 1: butterfly(); break;
                case 2: reconstruct(cursor); break;
                case 3:
                    if (cursor % OutputWeight == 0) output_bin(cursor / OutputWeight, emit);
                    break;
                }
                if (++cursor == lengths[stage]) { cursor = 0; ++stage; }
            }
        } else {
            // Dispatch once per contiguous stage segment, without changing this
            // sample's weighted quota or arithmetic order.
            while (quota) {
                const size_t count = std::min(quota, lengths[stage] - cursor);
                const size_t end = cursor + count;
                switch (stage) {
                case 0:
                    if (window_dirty) {
                        // Execute exactly the multiples of four in [cursor,end).
                        // Skipping credit in one step avoids per-pair bookkeeping.
                        const size_t stop = (end + 3) / 4;
                        for (size_t k = (cursor + 3) / 4; k < stop; ++k)
                            prepare_pair<true>(k);
                        cursor = end;
                    } else {
                        for (; cursor < end; ++cursor) prepare_pair<false>(cursor);
                    }
                    break;
                case 1:
                    // Keep scalar traversal; SIMD shares stride/group setup.
                    if (std::is_floating_point<T>::value) {
                        for (; cursor < end; ++cursor) butterfly();
                    } else {
                        butterfly_segment(count);
                        cursor = end;
                    }
                    break;
                case 2:
                    for (; cursor < end; ++cursor) reconstruct(cursor);
                    break;
                case 3:
                    if (OutputWeight == 1) {
                        for (; cursor < end; ++cursor) output_bin(cursor, emit);
                    } else {
                        const size_t stop = (end + OutputWeight-1) / OutputWeight;
                        for (size_t k = (cursor + OutputWeight-1) / OutputWeight; k < stop; ++k)
                            output_bin(k, emit);
                        cursor = end;
                    }
                    break;
                }
                quota -= count;
                if (cursor == lengths[stage]) { cursor = 0; ++stage; }
            }
        }
        if (++phase != settings.hop) return false;
        phase = 0;
        if (window_dirty) {
            window_dirty = false;
            update_schedule();
        }
        bands_dirty = clear_average = false;
        return true;
    }
};

}  // namespace Fourier
#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_SPECTRUM_ANALYSIS_HPP_
