// First-party analysis and FFT/RFFT/IFFT workloads and numerical checks.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_FOURIER_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_FOURIER_HPP_

#include <complex>
#include <limits>
#include <rack.hpp>
#include "../../src/dsp/spectrum_analysis.hpp"
#include "protocol.hpp"
#include "references.hpp"
namespace Paper {
template<typename T> T lanes(float value) { return T(value); }
template<> rack::simd::float_4 lanes(float value) {
    return rack::simd::float_4(value, value*0.75f, value*0.5f, value*0.25f);
}
template<typename T> double sum(T value) { return value; }
template<> double sum(rack::simd::float_4 value) { return value[0]+value[1]+value[2]+value[3]; }

/// @brief Production analysis; output work is precisely K magnitude stores.
template<typename T>
struct Core {
    Fourier::SpectrumAnalysis<T> analysis;
    Fourier::SpectrumSettings settings;
    std::vector<T> output;
    bool complete = false, live;
    size_t frames = 0;
    explicit Core(const Config& c) : analysis(16384, c.hop), output(c.n/2+1), live(c.state == "live") {
        settings.length = c.n;
        settings.hop = c.hop;
        settings.sample_rate = c.rate;
        settings.window = Fourier::Window::Function::Hann;
        settings.octave = c.smooth ? 1.f/3.f : 0.f;
        settings.alpha = c.smooth ? 0.8f : 0.f;
        require(analysis.configure(settings), "Core settings rejected");
    }
    void process_value(T input) {
        if (live && analysis.is_frame_start()) {
            // Rebuild both caches every frame, retaining the requested cadence.
            settings.window = frames++%2 ? Fourier::Window::Function::Hann
                                        : Fourier::Window::Function::BlackmanHarris;
            settings.octave = frames%2 ? 0.f : 1.f/3.f;
            analysis.configure(settings);
        }
        complete = analysis.process(input, [this](size_t bin, T magnitude) {
            output[bin] = magnitude;
        });
    }
    void process(float input) { process_value(lanes<T>(input)); }
    size_t delay() const { return settings.hop-1; }
    bool published() const { return complete; }
    void barrier() const { observe(output.data()); }
    void check() const {
        double total = 0;
        for (auto value : output) {
            require(std::isfinite(sum(value)), "Non-finite core output");
            total += sum(value);
        }
        require(total > 0, "Core output is empty");
    }
};

/// @brief Own RFFT as fixed-cadence batch/incremental controls, not old modules.
/// @details Both snapshot the same Hann frame at jH, retain full-complex RFFT
/// reconstruction, smooth magnitudes, and publish as soon as that frame is done.
/// They wait for the next scheduled frame instead of restarting early.
template<typename T>
struct Legacy {
    Config config;
    Fourier::OnTheFlyRFFT<T> fft;
    Fourier::Window::CachedWindow<float> window;
    std::vector<T> ring, frame, output;
    size_t head = 0, phase = 0, frames = 0;
    bool complete = false, delivered = false, smooth, immediate;
    explicit Legacy(const Config& c) : config(c), fft(c.n),
        window(Fourier::Window::Function::Hann, c.n, false, true),
        ring(c.n, T(0)), frame(c.n), output(c.n/2+1, T(0)), smooth(c.smooth),
        immediate(std::string(backend_descriptor(c.backend).schedule) == "immediate") {}
    size_t delay() const {
        if (immediate) return 0;
        const size_t steps = fft.get_total_steps();
        const size_t quota = (steps+config.hop-1)/config.hop;
        return (steps+quota-1)/quota-1;
    }
    void process(float value) {
        ring[head] = T(value);
        head = (head+1)%ring.size();
        complete = false;
        if (phase == 0) {
            if (config.state == "live") {
                const auto function = frames++%2 ? Fourier::Window::Function::Hann
                                                 : Fourier::Window::Function::BlackmanHarris;
                window.set_window(function, config.n, false, true);
                smooth = frames%2 == 0;
            }
            for (size_t i = 0; i < ring.size(); ++i) frame[i] = ring[(head+i)%ring.size()];
            fft.buffer(frame.data(), window.get_samples());
            delivered = false;
        }
        if (!delivered) {
            if (immediate) fft.compute();
            else fft.step(config.hop);
            if (fft.is_done_computing()) {
                if (smooth) fft.smooth(config.rate, 1.f/3.f);
                const float alpha = config.smooth ? 0.8f : 0.f;
                for (size_t k = 0; k < output.size(); ++k)
                    output[k] = alpha*output[k] + (1.f-alpha)*std::abs(fft.coefficients[k]);
                delivered = complete = true;
            }
        }
        phase = (phase+1)%config.hop;
    }
    bool published() const { return complete; }
    void barrier() const { observe(output.data()); }
    void check() const {
        double total = 0;
        for (auto value : output) {
            require(std::isfinite(value), "Non-finite legacy output");
            total += value;
        }
        require(total > 0, "Empty legacy output");
    }
};

/// @brief Compare matched frames despite their different publication delays.
template<typename T>
void verify_controls(const std::vector<size_t>& sizes = {128u, 2048u, 16384u},
        const std::vector<size_t>& hops = {257u, 1024u}) {
    const auto input = signal();
    for (size_t n : sizes)
        for (size_t hop : hops)
            for (bool smooth : {false, true})
                for (const std::string state : {"steady", "live"}) {
                    Config c;
                    c.n = n; c.hop = hop; c.rate = 48000; c.smooth = smooth; c.state = state;
                    c.backend = "legacy-batch-float";
                    Core<T> core(c);
                    Legacy<T> batch(c);
                    c.backend = "legacy-incremental-float";
                    Legacy<T> incremental(c);
                    for (size_t i = 0; i < ((n+hop-1)/hop+4)*hop; ++i) {
                        const float value = input[i%input.size()];
                        core.process(value); batch.process(value); incremental.process(value);
                        require(batch.published() == (i%hop == batch.delay()), "Batch publication delay");
                        require(incremental.published() == (i%hop == incremental.delay()), "Incremental publication delay");
                        if (!core.published()) continue;
                        for (size_t k = 0; k < core.output.size(); ++k) {
                            const double tolerance = (sizeof(T) == 4 ? 3e-4 : 1e-10)*std::max(1., double(batch.output[k]));
                            require(std::abs(core.output[k]-batch.output[k]) <= tolerance, "Matched core/batch spectrum differs");
                            require(std::abs(batch.output[k]-incremental.output[k]) <= tolerance, "Matched schedules differ");
                        }
                    }
                }
}

template<typename T>
void buffer_transform(Fourier::OnTheFlyRFFT<T>& fft, const std::vector<T>& real,
        const std::vector<std::complex<T>>&, const std::vector<float>& window) {
    fft.buffer(real.data(), window);
}
template<typename T, typename Transform>
void buffer_transform(Transform& fft, const std::vector<T>&,
        const std::vector<std::complex<T>>& complex, const std::vector<float>&) {
    fft.buffer(complex.data());
}

/// @brief All-bin fixtures shared with future canonical-layout adapters.
template<typename T, typename Transform>
void verify_transform(bool real, bool inverse) {
    Reference::transforms<T>([&](const std::vector<std::complex<T>>& input) {
        Transform fft(input.size());
        std::vector<T> values;
        for (auto value : input) values.push_back(value.real());
        const std::vector<float> unity(input.size(), 1.f);
        buffer_transform(fft, values, input, unity);
        fft.compute();
        return fft.coefficients;
    }, real, inverse);
}

/// @brief Independent magnitudes, startup and live window changes; no FFT oracle.
template<typename T>
void verify_analyzer() {
    Config c{};
    c.n = 128; c.hop = 37; c.rate = 48000;
    Core<T> core(c);
    const auto input = signal();
    bool blackman_harris = false;
    size_t stores = 0;
    for (size_t sample = 0; sample < 12*c.hop; ++sample) {
        if (sample%c.hop == 0) {
            blackman_harris = (sample/c.hop)%2;
            core.settings.window = blackman_harris ? Fourier::Window::Function::BlackmanHarris
                                                  : Fourier::Window::Function::Hann;
            require(core.analysis.configure(core.settings), "Reference settings rejected");
            stores = 0;
        }
        const bool published = core.analysis.process(T(input[sample%input.size()]), [&](size_t k, T value) {
            require(k == stores++, "Missing or reordered analyzer output");
            core.output[k] = value;
        });
        require(published == (sample%c.hop == c.hop-1), "Independent analyzer cadence");
        if (!published) continue;
        require(stores == c.n/2+1, "Incomplete analyzer output");
        const auto expected = Reference::magnitudes(input, sample-(c.hop-1), c.n, blackman_harris);
        // Window coefficients are float in both production precisions.
        for (size_t k = 0; k < expected.size(); ++k)
            require(std::abs(core.output[k]-expected[k]) <= 2e-5L*std::max(1.L, expected[k]),
                "Independent analyzer magnitude differs");
    }
}

/// @brief Time a phase without a type-erased call inside the interval.
struct PhaseTimer {
    std::vector<Row>& rows;
    template<typename Operation>
    void operator()(const std::string& phase, size_t frame, size_t first,
            size_t count, Operation operation) {
        const auto start = Clock::now();
        operation();
        asm volatile("" : : : "memory");
        const auto end = Clock::now();
        rows.emplace_back(phase, frame, 0, first, count, elapsed(start, end));
    }
};

/// @brief Legacy transform phases; no alternate-library implementation is used.
/// All total-frame passes include buffer preparation and final output work.
template<typename T, typename Transform>
void transform(const Config& c, bool real, bool inverse) {
    Runtime::set(Runtime::Phase::Setup);
    const auto input = signal();
    std::vector<T> values(c.n);
    std::vector<std::complex<T>> complex(c.n);
    for (size_t i = 0; i < c.n; ++i) {
        values[i] = input[i];
        complex[i] = {T(input[i]), T(input[(i+17)%input.size()])};
    }
    Fourier::Window::CachedWindow<float> window(Fourier::Window::Function::Hann, c.n, false, true);
    if (c.resources) {
        PaperResources::inspect<Transform>([&]() { return new Transform(c.n); }, [&](Transform& item) {
            for (size_t i = 0; i < 2; ++i) {
                buffer_transform(item, values, complex, window.get_samples());
                item.compute(); observe(item.coefficients.data());
            }
        }, 2);
        return;
    }
    Runtime::set(Runtime::Phase::TimedSetup);
    Transform fft(c.n);
    auto buffer = [&]() { buffer_transform(fft, values, complex, window.get_samples()); };
    const size_t steps = fft.get_total_steps();
    size_t expected_steps = inverse ? c.n : 0;
    for (size_t width = real ? c.n/2 : c.n; width > 1; width /= 2)
        expected_steps += real ? c.n/4 : c.n/2;
    require(steps == expected_steps, "Transform work count changed");
    const size_t butterflies = steps - (inverse ? c.n : 0);
    std::vector<Row> rows;
    rows.reserve(c.callbacks*(c.pass == "steps" ? steps+1 : 4)+1024);
    PhaseTimer timed{rows};
    Runtime::set(Runtime::Phase::TimerCalibration);
    for (size_t i = 0; i < 1024; ++i) {
        const auto start = Clock::now();
        observe(fft);
        const auto end = Clock::now();
        rows.emplace_back("timer", i, 0, 0, 0, elapsed(start, end));
    }
    Execution::settle();
    Runtime::set(Runtime::Phase::TimedWarmup);
    for (size_t i = 0; i < c.warm_hops; ++i) { buffer(); fft.compute(); }
    Runtime::set(Runtime::Phase::Measurement);
    for (size_t frame = 0; frame < c.callbacks; ++frame) {
        if (c.pass == "complete" || c.pass == "incremental") {
            timed(c.pass, frame, 0, steps, [&]() {
                buffer();
                if (c.pass == "complete") fft.compute();
                else while (!fft.is_done_computing()) fft.step(c.hop);
            });
        } else {
            timed("buffer", frame, 0, c.n, buffer);
            if (c.pass == "steps") {
                for (size_t step = 0; step < steps; ++step) {
                    const std::string phase = inverse && step >= butterflies ? "normalize_step" :
                        real && step+1 == steps ? "reconstruct_step" : "butterfly_step";
                    timed(phase, frame, step, 1, [&]() { fft.step(); });
                }
            } else {
                timed("butterflies", frame, 0, butterflies-(real ? 1 : 0), [&]() {
                    for (size_t i = 0; i < butterflies-(real ? 1 : 0); ++i) fft.step();
                });
                if (real) timed("last_butterfly_and_reconstruction", frame, butterflies-1, 1,
                    [&]() { fft.step(); });
                if (inverse) timed("normalization", frame, butterflies, c.n,
                    [&]() { for (size_t i = 0; i < c.n; ++i) fft.step(); });
            }
        }
        require(fft.is_done_computing(), "Transform step count did not complete");
    }
    Execution::measured();
    Runtime::set(Runtime::Phase::CorrectnessReplay);
    const auto measured = fft.coefficients;
    buffer();
    size_t calls = 0;
    while (!fft.is_done_computing()) { fft.step(c.hop); ++calls; }
    const size_t quota = (steps+c.hop-1)/c.hop;
    require(calls == (steps+quota-1)/quota, "Incremental transform call count changed");
    for (size_t i = 0; i < c.n; ++i) {
        require(std::isfinite(fft.coefficients[i].real()) && std::isfinite(fft.coefficients[i].imag()),
            "Non-finite transform output");
        require(std::abs(fft.coefficients[i]-measured[i]) < (sizeof(T) == 4 ? 1e-4 : 1e-10),
            "Timed and incremental transform outputs differ");
    }
    // Independent long-double direct sums at spread-out bins, including DC and
    // Nyquist. This checks sign/normalization without sharing the FFT algorithm.
    long double maximum_error = 0, maximum_reference = 0;
    for (size_t probe = 0; probe < 17; ++probe) {
        const size_t k = probe == 16 ? c.n-1 : probe*c.n/16;
        std::complex<long double> reference(0, 0);
        for (size_t j = 0; j < c.n; ++j) {
            const long double angle = (inverse ? 2 : -2)*std::acos(-1.L)*k*j/c.n;
            const auto value = real ? std::complex<long double>(values[j]*window.get_samples()[j], 0)
                                    : std::complex<long double>(complex[j]);
            reference += value * std::complex<long double>(std::cos(angle), std::sin(angle));
        }
        if (inverse) reference /= c.n;
        maximum_error = std::max(maximum_error, std::abs(std::complex<long double>(fft.coefficients[k])-reference));
        maximum_reference = std::max(maximum_reference, std::abs(reference));
    }
    require(maximum_error/std::max(1.L, maximum_reference) < (sizeof(T) == 4 ? 2e-5 : 1e-10),
        "Independent transform reference failed");
    long double roundtrip_error = 0;
    if (inverse) {
        Fourier::OnTheFlyFFT<T> forward(c.n);
        forward.buffer(fft.coefficients.data());
        forward.compute();
        for (size_t i = 0; i < c.n; ++i)
            roundtrip_error = std::max(roundtrip_error,
                Reference::absolute_error(Reference::Complex(forward.coefficients[i]), Reference::Complex(complex[i])));
    } else {
        Fourier::OnTheFlyIFFT<T> backward(c.n);
        backward.buffer(fft.coefficients.data());
        backward.compute();
        for (size_t i = 0; i < c.n; ++i) {
            const auto expected = real ? std::complex<long double>(values[i]*window.get_samples()[i], 0)
                                       : std::complex<long double>(complex[i]);
            roundtrip_error = std::max(roundtrip_error,
                Reference::absolute_error(Reference::Complex(backward.coefficients[i]), expected));
        }
    }
    require(roundtrip_error < (sizeof(T) == 4 ? 2e-5 : 1e-10), "Transform round trip failed");
    // stderr is an independent numerical report, retained with the raw CSV.
    std::cerr.precision(17);
    std::cerr << "{\"reference\":\"17-bin long-double direct sum\",\"max_abs_error\":"
        << double(maximum_error) << ",\"max_reference\":" << double(maximum_reference)
        << ",\"roundtrip_max_abs_error\":" << double(roundtrip_error)
        << ",\"scheduled_calls\":" << calls << ",\"steps\":" << steps << "}\n";
    Runtime::set(Runtime::Phase::Other);
    print(rows);
}

}  // namespace Paper

#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_FOURIER_HPP_
