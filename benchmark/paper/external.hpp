// Benchmark-only workloads for opaque FFT providers; never included by modules.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_EXTERNAL_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_EXTERNAL_HPP_
#include <map>
#include "synthesis.hpp"
#include "../../src/dsp/window.hpp"

namespace Paper {
/// @brief Immediate batch analysis with cached float windows and band intervals.
/// @details Preparation, conversion, magnitudes, smoothing and K stores are timed.
template<typename T, typename Backend>
struct ExternalAnalysis {
    Config config;
    Backend fft;
    Fourier::Window::CachedWindow<float> window;
    std::vector<T> ring, frame, magnitudes, prefix, output;
    std::vector<std::complex<T>> coefficients;
    std::vector<size_t> low, high;
    size_t head = 0, phase = 0, frames = 0;
    bool complete = false, bands;
    explicit ExternalAnalysis(const Config& c) : config(c), fft(c.n, "analysis"),
        window(Fourier::Window::Function::Hann, c.n, false, true), ring(c.n), frame(c.n),
        magnitudes(c.n/2+1), prefix(c.n/2+2), output(c.n/2+1), coefficients(c.n/2+1),
        low(c.n/2+1), high(c.n/2+1), bands(c.smooth) { prepare_bands(); }
    // Binary64 interval arithmetic avoids float floor boundaries changing when
    // the compiler vectorizes this loop versus the hybrid's single-bin task.
    void prepare_bands() {
        const float half = std::pow(2.f, (1.f/3.f)/2.f), ratio = std::pow(2.f, 1.f/3.f);
        const double width = double(config.rate)/config.n, maximum = double(config.rate)/2;
        for (size_t k = 0; k < low.size(); ++k) {
            double a = k*width/half, b = k*width*half;
            if (b > maximum) { b = maximum; a = b/ratio; }
            low[k] = size_t(std::floor(a/width));
            high[k] = std::min(config.n/2, size_t(std::floor(b/width)));
        }
    }
    void process(float value) {
        ring[head] = T(value); head = (head+1)%ring.size();
        complete = phase == 0;
        if (complete) {
            if (config.state == "live") {
                window.set_window(frames++%2 ? Fourier::Window::Function::Hann
                    : Fourier::Window::Function::BlackmanHarris, config.n, false, true);
                bands = frames%2 == 0;
                prepare_bands();
            }
            for (size_t i = 0; i < ring.size(); ++i)
                frame[i] = ring[(head+i)%ring.size()]*window.get_samples()[i];
            fft.forward_real_positive(frame.data(), coefficients.data());
            prefix[0] = 0;
            for (size_t k = 0; k < output.size(); ++k) {
                magnitudes[k] = std::abs(coefficients[k]);
                prefix[k+1] = prefix[k]+magnitudes[k];
            }
            const float alpha = config.smooth ? 0.8f : 0.f;
            for (size_t k = 0; k < output.size(); ++k) {
                const T magnitude = bands ? (prefix[high[k]+1]-prefix[low[k]])/T(high[k]-low[k]+1)
                                          : magnitudes[k];
                output[k] = alpha*output[k]+(1.f-alpha)*magnitude;
            }
        }
        phase = (phase+1)%config.hop;
    }
    std::string info_json() const { return fft.info_json(); }
    size_t delay() const { return 0; }
    bool published() const { return complete; }
    void barrier() const { observe(output.data()); }
    void check() const { for (auto value : output) require(std::isfinite(value), "Invalid external analysis output"); }
};

/// @brief Released inverse spectra; canonical full-complex stores belong to the job.
template<typename T, typename Backend>
struct ExternalInverse {
    Backend fft;
    std::vector<std::complex<T>> spectra[2], expected[2], output;
    FrameSchedule schedule;
    size_t fixture = 0, frame = 0;
    explicit ExternalInverse(const Config& c) : fft(c.n, "inverse"), output(c.n), schedule(c, 1) {
        inverse_fixtures<T>(c.n, spectra, expected);
    }
    void process(float) {
        schedule.complete = schedule.phase == 0;
        if (schedule.complete) {
            fixture = frame++%2;
            fft.inverse_complex(spectra[fixture].data(), output.data());
        }
        schedule.advance();
    }
    std::string info_json() const { return fft.info_json(); }
    size_t delay() const { return 0; }
    bool published() const { return schedule.complete; }
    void barrier() const { observe(output.data()); }
    void check() const { for (auto value : output) require(std::isfinite(std::abs(value)), "Invalid external inverse"); }
};

/// @brief Immediate full-complex overlap-save with the same dependent audio sink.
template<typename T, typename Backend>
struct ExternalChain {
    using Complex = std::complex<T>;
    Backend fft;
    std::vector<Complex> ring, frame, spectrum, inverse, transfer, playing;
    FrameSchedule schedule;
    size_t head = 0, playback;
    bool identity;
    Complex audio_output = Complex(0, 0), delivered_checksum = Complex(0, 0);
    explicit ExternalChain(const Config& c) : fft(c.n, backend_descriptor(c.backend).operation),
        ring(c.n), frame(c.n), spectrum(c.n), inverse(c.n), transfer(c.n), playing(c.hop),
        schedule(c, 1), playback(c.hop), identity(std::string(backend_descriptor(c.backend).operation) == "identity") {
        require(c.hop <= c.n-2, "Overlap-save requires H <= N-2");
        for (size_t k = 0; k < c.n; ++k) {
            const long double angle = -2*std::acos(-1.L)*k/c.n;
            const std::complex<long double> z(std::cos(angle), std::sin(angle));
            transfer[k] = identity ? Complex(1, 0) : Complex(0.5L-0.25L*z+0.125L*z*z);
        }
    }
    void process(float value) {
        ring[head] = Complex(T(value), T(-0.25f*value)); head = (head+1)%ring.size();
        schedule.complete = schedule.phase == 0;
        if (schedule.complete) {
            for (size_t i = 0; i < ring.size(); ++i) frame[i] = ring[(head+i)%ring.size()];
            fft.forward_complex(frame.data(), spectrum.data());
            for (size_t k = 0; k < ring.size(); ++k)
                spectrum[k] = Fourier::complex_multiply(spectrum[k], transfer[k]);
            fft.inverse_complex(spectrum.data(), inverse.data());
            for (size_t k = 0; k < playing.size(); ++k) playing[k] = inverse[ring.size()-playing.size()+k];
            playback = 0;
        }
        audio_output = playback < playing.size() ? playing[playback++] : Complex(0, 0);
        delivered_checksum = T(0.999)*delivered_checksum+audio_output;
        schedule.advance();
    }
    std::string info_json() const { return fft.info_json(); }
    size_t delay() const { return 0; }
    size_t playback_delay() const { return playing.size()-1; }
    bool published() const { return schedule.complete; }
    void barrier() const { observe(playing.data()); observe(delivered_checksum); }
    void check() const { for (auto value : playing) require(std::isfinite(std::abs(value)), "Invalid external chain"); }
};

/// @brief Untimed scalar analysis oracle, independently summed bands and EMA.
/// @details Direct DFT at small N; independent binary64 complex FFT at large N.
/// Common float window/input bytes preserve the workload's precision contract.
template<typename T>
struct AnalysisReference {
    Config config;
    // Binary32 oracle error can exceed the adapter tolerance near weak bins
    // beside strong tones at large N. Widen after the matched frame product;
    // leave the measured providers and their existing tolerances unchanged.
    Fourier::OnTheFlyFFT<double> forward;
    Fourier::Window::CachedWindow<float> window;
    std::vector<std::complex<double>> frame;
    std::vector<T> expected;
    size_t next_endpoint = 0;
    explicit AnalysisReference(const Config& c) : config(c), forward(c.n),
        window(Fourier::Window::Function::Hann, c.n, false, true), frame(c.n), expected(c.n/2+1) {}
    void advance(const std::vector<float>& input, size_t endpoint) {
        for (; next_endpoint <= endpoint; next_endpoint += config.hop) {
            const size_t index = next_endpoint/config.hop;
            const bool live = config.state == "live";
            window.set_window(live && index%2 == 0 ? Fourier::Window::Function::BlackmanHarris
                : Fourier::Window::Function::Hann, config.n, false, true);
            for (size_t i = 0; i < config.n; ++i) {
                const int64_t source = int64_t(next_endpoint)-int64_t(config.n)+1+int64_t(i);
                frame[i] = source < 0 ? T(0) : T(input[size_t(source)%input.size()])*window.get_samples()[i];
            }
            std::vector<long double> magnitudes(expected.size());
            if (config.n <= 256) {
                const std::vector<Reference::Complex> precise(frame.begin(), frame.end());
                for (size_t k = 0; k < expected.size(); ++k)
                    magnitudes[k] = std::abs(Reference::coefficient(precise, k, false));
            } else {
                forward.buffer(frame.data()); forward.compute();
                for (size_t k = 0; k < expected.size(); ++k) magnitudes[k] = std::abs(forward.coefficients[k]);
            }
            const bool bands = live ? index%2 == 1 : config.smooth;
            const float alpha = config.smooth ? 0.8f : 0.f;
            const double width = double(config.rate)/config.n, maximum = double(config.rate)/2;
            for (size_t k = 0; k < expected.size(); ++k) {
                long double value = magnitudes[k];
                if (bands) {
                    double low = k*width/std::pow(2.f, (1.f/3.f)/2.f);
                    double high = k*width*std::pow(2.f, (1.f/3.f)/2.f);
                    if (high > maximum) { high = maximum; low = high/std::pow(2.f, 1.f/3.f); }
                    const size_t first = size_t(std::floor(low/width));
                    const size_t last = std::min(config.n/2, size_t(std::floor(high/width)));
                    value = 0;
                    for (size_t j = first; j <= last; ++j) value += magnitudes[j];
                    value /= last-first+1;
                }
                expected[k] = T(alpha*expected[k]+(1.f-alpha)*value);
            }
        }
    }
};

/// @brief All required outputs checked on the independent untimed replay.
template<typename T, typename Backend>
struct ExternalAudit {
    SynthesisAccuracy& accuracy;
    std::vector<std::string>& instances;
    std::map<const void*, std::unique_ptr<AnalysisReference<T>>> references;
    ExternalAudit(SynthesisAccuracy& a, std::vector<std::string>& p) : accuracy(a), instances(p) {}
    template<typename Adapter> void timed_instance(const Adapter& adapter) { instances.push_back(adapter.info_json()); }
    ExternalAudit(ExternalAudit&&) = default;
    void operator()(const ExternalInverse<T, Backend>& adapter, const std::vector<float>& input, size_t sample) {
        SynthesisAudit{accuracy}.inverse(adapter, input, sample);
    }
    void operator()(const ExternalChain<T, Backend>& adapter, const std::vector<float>& input, size_t sample) {
        SynthesisAudit{accuracy}.template chain<T>(adapter, input, sample);
    }
    void operator()(const ExternalAnalysis<T, Backend>& adapter, const std::vector<float>& input, size_t sample) {
        analysis(adapter, input, sample);
    }
    template<typename Adapter>
    void analysis(const Adapter& adapter, const std::vector<float>& input, size_t sample) {
        if (!adapter.published()) return;
        auto& reference = references[&adapter];
        if (!reference) reference.reset(new AnalysisReference<T>(adapter.config));
        reference->advance(input, sample-adapter.delay());
        require(adapter.output.size() == reference->expected.size(), "Missing external analysis output");
        ++accuracy.publications;
        for (size_t k = 0; k < adapter.output.size(); ++k) {
            const double error = std::abs(double(adapter.output[k])-double(reference->expected[k]));
            const double scale = std::abs(double(reference->expected[k]));
            require(std::isfinite(error) && error <= (sizeof(T) == 4 ? 3e-4 : 1e-10)*std::max(1., scale),
                "Independent external analyzer differs");
            accuracy.maximum_error = std::max(accuracy.maximum_error, error);
            accuracy.maximum_reference = std::max(accuracy.maximum_reference, scale);
            ++accuracy.checked;
        }
    }
};

/// @brief Buffered transform including input/window preparation and canonical stores.
template<typename T, typename Backend>
struct ExternalTransform {
    Backend fft;
    std::vector<T> input, prepared;
    std::vector<std::complex<T>> complex, output;
    Fourier::Window::CachedWindow<float> window;
    bool real, inverse;
    explicit ExternalTransform(const Config& c) : fft(c.n, backend_descriptor(c.backend).operation),
        input(c.n), prepared(c.n), complex(c.n), output(c.n),
        window(Fourier::Window::Function::Hann, c.n, false, true),
        real(std::string(backend_descriptor(c.backend).operation) == "rfft"),
        inverse(std::string(backend_descriptor(c.backend).operation) == "ifft") {
        const auto values = signal();
        for (size_t i = 0; i < c.n; ++i) { input[i] = T(values[i]); complex[i] = {T(values[i]), T(values[(i+17)%values.size()])}; }
    }
    void compute() {
        if (real) {
            for (size_t i = 0; i < input.size(); ++i) prepared[i] = input[i]*window.get_samples()[i];
            fft.forward_real(prepared.data(), output.data());
        } else if (inverse) fft.inverse_complex(complex.data(), output.data());
        else fft.forward_complex(complex.data(), output.data());
        observe(output.data());
    }
};

template<typename T, typename Backend>
void external_transform(const Config& c) {
    using Job = ExternalTransform<T, Backend>;
    if (c.resources) {
        // Match first-party transform resource scope: plans and owned output;
        // signal/window/caller input fixtures are prepared outside the probe.
        const auto input = signal();
        const std::string operation(backend_descriptor(c.backend).operation);
        std::vector<T> real(c.n), prepared(c.n);
        std::vector<std::complex<T>> complex(c.n);
        Fourier::Window::CachedWindow<float> window(Fourier::Window::Function::Hann, c.n, false, true);
        for (size_t i = 0; i < c.n; ++i) { real[i] = input[i]; complex[i] = {T(input[i]), T(input[(i+17)%input.size()])}; }
        struct Resource {
            Backend fft;
            std::vector<std::complex<T>> output;
            Resource(size_t n, const std::string& operation) : fft(n, operation), output(n) {}
            std::string info_json() const { return fft.info_json(); }
        };
        PaperResources::inspect<Resource>([&]() { return new Resource(c.n, operation); }, [&](Resource& job) {
            for (size_t i = 0; i < 2; ++i) {
                if (operation == "rfft") {
                    for (size_t j = 0; j < c.n; ++j) prepared[j] = real[j]*window.get_samples()[j];
                    job.fft.forward_real(prepared.data(), job.output.data());
                } else if (operation == "ifft") job.fft.inverse_complex(complex.data(), job.output.data());
                else job.fft.forward_complex(complex.data(), job.output.data());
                observe(job.output.data());
            }
        }, 2);
        return;
    }
    Job job(c);
    std::vector<Row> rows;
    rows.reserve(1024+c.callbacks);
    for (size_t i = 0; i < 1024; ++i) {
        const auto start = Clock::now(); observe(job); const auto end = Clock::now();
        rows.emplace_back("timer", i, 0, 0, 0, elapsed(start, end));
    }
    for (size_t i = 0; i < c.warm_hops; ++i) job.compute();
    for (size_t i = 0; i < c.callbacks; ++i) {
        const auto start = Clock::now(); job.compute(); const auto end = Clock::now();
        rows.emplace_back("complete", i, 0, 0, c.n, elapsed(start, end));
    }
    for (const auto value : job.output)
        require(std::isfinite(value.real()) && std::isfinite(value.imag()), "Non-finite external transform output");
    std::vector<std::complex<T>> canonical = job.complex;
    if (job.real) for (size_t i = 0; i < c.n; ++i) canonical[i] = job.prepared[i];
    const std::vector<Reference::Complex> precise(canonical.begin(), canonical.end());
    double error = 0, scale = 0, roundtrip = 0;
    const size_t probes = c.n <= 256 ? c.n : 17;
    for (size_t p = 0; p < probes; ++p) {
        const size_t k = probes == c.n ? p : p == 16 ? c.n-1 : p*c.n/16;
        const auto expected = Reference::coefficient(precise, k, job.inverse);
        error = std::max(error, double(std::abs(Reference::Complex(job.output[k])-expected)));
        scale = std::max(scale, double(std::abs(expected)));
    }
    // Independent implementation for all bins, plus opposite-transform recovery.
    Fourier::OnTheFlyFFT<T> forward(c.n);
    Fourier::OnTheFlyIFFT<T> backward(c.n);
    if (job.inverse) { backward.buffer(canonical.data()); backward.compute(); }
    else { forward.buffer(canonical.data()); forward.compute(); }
    const auto& expected = job.inverse ? backward.coefficients : forward.coefficients;
    for (size_t k = 0; k < c.n; ++k) {
        error = std::max(error, double(std::abs(job.output[k]-expected[k])));
        scale = std::max(scale, double(std::abs(expected[k])));
    }
    if (job.inverse) { forward.buffer(job.output.data()); forward.compute(); }
    else { backward.buffer(job.output.data()); backward.compute(); }
    const auto& recovered = job.inverse ? forward.coefficients : backward.coefficients;
    for (size_t k = 0; k < c.n; ++k) {
        require(std::isfinite(recovered[k].real()) && std::isfinite(recovered[k].imag()), "Non-finite independent roundtrip");
        roundtrip = std::max(roundtrip, double(std::abs(recovered[k]-canonical[k])));
    }
    require(error <= Reference::tolerance<T>()*std::max(1., scale) && roundtrip < Reference::tolerance<T>(),
        "Independent external transform differs");
    std::cerr.precision(17);
    std::cerr << "{\"reference\":\"direct DFT plus independent first-party all-bin FFT\",\"max_abs_error\":" << error
        << ",\"max_reference\":" << scale << ",\"roundtrip_max_abs_error\":" << roundtrip
        << ",\"checked_bins\":" << c.n << ",\"direct_bins\":" << probes
        << ",\"provider_instances\":[" << job.fft.info_json() << "]}\n";
    print(rows);
}

template<typename T, typename Backend>
void external_dispatch(const Config& c, bool provider_info = false) {
    const auto& d = backend_descriptor(c.backend);
    if (provider_info) { Backend backend(c.n, d.operation); std::cout << backend.info_json() << '\n'; return; }
    const std::string boundary(d.boundary);
    if (boundary == "transform") { external_transform<T, Backend>(c); return; }
    SynthesisAccuracy accuracy;
    std::vector<std::string> instances;
    if (boundary == "analysis") stream<ExternalAnalysis<T, Backend>>(c, ExternalAudit<T, Backend>(accuracy, instances));
    else if (boundary == "inverse-job") stream<ExternalInverse<T, Backend>>(c, ExternalAudit<T, Backend>(accuracy, instances));
    else if (boundary == "chain") stream<ExternalChain<T, Backend>>(c, ExternalAudit<T, Backend>(accuracy, instances), (c.hop-1)/2., c.hop-1);
    else require(false, "Unsupported external boundary");
    if (!c.resources) {
        require(accuracy.checked && accuracy.publications, "Missing external numerical audit");
        accuracy.print(boundary == "analysis" ? "direct DFT or binary64 FFT of matched frame bytes; direct band sums and EMA"
            : "analytical complex inverse or direct time-domain FIR", instances);
    }
}

/// @brief Preflight every registered streaming operation across startup/wraparound/settings.
template<typename T, typename Backend>
void verify_external(const std::string& provider, const std::string& precision) {
    const auto input = signal();
    for (size_t n : {128u, 2048u, 16384u}) {
        for (const std::string suffix : {"analysis", "inverse", "ols-identity", "ols-fir"}) {
            Config c{};
            c.n = n; c.hop = n == 128 ? 37 : n/2; c.rate = 48000;
            c.backend = provider+"-"+suffix+"-"+precision;
            SynthesisAccuracy accuracy;
            std::vector<std::string> instances;
            ExternalAudit<T, Backend> audit(accuracy, instances);
            if (suffix == "analysis") {
                for (const std::string state : {"steady", "live"}) {
                    c.state = state; c.smooth = true;
                    ExternalAnalysis<T, Backend> adapter(c);
                    for (size_t sample = 0; sample < 2*n+4*c.hop; ++sample) {
                        adapter.process(input[sample%input.size()]); audit(adapter, input, sample);
                        require(adapter.published() == (sample%c.hop == 0), "External analyzer cadence");
                    }
                    audit.references.clear();
                }
            } else if (suffix == "inverse") {
                ExternalInverse<T, Backend> adapter(c);
                for (size_t sample = 0; sample < 4*c.hop; ++sample) {
                    adapter.process(0); audit(adapter, input, sample);
                    require(adapter.published() == (sample%c.hop == 0), "External inverse cadence");
                }
            } else {
                ExternalChain<T, Backend> adapter(c);
                for (size_t sample = 0; sample < 2*n+4*c.hop; ++sample) {
                    adapter.process(input[sample%input.size()]); audit(adapter, input, sample);
                    require(adapter.published() == (sample%c.hop == 0), "External chain cadence");
                }
            }
        }
    }
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_EXTERNAL_HPP_
