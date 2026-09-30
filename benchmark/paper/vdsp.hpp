// Benchmark-only canonical adapter for the macOS Accelerate FFT APIs.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_PROVIDERS_VDSP_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_PROVIDERS_VDSP_HPP_

#if defined(__APPLE__) && defined(PAPER_HAVE_VDSP)
// Avoid the umbrella's unrelated graphics types, which collide with Rack names.
#include <vecLib/vDSP.h>
#include <AvailabilityMacros.h>
#include <dlfcn.h>
#include <sys/sysctl.h>
#include <complex>
#include <cstddef>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Paper {
namespace VdspDetail {
template<typename T> struct Api;
template<> struct Api<float> {
    using Setup = FFTSetup;
    using Split = DSPSplitComplex;
    static Setup create(size_t exponent) { return vDSP_create_fftsetup(exponent, kFFTRadix2); }
    static void destroy(Setup setup) { vDSP_destroy_fftsetup(setup); }
    static void real(Setup setup, Split* split, size_t exponent) {
        vDSP_fft_zrip(setup, split, 1, exponent, FFT_FORWARD);
    }
    static void complex(Setup setup, Split* split, size_t exponent, bool inverse) {
        vDSP_fft_zip(setup, split, 1, exponent, inverse ? FFT_INVERSE : FFT_FORWARD);
    }
};
template<> struct Api<double> {
    using Setup = FFTSetupD;
    using Split = DSPDoubleSplitComplex;
    static Setup create(size_t exponent) { return vDSP_create_fftsetupD(exponent, kFFTRadix2); }
    static void destroy(Setup setup) { vDSP_destroy_fftsetupD(setup); }
    static void real(Setup setup, Split* split, size_t exponent) {
        vDSP_fft_zripD(setup, split, 1, exponent, FFT_FORWARD);
    }
    static void complex(Setup setup, Split* split, size_t exponent, bool inverse) {
        vDSP_fft_zipD(setup, split, 1, exponent, inverse ? FFT_INVERSE : FFT_FORWARD);
    }
};

/// @brief Escape identity strings supplied by macOS before embedding in JSON.
inline std::string quoted(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32) out << "\\u00" << "0123456789abcdef"[c >> 4]
                            << "0123456789abcdef"[c & 15];
        else out << c;
    }
    out << '"';
    return out.str();
}
inline std::string system_value(const char* key) {
    char value[256] = {};
    size_t length = sizeof(value)-1;
    return sysctlbyname(key, value, &length, nullptr, 0) == 0 ? value : "unknown";
}
}  // namespace VdspDetail

/// @brief Canonical real/complex FFTs with explicitly timed packing and scaling.
/// @details All N bins are written in natural order. Forward sign is negative;
/// inverse sign is positive with 1/N normalization. Instances are noncopyable,
/// single-threaded, and own one setup reused for every call. The real-only
/// operations analysis/rfft allocate N scalars; complex operations allocate 2N.
/// Apple's opaque setup and internal scratch are additional, unknown storage.
template<typename T>
class VdspBackend {
    using Api = VdspDetail::Api<T>;
    size_t n, exponent = 0;
    bool real_only;
    std::vector<T> real_part, imaginary_part;
    typename Api::Setup setup = nullptr;

    static size_t checked_size(size_t length, const std::string& operation) {
        if (length < 128 || length > 16384 || (length & (length-1)))
            throw std::invalid_argument("vDSP benchmark length must be a power of two in [128, 16384]");
        if (operation != "analysis" && operation != "rfft" && operation != "fft"
                && operation != "ifft" && operation != "inverse"
                && operation != "identity" && operation != "fir")
            throw std::invalid_argument("Unsupported vDSP benchmark operation");
        return length;
    }

    void complex(const std::complex<T>* input, std::complex<T>* output, bool inverse) {
        if (real_only) throw std::logic_error("Complex transform requested from real-only vDSP setup");
        for (size_t i = 0; i < n; ++i) {
            real_part[i] = input[i].real();
            imaginary_part[i] = input[i].imag();
        }
        typename Api::Split split{real_part.data(), imaginary_part.data()};
        Api::complex(setup, &split, exponent, inverse);
        const T scale = inverse ? T(1)/T(n) : T(1);
        for (size_t i = 0; i < n; ++i)
            output[i] = {real_part[i]*scale, imaginary_part[i]*scale};
    }

 public:
    VdspBackend(size_t length, const std::string& operation)
        : n(checked_size(length, operation)),
          real_only(operation == "analysis" || operation == "rfft"),
          real_part(real_only ? n/2 : n), imaginary_part(real_only ? n/2 : n) {
        for (size_t size = n; size > 1; size >>= 1) ++exponent;
        setup = Api::create(exponent);
        if (!setup) throw std::bad_alloc();
    }
    ~VdspBackend() { Api::destroy(setup); }
    VdspBackend(const VdspBackend&) = delete;
    VdspBackend& operator=(const VdspBackend&) = delete;

    /// @brief Write only N/2+1 bins for the analyzer's positive-spectrum boundary.
    void forward_real_positive(const T* input, std::complex<T>* output) {
        // Native real input is split into even and odd samples. No aliasing
        // assumptions about std::complex or Apple interleaved structs are used.
        for (size_t i = 0; i < n/2; ++i) {
            real_part[i] = input[2*i];
            imaginary_part[i] = input[2*i+1];
        }
        typename Api::Split split{real_part.data(), imaginary_part.data()};
        Api::real(setup, &split, exponent);
        // zrip returns twice the mathematical DFT; Nyquist occupies imag[0].
        output[0] = {real_part[0]*T(0.5), T(0)};
        output[n/2] = {imaginary_part[0]*T(0.5), T(0)};
        for (size_t k = 1; k < n/2; ++k)
            output[k] = {real_part[k]*T(0.5), imaginary_part[k]*T(0.5)};
    }
    void forward_real(const T* input, std::complex<T>* output) {
        forward_real_positive(input, output);
        for (size_t k = 1; k < n/2; ++k)
            output[n-k] = std::conj(output[k]);
    }
    void forward_complex(const std::complex<T>* input, std::complex<T>* output) {
        complex(input, output, false);
    }
    void inverse_complex(const std::complex<T>* input, std::complex<T>* output) {
        complex(input, output, true);
    }

    /// @brief Identity/storage inspection runs outside the timed execution path.
    std::string info_json() const {
        Dl_info image = {};
        dladdr(reinterpret_cast<const void*>(&vDSP_create_fftsetup), &image);
        std::ostringstream out;
        out << "{\"provider\":\"Apple Accelerate/vDSP\",\"precision\":"
            << VdspDetail::quoted(sizeof(T) == 4 ? "float" : "double")
            << ",\"framework_image\":" << VdspDetail::quoted(image.dli_fname ? image.dli_fname : "unknown")
            << ",\"os_product_version\":" << VdspDetail::quoted(VdspDetail::system_value("kern.osproductversion"))
            << ",\"os_build\":" << VdspDetail::quoted(VdspDetail::system_value("kern.osversion"))
            << ",\"sdk_version_max_allowed\":" << MAC_OS_X_VERSION_MAX_ALLOWED
            << ",\"deployment_target\":" << MAC_OS_X_VERSION_MIN_REQUIRED
            << ",\"plan\":\"exact-size radix-2 reusable FFT setup\",\"setup_count\":1"
            << ",\"setup_log2_n\":" << exponent << ",\"setup_bytes\":null"
            << ",\"native_layout\":" << VdspDetail::quoted(real_only ? "even-odd split; DC/Nyquist packed" : "split complex")
            << ",\"persistent_buffer_bytes\":" << (real_part.capacity()+imaginary_part.capacity())*sizeof(T)
            << ",\"execution_scratch_buffer_bytes\":0,\"native_internal_scratch_bytes\":null"
            << ",\"native_execution_allocations\":null,\"framework_binary_hash\":null"
            << ",\"limitations\":\"System framework may reside in dyld shared cache; no independently rebuildable source or provider version API. Setup storage, native allocator calls and internal stack/scratch are opaque. C++ allocation audit does not intercept native allocations.\"}";
        return out.str();
    }
};
}  // namespace Paper
#endif  // defined(__APPLE__) && defined(PAPER_HAVE_VDSP)
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_PROVIDERS_VDSP_HPP_
