// Inverse jobs and overlap-save frequency-domain processing for paper measurements.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_SYNTHESIS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_SYNTHESIS_HPP_

#include <complex>
#include <limits>
#include "protocol.hpp"
#include "references.hpp"
#include "../../src/dsp/fft.hpp"

namespace Paper {

/// @brief Fixed-cadence batch or balanced execution of a known work sequence.
/// @details Work units can include bulk buffer calls; this is not a WCET model.
struct FrameSchedule {
    size_t hop, work, base, remainder, phase = 0, error = 0;
    bool batch, complete = false;
    FrameSchedule(const Config& c, size_t units) : hop(c.hop), work(units),
        base(units/c.hop), remainder(units%c.hop),
        batch(std::string(backend_descriptor(c.backend).schedule) == "immediate") {}
    size_t quota() {
        complete = false;
        if (batch) return phase == 0 ? work : 0;
        error += remainder;
        const size_t extra = error >= hop;
        if (extra) error -= hop;
        return base + extra;
    }
    size_t delay() const { return batch ? 0 : hop-1; }
    void advance() { phase = (phase+1)%hop; }
};

/// @brief Periodically released complex spectra, independent of any analyzer.
/// @details Alternating sparse non-Hermitian fixtures have analytical inverses.
/// All N output stores are charged. Buffering remains a timed O(N) boundary pass.
template<typename T>
struct InverseStream {
    using Complex = std::complex<T>;
    Fourier::OnTheFlyIFFT<T> inverse;
    std::vector<Complex> spectra[2], expected[2], output;
    FrameSchedule schedule;
    size_t frame = 0, fixture = 0, cursor = 0;

    explicit InverseStream(const Config& c) : inverse(c.n), output(c.n),
        schedule(c, inverse.get_total_steps()+c.n) {
        for (size_t f = 0; f < 2; ++f) {
            spectra[f].resize(c.n);
            expected[f].resize(c.n);
            const Complex dc(T(0.125*(f+1)), T(-0.0625));
            const Complex positive(T(0.5), T(0.125*(f+1)));
            const Complex negative(T(-0.25), T(0.0625*(f+1)));
            spectra[f][0] = T(c.n)*dc;
            spectra[f][7] = T(c.n)*positive;
            spectra[f][c.n-3] = T(c.n)*negative;
            for (size_t i = 0; i < c.n; ++i) {
                const long double angle = 2*std::acos(-1.L)*i/c.n;
                const auto a = std::complex<long double>(std::cos(7*angle), std::sin(7*angle));
                const auto b = std::complex<long double>(std::cos(-3*angle), std::sin(-3*angle));
                expected[f][i] = Complex(std::complex<long double>(dc)
                    + std::complex<long double>(positive)*a + std::complex<long double>(negative)*b);
            }
        }
    }
    void process(float) {
        if (schedule.phase == 0) {
            fixture = frame++%2;
            inverse.buffer(spectra[fixture].data());
            cursor = 0;
        }
        const size_t steps = inverse.get_total_steps();
        const size_t quota = schedule.quota();
        for (size_t i = 0; i < quota; ++i, ++cursor) {
            if (cursor < steps) inverse.step();
            else output[cursor-steps] = inverse.coefficients[cursor-steps];
        }
        schedule.complete = quota && cursor == schedule.work;
        schedule.advance();
    }
    size_t delay() const { return schedule.delay(); }
    bool published() const { return schedule.complete; }
    void barrier() const { observe(output.data()); }
    void check() const {
        for (auto value : output)
            require(std::isfinite(value.real()) && std::isfinite(value.imag()), "Invalid inverse job output");
    }
};

/// @brief Full complex FFT -> transfer function -> IFFT -> overlap-save delivery.
/// @details A frame ending at jH emits its last H samples; H <= N-2 prevents
/// circular aliasing for the three-tap FIR. No analysis/synthesis window is
/// applied. Playback begins on publication, for total delay H-1+delay().
/// Identity and FIR use the same path, including N spectral multiplications.
template<typename T>
struct FrequencyChain {
    using Complex = std::complex<T>;
    Fourier::OnTheFlyFFT<T> forward;
    Fourier::OnTheFlyIFFT<T> inverse;
    std::vector<Complex> ring, frame, transfer, pending, playing;
    FrameSchedule schedule;
    size_t head = 0, cursor = 0, playback = 0;
    bool identity;
    Complex audio_output = Complex(0, 0);
    Complex delivered_checksum = Complex(0, 0);

    explicit FrequencyChain(const Config& c) : forward(c.n), inverse(c.n),
        ring(c.n), frame(c.n), transfer(c.n), pending(c.hop), playing(c.hop),
        schedule(c, forward.get_total_steps()+c.n+1+inverse.get_total_steps()+c.hop),
        playback(c.hop), identity(std::string(backend_descriptor(c.backend).operation) == "identity") {
        require(c.hop <= c.n-2, "Overlap-save requires H <= N-2 for the three-tap FIR");
        for (size_t k = 0; k < c.n; ++k) {
            const long double angle = -2*std::acos(-1.L)*k/c.n;
            const std::complex<long double> z(std::cos(angle), std::sin(angle));
            transfer[k] = identity ? Complex(1, 0) : Complex(0.5L - 0.25L*z + 0.125L*z*z);
        }
    }
    void process(float value) {
        ring[head] = Complex(T(value), T(-0.25f*value));
        head = (head+1)%ring.size();
        if (schedule.phase == 0) {
            for (size_t i = 0; i < ring.size(); ++i) frame[i] = ring[(head+i)%ring.size()];
            forward.buffer(frame.data());
            cursor = 0;
        }
        const size_t forward_end = forward.get_total_steps();
        const size_t multiply_end = forward_end + ring.size();
        const size_t inverse_end = multiply_end + 1 + inverse.get_total_steps();
        const size_t quota = schedule.quota();
        for (size_t i = 0; i < quota; ++i, ++cursor) {
            if (cursor < forward_end) forward.step();
            else if (cursor < multiply_end) {
                const size_t k = cursor-forward_end;
                forward.coefficients[k] = Fourier::complex_multiply(forward.coefficients[k], transfer[k]);
            } else if (cursor == multiply_end) inverse.buffer(forward.coefficients.data());
            else if (cursor < inverse_end) inverse.step();
            else {
                const size_t k = cursor-inverse_end;
                pending[k] = inverse.coefficients[ring.size()-pending.size()+k];
            }
        }
        schedule.complete = quota && cursor == schedule.work;
        if (schedule.complete) { playing.swap(pending); playback = 0; }
        audio_output = playback < playing.size() ? playing[playback++] : Complex(0, 0);
        // Keep every delivered sample observable, including continuous passes.
        // This common dependent sink is charged to every complete-chain backend.
        delivered_checksum = T(0.999)*delivered_checksum + audio_output;
        schedule.advance();
    }
    size_t delay() const { return schedule.delay(); }
    size_t playback_delay() const { return playing.size()-1+delay(); }
    bool published() const { return schedule.complete; }
    void barrier() const { observe(playing.data()); observe(pending.data()); observe(delivered_checksum); }
    void check() const {
        for (auto value : playing)
            require(std::isfinite(value.real()) && std::isfinite(value.imag()), "Invalid overlap-save output");
    }
};

/// @brief Independent numerical audit, executed only outside timed intervals.
struct SynthesisAccuracy {
    double maximum_error = 0, maximum_reference = 0;
    size_t checked = 0, publications = 0, playback_checked = 0;
    template<typename T>
    void compare(std::complex<T> actual, std::complex<T> expected) {
        const double error = std::abs(actual-expected), scale = std::abs(expected);
        require(std::isfinite(error) && std::isfinite(scale), "Non-finite synthesis reference");
        maximum_error = std::max(maximum_error, error);
        maximum_reference = std::max(maximum_reference, scale);
        ++checked;
        require(error <= Reference::tolerance<T>()*std::max(1., scale),
            "Independent synthesis output differs");
    }
    void print() const {
        std::cerr.precision(17);
        std::cerr << "{\"reference\":\"analytical complex inverse or direct time-domain FIR\","
            << "\"max_abs_error\":" << maximum_error << ",\"max_reference\":" << maximum_reference
            << ",\"checked_samples\":" << checked << ",\"publications\":" << publications
            << ",\"playback_checked_samples\":" << playback_checked << "}\n";
    }
};

/// @brief Closed-form inverse and direct convolution; no transform oracle reuse.
struct SynthesisAudit {
    SynthesisAccuracy& accuracy;
    template<typename T>
    void operator()(const InverseStream<T>& adapter, const std::vector<float>&, size_t sample) const {
        if (!adapter.published()) return;
        const size_t fixture = ((sample-adapter.delay())/adapter.schedule.hop)%2;
        require(adapter.fixture == fixture, "Inverse spectrum release sequence changed");
        ++accuracy.publications;
        for (size_t k = 0; k < adapter.output.size(); ++k)
            accuracy.compare(adapter.output[k], adapter.expected[fixture][k]);
    }
    template<typename T>
    static std::complex<T> reference(const std::vector<float>& input, int64_t index, bool identity) {
        std::complex<T> result(0, 0);
        const T taps[] = {T(0.5), T(-0.25), T(0.125)};
        for (int64_t j = 0; j < (identity ? 1 : 3); ++j) {
            if (index-j < 0) continue;
            const float value = input[size_t(index-j)%input.size()];
            result += (identity ? T(1) : taps[j])*std::complex<T>(T(value), T(-0.25f*value));
        }
        return result;
    }
    template<typename T>
    void operator()(const FrequencyChain<T>& adapter, const std::vector<float>& input, size_t sample) const {
        accuracy.compare(adapter.audio_output, reference<T>(input,
            int64_t(sample)-int64_t(adapter.playback_delay()), adapter.identity));
        ++accuracy.playback_checked;
        if (!adapter.published()) return;
        ++accuracy.publications;
        for (size_t k = 0; k < adapter.playing.size(); ++k)
            accuracy.compare(adapter.playing[k], reference<T>(input,
                int64_t(sample)-int64_t(adapter.playback_delay())+int64_t(k), adapter.identity));
    }
};

/// @brief Validate supported names/options before dispatch or storage preparation.
inline bool synthesis_backend(const std::string& backend) {
    const std::string kind(backend_descriptor(backend).kind);
    return kind == "inverse-job" || kind == "chain";
}

template<typename T>
void synthesis_stream(const Config& c) {
    require(!c.smooth && c.state != "live", "Synthesis has no analyzer smoothing or live window controls");
    SynthesisAccuracy accuracy;
    if (std::string(backend_descriptor(c.backend).kind) == "inverse-job")
        stream<InverseStream<T>>(c, SynthesisAudit{accuracy}, 0);
    else {
        require(c.hop <= c.n-2, "Overlap-save requires H <= N-2");
        const size_t delay = std::string(backend_descriptor(c.backend).schedule) == "immediate" ? 0 : c.hop-1;
        stream<FrequencyChain<T>>(c, SynthesisAudit{accuracy}, (c.hop-1)/2., c.hop-1+delay);
    }
    if (c.resources) return;
    require(accuracy.checked && accuracy.publications, "Missing synthesis numerical audit");
    accuracy.print();
}

/// @brief Exercise cadence, startup, wraparound and every output before campaigns.
template<typename T>
void verify_synthesis() {
    const auto input = signal();
    for (const std::string family : {"inverse-stream", "ols-identity", "ols-fir"})
        for (const std::string mode : {"batch", "incremental"})
            for (size_t n : {128u, 2048u, 16384u})
                for (size_t hop : {size_t(1), size_t(37), n/2}) {
                    if (n > 128 && hop < n/4) continue;
                    Config c{};
                    c.n = n; c.hop = hop; c.backend = family+"-"+mode+"-float";
                    SynthesisAccuracy accuracy;
                    SynthesisAudit audit{accuracy};
                    // Eight hops include multiple fixture changes and output-buffer swaps.
                    if (family == "inverse-stream") {
                        InverseStream<T> adapter(c);
                        for (size_t s = 0; s < 8*hop; ++s) {
                            adapter.process(input[s%input.size()]);
                            require(adapter.published() == (s%hop == adapter.delay()), "Inverse job cadence");
                            audit(adapter, input, s);
                        }
                    } else {
                        FrequencyChain<T> adapter(c);
                        // Include retained-input wraparound, even for a short hop.
                        for (size_t s = 0; s < 2*n+8*hop; ++s) {
                            adapter.process(input[s%input.size()]);
                            require(adapter.published() == (s%hop == adapter.delay()), "Overlap-save cadence");
                            audit(adapter, input, s);
                        }
                    }
                    require(accuracy.checked && accuracy.publications, "Missing synthesis checks");
                }
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_SYNTHESIS_HPP_
