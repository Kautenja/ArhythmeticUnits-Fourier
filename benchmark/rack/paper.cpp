// Publication experiments using current production code and the common protocol.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <complex>
#include <cstdlib>
#include <limits>
#include "../../src/SpectrumAnalyzer.cpp"
#include "../../src/Spectrogram.cpp"
#include "../paper/protocol.hpp"
#include "../paper/synthesis.hpp"
#include "../paper/external.hpp"
#include "../paper/providers/pffft.hpp"
#ifdef PAPER_HAVE_VDSP
#include "../paper/providers/vdsp.hpp"
#endif
#ifdef PAPER_HAVE_FFTW
#include "../paper/providers/fftw.hpp"
#endif

Plugin* plugin_instance = nullptr;

namespace Paper {
/// @brief Headless module context; no engine worker or display thread is started.
struct Context {
    rack::Context context;
    explicit Context(float rate) {
        rack::contextSet(&context);
        context.engine = new rack::engine::Engine;
        context.engine->setSampleRate(rate);
    }
    ~Context() { rack::contextSet(nullptr); }
};

template<typename T> T lanes(float value) { return T(value); }
template<> simd::float_4 lanes(float value) {
    return simd::float_4(value, value*0.75f, value*0.5f, value*0.25f);
}
template<typename T> double sum(T value) { return value; }
template<> double sum(simd::float_4 value) { return value[0]+value[1]+value[2]+value[3]; }

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
    void process(float input) {
        if (live && analysis.is_frame_start()) {
            // Rebuild both caches every frame, retaining the requested cadence.
            settings.window = frames++%2 ? Fourier::Window::Function::Hann
                                        : Fourier::Window::Function::BlackmanHarris;
            settings.octave = frames%2 ? 0.f : 1.f/3.f;
            analysis.configure(settings);
        }
        complete = analysis.process(lanes<T>(input), [this](size_t bin, T magnitude) {
            output[bin] = magnitude;
        });
    }
    size_t delay() const { return settings.hop-1; }
    bool published() { return complete; }
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
    bool published() { return complete; }
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

/// @brief Harness/load control, labeled separately from spectral computation.
struct Driver {
    float output = 0;
    explicit Driver(const Config&) {}
    void process(float value) { output = 0.999f*output + value; }
    bool published() { return false; }
    size_t delay() const { return 0; }
    void barrier() const { observe(output); }
    void check() const { require(std::isfinite(output), "Invalid driver output"); }
};

/// @brief Compare matched frames despite their different publication delays.
template<typename T>
void verify_controls() {
    const auto input = signal();
    for (size_t n : {128u, 2048u, 16384u})
        for (size_t hop : {257u, 1024u})
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

void configure(SpectrumAnalyzer& module, const Config& c) {
    module.set_window_length(c.n);
    module.set_hop_length(c.hop);
    require(module.get_window_length() == c.n && module.get_hop_length() == c.hop,
        "Fourier panel quantized N/H; choose exactly representable settings");
}
void configure(Spectrogram& module, const Config& c) {
    require(c.n == Spectrogram::N_FFT && c.hop == Spectrogram::N_FFT/2,
        "Spectre has fixed N=2048 H=1024");
}

/// @brief Publication detection is consumer-side and only used by untimed audit.
struct Publication {
    const void* last = nullptr;
    uint32_t column = 0;
    bool poll(SpectrumAnalyzer& module) {
        const auto& snapshot = module.consume_display_spectrum();
        const bool changed = last != &snapshot && snapshot.count != 0;
        last = &snapshot;
        return changed;
    }
    bool poll(Spectrogram& module) {
        const auto next = module.get_hop_index();
        const bool changed = next != column;
        column = next;
        return changed;
    }
};
void check(SpectrumAnalyzer& module, size_t n) {
    const auto& snapshot = module.consume_display_spectrum();
    require(snapshot.count == n/2+1, "Wrong module spectrum size");
    for (const auto& lane : snapshot.points)
        require(std::isfinite(lane[7].y), "Empty module spectrum");
}
void check(Spectrogram& module, size_t) {
    const auto index = (module.get_hop_index()+Spectrogram::N_STFT-1)%Spectrogram::N_STFT;
    const auto* column = module.consume_display_column(index, true);
    require(column && column->revision > 0 && column->values[7] > 0, "Empty module history");
}

template<typename Module>
struct Host {
    Module module;
    Publication publication;
    rack::engine::Module::ProcessArgs args = {};
    Config config;
    size_t phase = 0, frames = 0;
    explicit Host(const Config& c) : config(c) {
        configure(module, c);
        module.set_window_function(Fourier::Window::Function::Hann);
        module.set_frequency_smoothing(c.smooth ? FrequencySmoothing::_1_3 : FrequencySmoothing::None);
        module.set_time_smoothing(c.smooth ? 0.1f : 0.f);
        for (auto& input : module.inputs) input.channels = c.voices;
        args.sampleRate = c.rate;
        args.sampleTime = 1.f/c.rate;
        publication.poll(module);
    }
    void process(float value) {
        if (config.state == "live" && phase == 0) {
            module.set_window_function(frames++%2 ? Fourier::Window::Function::Hann
                                                 : Fourier::Window::Function::BlackmanHarris);
            module.set_frequency_smoothing(frames%2 ? FrequencySmoothing::None : FrequencySmoothing::_1_3);
        }
        for (size_t port = 0; port < module.inputs.size(); ++port)
            for (size_t voice = 0; voice < config.voices; ++voice)
                module.inputs[port].setVoltage(5.f*value*(1.f-0.15f*port)/config.voices, voice);
        module.process(args);
        ++args.frame;
        phase = (phase+1)%config.hop;
    }
    size_t delay() const { return config.hop-1; }
    bool published() { return publication.poll(module); }
    void barrier() const { observe(module); }
    void check() { Paper::check(module, config.n); }
};

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
    for (size_t i = 0; i < 1024; ++i) {
        const auto start = Clock::now();
        observe(fft);
        const auto end = Clock::now();
        rows.emplace_back("timer", i, 0, 0, 0, elapsed(start, end));
    }
    for (size_t i = 0; i < c.warm_hops; ++i) { buffer(); fft.compute(); }
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
                std::abs(std::complex<long double>(forward.coefficients[i])-std::complex<long double>(complex[i])));
    } else {
        Fourier::OnTheFlyIFFT<T> backward(c.n);
        backward.buffer(fft.coefficients.data());
        backward.compute();
        for (size_t i = 0; i < c.n; ++i) {
            const auto expected = real ? std::complex<long double>(values[i]*window.get_samples()[i], 0)
                                       : std::complex<long double>(complex[i]);
            roundtrip_error = std::max(roundtrip_error,
                std::abs(std::complex<long double>(backward.coefficients[i])-expected));
        }
    }
    require(roundtrip_error < (sizeof(T) == 4 ? 2e-5 : 1e-10), "Transform round trip failed");
    // stderr is an independent numerical report, retained with the raw CSV.
    std::cerr.precision(17);
    std::cerr << "{\"reference\":\"17-bin long-double direct sum\",\"max_abs_error\":"
        << double(maximum_error) << ",\"max_reference\":" << double(maximum_reference)
        << ",\"roundtrip_max_abs_error\":" << double(roundtrip_error)
        << ",\"scheduled_calls\":" << calls << ",\"steps\":" << steps << "}\n";
    print(rows);
}

size_t integer(const char* value) {
    char* end = nullptr;
    const auto result = std::strtoull(value, &end, 10);
    require(*value && *value != '-' && end && !*end && result <= (1u << 28), "Invalid integer argument");
    return result;
}
}  // namespace Paper

int main(int argc, char** argv) {
    using namespace Paper;
    try {
        if (argc == 2 && std::string(argv[1]) == "--verify") {
            verify_transform<float, Fourier::OnTheFlyFFT<float>>(false, false);
            verify_transform<double, Fourier::OnTheFlyFFT<double>>(false, false);
            verify_transform<float, Fourier::OnTheFlyRFFT<float>>(true, false);
            verify_transform<double, Fourier::OnTheFlyRFFT<double>>(true, false);
            verify_transform<float, Fourier::OnTheFlyIFFT<float>>(false, true);
            verify_transform<double, Fourier::OnTheFlyIFFT<double>>(false, true);
            verify_analyzer<float>();
            verify_analyzer<double>();
            verify_controls<float>();
            verify_controls<double>();
            verify_synthesis<float>();
            verify_synthesis<double>();
            verify_external<float, PffftBackend>("pffft", "float");
#ifdef PAPER_HAVE_VDSP
            verify_external<float, VdspBackend<float>>("vdsp", "float");
            verify_external<double, VdspBackend<double>>("vdsp", "double");
#endif
#ifdef PAPER_HAVE_FFTW
            verify_external<float, FftwBackend<float>>("fftw", "float");
            verify_external<double, FftwBackend<double>>("fftw", "double");
#endif
            std::cout << "Independent transform/analyzer fixtures and matched analysis frames verified for 48 configurations and two controls; "
                << "inverse jobs and overlap-save identity/FIR verified in both precisions\n";
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--inventory") {
            std::cout << registry_json << '\n'; return 0;
        }
        bool describe = false, resources = false, provider_info = false;
        if (argc == 18) {
            describe = std::string(argv[1]) == "--describe";
            resources = std::string(argv[1]) == "--resources";
            provider_info = std::string(argv[1]) == "--provider-info";
            require(describe || resources || provider_info, "Unknown command");
            --argc; ++argv;
        }
        require(argc == 17, "Use benchmark/paper/run.py; expected 16 protocol arguments");
        Config c;
        c.backend = argv[1]; c.pass = argv[2]; c.n = integer(argv[3]); c.hop = integer(argv[4]);
        c.block = integer(argv[5]); c.count = integer(argv[6]); c.alignment = argv[7];
        c.load = integer(argv[8]);
        const size_t smooth = integer(argv[9]);
        require(smooth <= 1, "Smoothing must be 0 or 1");
        c.smooth = smooth; c.voices = integer(argv[10]);
        c.callbacks = integer(argv[11]); c.warm_hops = integer(argv[12]); c.rate = integer(argv[13]);
        c.state = argv[14]; c.cache_mib = integer(argv[15]);
        require(std::string(argv[16]) == "v1", "Unknown protocol version");
        require(c.n >= 128 && c.n <= 16384 && !(c.n & (c.n-1)) && c.hop && c.hop <= 65536,
            "Invalid FFT length or hop");
        require(c.block && c.block <= 65536 && c.count && c.count <= 64 && c.callbacks
            && c.callbacks <= 1000000 && c.load <= 4096 && c.voices >= 1 && c.voices <= 16
            && c.rate >= 8000 && c.rate <= 192000 && c.cache_mib <= 256 && c.warm_hops <= 4096,
            "Workload outside protocol bounds");
        require(c.alignment == "aligned" || c.alignment == "staggered", "Invalid alignment");
        require(c.state == "steady" || c.state == "startup" || c.state == "live", "Invalid state");
        validate_backend(c);
        if (describe) { std::cout << contract_json(c) << '\n'; return 0; }
        c.resources = resources;
        const auto& descriptor = backend_descriptor(c.backend);
        const std::string kind(descriptor.kind), precision(descriptor.precision);
        require(!provider_info || kind == "external", "Provider metadata is only available for external adapters");
        Paper::Context context(c.rate);
        if (std::string(descriptor.provider) == "pffft") external_dispatch<float, PffftBackend>(c, provider_info);
#ifdef PAPER_HAVE_FFTW
        else if (std::string(descriptor.provider) == "fftw") {
            if (precision == "float") external_dispatch<float, FftwBackend<float>>(c, provider_info);
            else external_dispatch<double, FftwBackend<double>>(c, provider_info);
        }
#endif
#ifdef PAPER_HAVE_VDSP
        else if (std::string(descriptor.provider) == "vdsp") {
            if (precision == "float") external_dispatch<float, VdspBackend<float>>(c, provider_info);
            else external_dispatch<double, VdspBackend<double>>(c, provider_info);
        }
#endif
        else if (kind == "inverse-job" || kind == "chain") {
            if (precision == "float") synthesis_stream<float>(c);
            else synthesis_stream<double>(c);
        } else if (kind == "driver") stream<Driver>(c);
        else if (kind == "legacy") {
            if (precision == "float") stream<Legacy<float>>(c);
            else stream<Legacy<double>>(c);
        } else if (kind == "core") {
            if (descriptor.channels == 4) stream<Core<simd::float_4>>(c);
            else if (precision == "float") stream<Core<float>>(c);
            else stream<Core<double>>(c);
        } else if (kind == "fourier") stream<Host<SpectrumAnalyzer>>(c);
        else if (kind == "spectre") stream<Host<Spectrogram>>(c);
        else if (kind == "fft") {
            if (precision == "float") transform<float, Fourier::OnTheFlyFFT<float>>(c, false, false);
            else transform<double, Fourier::OnTheFlyFFT<double>>(c, false, false);
        } else if (kind == "rfft") {
            if (precision == "float") transform<float, Fourier::OnTheFlyRFFT<float>>(c, true, false);
            else transform<double, Fourier::OnTheFlyRFFT<double>>(c, true, false);
        } else if (kind == "ifft") {
            if (precision == "float") transform<float, Fourier::OnTheFlyIFFT<float>>(c, false, true);
            else transform<double, Fourier::OnTheFlyIFFT<double>>(c, false, true);
        } else require(false, "Adapter implementation missing");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
