// Prepared native analysis kernels; no canonical-complex transfer buffers.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_KERNELS_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_KERNELS_HPP_

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <vector>
#include <rack.hpp>
#include "../../src/dsp/math.hpp"
#include "pffft.hpp"
#ifdef PAPER_HAVE_VDSP
#include "vdsp.hpp"
#endif
#ifdef PAPER_HAVE_FFTW
#include "fftw.hpp"
#endif

namespace Paper {
/// @brief Contiguous, compiler-vectorizable window/input stores.
template<typename T>
void native_window(const T* source, const T* window, T* destination, size_t count) {
    for (size_t i = 0; i < count; ++i) destination[i] = source[i]*window[i];
}

/// @brief Explicit unordered+reorder control; natural bin conversion is charged.
template<bool Unordered = false>
class PffftNative {
    struct Delete { void operator()(float* p) const { pffft_aligned_free(p); } };
    using Buffer = std::unique_ptr<float, Delete>;
    struct PlanDelete { void operator()(PFFFT_Setup* p) const { pffft_destroy_setup(p); } };
    size_t n;
    std::unique_ptr<PFFFT_Setup, PlanDelete> plan;
    Buffer input, packed, scratch, unordered;
    static Buffer allocate(size_t n) {
        Buffer result(static_cast<float*>(pffft_aligned_malloc(n*sizeof(float))));
        if (!result) throw std::bad_alloc();
        return result;
    }
 public:
    using Scalar = float;
    static constexpr size_t channels = 1;
    explicit PffftNative(size_t length) : n(length), plan(pffft_new_setup(int(n), PFFFT_REAL)),
        input(allocate(n)), packed(allocate(n)), scratch(allocate(n)), unordered(Unordered ? allocate(n) : Buffer()) {
        if (!plan) throw std::bad_alloc();
    }
    void prepare(size_t, size_t at, const float* source, const float* window, size_t count) {
        native_window(source, window, input.get()+at, count);
    }
    void transform() {
        if (Unordered) {
            pffft_transform(plan.get(), input.get(), unordered.get(), scratch.get(), PFFFT_FORWARD);
            pffft_zreorder(plan.get(), unordered.get(), packed.get(), PFFFT_FORWARD);
        } else pffft_transform_ordered(plan.get(), input.get(), packed.get(), scratch.get(), PFFFT_FORWARD);
    }
    void magnitudes(size_t, size_t at, size_t count, float* output) const {
        if (at == 0 && count) output[0] = std::abs(packed.get()[0]);
        if (at <= n/2 && at+count > n/2) output[n/2-at] = std::abs(packed.get()[1]);
        const size_t begin = std::max(size_t(1), at), end = std::min(n/2, at+count);
        for (size_t k = begin; k < end; ++k) {
            const float re = packed.get()[2*k], im = packed.get()[2*k+1];
            output[k-at] = Fourier::complex_magnitude(std::complex<float>(re, im));
        }
    }
    std::string info_json() const {
        std::ostringstream out;
        out << "{\"provider\":\"rack-pffft\",\"precision\":\"float\",\"plan_policy\":\"exact-size reusable real plan\","
            << "\"native_plan_bytes\":null,\"native_batch_channels\":1,\"simd_lanes\":" << pffft_simd_size()
            << ",\"layout_policy\":\"" << (Unordered ? "unordered then pffft_zreorder" : "ordered packed")
            << "\",\"natural_frequency_output\":true,\"aligned_payload_bytes\":" << (Unordered ? 4 : 3)*n*sizeof(float)
            << ",\"native_execution_allocations\":null,\"limitations\":\"Explicit aligned scratch; native allocator calls and opaque plan storage are outside C++ audit\"}";
        return out.str();
    }
};

#ifdef PAPER_HAVE_VDSP
namespace NativeVdsp {
template<typename T> struct Api;
#define PAPER_NATIVE_VDSP(TYPE, SUFFIX) \
template<> struct Api<TYPE> : VdspDetail::Api<TYPE> { \
    using Base = VdspDetail::Api<TYPE>; \
    static void multiply(const TYPE* x, const TYPE* w, TYPE* y, size_t n) { \
        vDSP_vmul##SUFFIX(x, 2, w, 2, y, 1, n); \
    } \
    static void many(typename Base::Setup setup, typename Base::Split* split, size_t exponent, size_t n, size_t channels) { \
        vDSP_fftm_zrip##SUFFIX(setup, split, 1, n/2, exponent, channels, FFT_FORWARD); \
    } \
    static void magnitude(typename Base::Split* split, TYPE* output, size_t n) { \
        for (size_t i = 0; i < n; ++i) \
            output[i] = Fourier::complex_magnitude(std::complex<TYPE>(split->realp[i], split->imagp[i])) * TYPE(.5); \
    } \
};
PAPER_NATIVE_VDSP(float, )
PAPER_NATIVE_VDSP(double, D)
#undef PAPER_NATIVE_VDSP
}  // namespace NativeVdsp

/// @brief Fuse two-span windowing directly into native even/odd split input.
template<typename T, size_t Channels = 1>
class VdspNative {
    using Api = NativeVdsp::Api<T>;
    size_t n, exponent = 0;
    std::vector<T> real, imaginary;
    typename Api::Setup setup;
 public:
    using Scalar = T;
    static constexpr size_t channels = Channels;
    explicit VdspNative(size_t length) : n(length), real(Channels*n/2), imaginary(Channels*n/2) {
        for (size_t size = n; size > 1; size /= 2) ++exponent;
        setup = Api::create(exponent);
        if (!setup) throw std::bad_alloc();
    }
    ~VdspNative() { Api::destroy(setup); }
    VdspNative(const VdspNative&) = delete;
    VdspNative& operator=(const VdspNative&) = delete;
    void prepare(size_t channel, size_t at, const T* source, const T* window, size_t count) {
        // Segments can start on an odd sample, including a retained-ring wrap.
        for (size_t parity = 0; parity < 2; ++parity) {
            const size_t first = (parity+2-at%2)%2;
            if (first >= count) continue;
            T* destination = (parity ? imaginary.data() : real.data())+channel*n/2+(at+first)/2;
            Api::multiply(source+first, window+first, destination, (count-first+1)/2);
        }
    }
    void transform() {
        typename Api::Split split{real.data(), imaginary.data()};
        if (Channels == 1) Api::real(setup, &split, exponent);
        else Api::many(setup, &split, exponent, n, Channels);
    }
    void magnitudes(size_t channel, size_t at, size_t count, T* output) {
        const size_t begin = std::max(size_t(1), at), end = std::min(n/2, at+count);
        const size_t base = channel*n/2;
        if (at == 0 && count) output[0] = std::abs(real[base]*T(.5));
        if (at <= n/2 && at+count > n/2) output[n/2-at] = std::abs(imaginary[base]*T(.5));
        if (begin < end) {
            typename Api::Split split{real.data()+base+begin, imaginary.data()+base+begin};
            Api::magnitude(&split, output+begin-at, end-begin);
        }
    }
    std::string info_json() const {
        Dl_info image = {}; dladdr(reinterpret_cast<const void*>(&vDSP_create_fftsetup), &image);
        std::ostringstream out;
        out << "{\"provider\":\"Apple Accelerate/vDSP\",\"precision\":\"" << (sizeof(T) == 4 ? "float" : "double")
            << "\",\"setup_count\":1,\"plan\":\"exact-size radix-2 reusable setup\",\"setup_bytes\":null"
            << ",\"framework_image\":" << VdspDetail::quoted(image.dli_fname ? image.dli_fname : "unknown")
            << ",\"os_build\":" << VdspDetail::quoted(VdspDetail::system_value("kern.osversion"))
            << ",\"sdk_version_max_allowed\":" << MAC_OS_X_VERSION_MAX_ALLOWED
            << ",\"native_batch_channels\":" << Channels << ",\"persistent_buffer_bytes\":" << Channels*n*sizeof(T)
            << ",\"layout_policy\":\"fused strided window/even-odd packing; split magnitudes; zrip factor 1/2\""
            << ",\"natural_frequency_output\":true,\"native_execution_allocations\":null"
            << ",\"limitations\":\"System framework/shared cache, setup storage and native worker/allocation/scratch behavior are opaque\"}";
        return out.str();
    }
};
#endif

#ifdef PAPER_HAVE_FFTW
/// @brief Aligned direct window stores and a real plan-many across independent rows.
template<typename T, size_t Channels = 1>
class FftwNative {
    using Api = FftwDetail::Api<T>;
    size_t n;
    T* input;
    typename Api::Complex* output;
    typename Api::Plan plan = nullptr;
 public:
    using Scalar = T;
    static constexpr size_t channels = Channels;
    explicit FftwNative(size_t length) : n(length), input(static_cast<T*>(Api::allocate(Channels*n*sizeof(T)))),
        output(static_cast<typename Api::Complex*>(Api::allocate(Channels*(n/2+1)*sizeof(typename Api::Complex)))) {
        if (!input || !output) { Api::release(input); Api::release(output); throw std::bad_alloc(); }
        Api::forget();
        plan = Api::many(int(n), int(Channels), input, output);
        if (!plan) { Api::release(input); Api::release(output); throw std::runtime_error("FFTW plan-many failed"); }
    }
    ~FftwNative() { Api::destroy(plan); Api::release(input); Api::release(output); }
    FftwNative(const FftwNative&) = delete;
    FftwNative& operator=(const FftwNative&) = delete;
    void prepare(size_t channel, size_t at, const T* source, const T* window, size_t count) {
        native_window(source, window, input+channel*n+at, count);
    }
    void transform() { Api::execute(plan); }
    void magnitudes(size_t channel, size_t at, size_t count, T* result) const {
        const auto* values = output+channel*(n/2+1)+at;
        for (size_t i = 0; i < count; ++i)
            result[i] = Fourier::complex_magnitude(std::complex<T>(values[i][0], values[i][1]));
    }
    std::string info_json() const {
        char* text = Api::plan_text(plan); char* wisdom = Api::wisdom();
        if (!text || !wisdom) { std::free(text); std::free(wisdom); throw std::bad_alloc(); }
        std::ostringstream out;
        out << "{\"provider\":\"fftw\",\"precision\":\"" << (sizeof(T) == 4 ? "float" : "double")
            << "\",\"version\":" << FftwDetail::quoted(Api::version())
            << ",\"compiler\":" << FftwDetail::quoted(Api::compiler())
            << ",\"codelet_optimization\":" << FftwDetail::quoted(Api::codelets())
            << ",\"threads\":1,\"plan_policy\":\"FFTW_MEASURE\",\"imported_wisdom\":false"
            << ",\"real_plan\":" << FftwDetail::quoted(text) << ",\"exported_wisdom\":" << FftwDetail::quoted(wisdom)
            << ",\"native_batch_channels\":" << Channels << ",\"native_buffer_requested_bytes\":" << Channels*(2*n+2)*sizeof(T)
            << ",\"layout_policy\":\"aligned plan-many r2c rows; direct window stores and magnitudes\""
            << ",\"natural_frequency_output\":true,\"plan_storage_bytes\":null,\"native_execution_allocations\":null"
            << ",\"limitations\":\"Serial plans; native execution scratch/allocation and planner global storage are opaque to C++ audit\"}";
        std::free(text); std::free(wisdom); return out.str();
    }
};
#endif
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_NATIVE_KERNELS_HPP_
