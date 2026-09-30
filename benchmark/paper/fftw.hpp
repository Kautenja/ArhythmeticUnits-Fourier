// Optional serial FFTW adapter for research benchmarks, never the Rack plugin.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_FFTW_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_FFTW_HPP_

#include <fftw3.h>

#include <climits>
#include <complex>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

namespace Paper {
namespace FftwDetail {

/// @brief Match precision to the independently linked serial FFTW API.
template<typename T> struct Api;
#define PAPER_FFTW_API(TYPE, PREFIX) \
template<> struct Api<TYPE> { \
    using Complex = PREFIX##_complex; \
    using Plan = PREFIX##_plan; \
    static void* allocate(size_t bytes) { return PREFIX##_malloc(bytes); } \
    static void release(void* pointer) { PREFIX##_free(pointer); } \
    static void forget() { PREFIX##_forget_wisdom(); } \
    static Plan real(int n, TYPE* in, Complex* out, unsigned flags) { \
        return PREFIX##_plan_dft_r2c_1d(n, in, out, flags); \
    } \
    static Plan complex(int n, Complex* in, Complex* out, int sign, unsigned flags) { \
        return PREFIX##_plan_dft_1d(n, in, out, sign, flags); \
    } \
    static void execute(Plan plan) { PREFIX##_execute(plan); } \
    static void destroy(Plan plan) { PREFIX##_destroy_plan(plan); } \
    static char* plan_text(Plan plan) { return PREFIX##_sprint_plan(plan); } \
    static char* wisdom() { return PREFIX##_export_wisdom_to_string(); } \
    static const char* version() { return PREFIX##_version; } \
    static const char* compiler() { return PREFIX##_cc; } \
    static const char* codelets() { return PREFIX##_codelet_optim; } \
};
PAPER_FFTW_API(float, fftwf)
PAPER_FFTW_API(double, fftw)
#undef PAPER_FFTW_API

inline std::string quoted(const char* value) {
    std::ostringstream out;
    out << '"';
    if (value) for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value); *p; ++p) {
        if (*p == '"' || *p == '\\') out << '\\' << char(*p);
        else if (*p == '\n') out << "\\n";
        else if (*p == '\r') out << "\\r";
        else if (*p == '\t') out << "\\t";
        else if (*p < 32) {
            const char* digits = "0123456789abcdef";
            out << "\\u00" << digits[*p >> 4] << digits[*p & 15];
        } else out << char(*p);
    }
    out << '"';
    return out.str();
}
}  // namespace FftwDetail

/// @brief Natural-order FFTW transforms using fixed, aligned native arrays.
/// @details Forward transforms are unnormalized; complex inverse stores all N
/// complex samples with 1/N scaling. Input copies, layout conversion, conjugate
/// reconstruction and inverse scaling execute inside the measured operation.
/// Planning is single-threaded and forgets prior wisdom before creating the
/// requested plans. Callers overwrite native inputs after MEASURE planning.
template<typename T>
class FftwBackend {
    using Api = FftwDetail::Api<T>;
    using NativeComplex = typename Api::Complex;
    using Plan = typename Api::Plan;
    size_t n;
    unsigned policy;
    T* real_input = nullptr;
    NativeComplex* complex_input = nullptr;
    NativeComplex* output = nullptr;
    size_t output_count = 0;
    Plan real_plan = nullptr;
    Plan forward_plan = nullptr;
    Plan inverse_plan = nullptr;

    void release() {
        if (real_plan) Api::destroy(real_plan);
        if (forward_plan) Api::destroy(forward_plan);
        if (inverse_plan) Api::destroy(inverse_plan);
        Api::release(real_input);
        Api::release(complex_input);
        Api::release(output);
    }

    void complex_transform(const std::complex<T>* input, std::complex<T>* result,
            Plan plan, T scale) {
        if (!plan) throw std::logic_error("FFTW operation was not prepared");
        for (size_t i = 0; i < n; ++i) {
            complex_input[i][0] = input[i].real();
            complex_input[i][1] = input[i].imag();
        }
        Api::execute(plan);
        for (size_t i = 0; i < n; ++i)
            result[i] = {output[i][0]*scale, output[i][1]*scale};
    }

    static std::string plan_json(Plan plan) {
        if (!plan) return "null";
        char* text = Api::plan_text(plan);
        if (!text) throw std::bad_alloc();
        const auto result = FftwDetail::quoted(text);
        std::free(text);
        return result;
    }

 public:
    /// @param operation analysis/rfft, fft, ifft/inverse, or identity/fir.
    /// @param flags MEASURE by default; ESTIMATE must be a separate workload.
    explicit FftwBackend(size_t size, const std::string& operation,
            unsigned flags = FFTW_MEASURE) : n(size), policy(flags) {
        if (!n || n > INT_MAX || (flags != FFTW_MEASURE && flags != FFTW_ESTIMATE))
            throw std::invalid_argument("Unsupported FFTW length or plan policy");
        const bool real = operation == "analysis" || operation == "rfft";
        const bool forward = operation == "fft" || operation == "identity" || operation == "fir";
        const bool inverse = operation == "ifft" || operation == "inverse"
            || operation == "identity" || operation == "fir";
        if (!real && !forward && !inverse)
            throw std::invalid_argument("Unsupported FFTW operation");
        output_count = real ? n/2+1 : n;
        try {
            output = static_cast<NativeComplex*>(Api::allocate(output_count*sizeof(NativeComplex)));
            if (!output) throw std::bad_alloc();
            if (real) {
                real_input = static_cast<T*>(Api::allocate(n*sizeof(T)));
                if (!real_input) throw std::bad_alloc();
            } else {
                complex_input = static_cast<NativeComplex*>(Api::allocate(n*sizeof(NativeComplex)));
                if (!complex_input) throw std::bad_alloc();
            }
            Api::forget();
            if (real) real_plan = Api::real(int(n), real_input, output, flags);
            if (forward) forward_plan = Api::complex(int(n), complex_input, output, FFTW_FORWARD, flags);
            if (inverse) inverse_plan = Api::complex(int(n), complex_input, output, FFTW_BACKWARD, flags);
            if ((real && !real_plan) || (forward && !forward_plan) || (inverse && !inverse_plan))
                throw std::runtime_error("FFTW could not create the requested plan");
        } catch (...) {
            release();
            throw;
        }
    }

    ~FftwBackend() { release(); }
    FftwBackend(const FftwBackend&) = delete;
    FftwBackend& operator=(const FftwBackend&) = delete;

    /// @brief Store exactly N/2+1 bins for an analyzer consuming positive frequencies.
    void forward_real_positive(const T* input, std::complex<T>* result) {
        if (!real_plan) throw std::logic_error("FFTW real operation was not prepared");
        for (size_t i = 0; i < n; ++i) real_input[i] = input[i];
        Api::execute(real_plan);
        for (size_t k = 0; k <= n/2; ++k) result[k] = {output[k][0], output[k][1]};
    }

    /// @brief Store all N bins, reconstructing the conjugate negative-frequency half.
    void forward_real(const T* input, std::complex<T>* result) {
        forward_real_positive(input, result);
        for (size_t k = n/2+1; k < n; ++k) result[k] = std::conj(result[n-k]);
    }

    void forward_complex(const std::complex<T>* input, std::complex<T>* result) {
        complex_transform(input, result, forward_plan, T(1));
    }

    void inverse_complex(const std::complex<T>* input, std::complex<T>* result) {
        complex_transform(input, result, inverse_plan, T(1)/T(n));
    }

    /// @brief Export provider evidence outside timing/allocation measurement.
    std::string info_json() const {
        char* wisdom = Api::wisdom();
        if (!wisdom) throw std::bad_alloc();
        const auto wisdom_json = FftwDetail::quoted(wisdom);
        std::free(wisdom);
        const size_t bytes = output_count*sizeof(NativeComplex)
            + (real_input ? n*sizeof(T) : n*sizeof(NativeComplex));
        std::ostringstream out;
        out << "{\"provider\":\"fftw\",\"version\":" << FftwDetail::quoted(Api::version())
            << ",\"precision\":" << FftwDetail::quoted(sizeof(T) == 4 ? "float" : "double")
            << ",\"length\":" << n
            << ",\"compiler\":" << FftwDetail::quoted(Api::compiler())
            << ",\"codelet_optimization\":" << FftwDetail::quoted(Api::codelets())
            << ",\"threads\":1,\"plan_policy\":\""
            << (policy == FFTW_MEASURE ? "FFTW_MEASURE" : "FFTW_ESTIMATE")
            << "\",\"imported_wisdom\":false,\"forget_wisdom_before_planning\":true"
            << ",\"native_buffer_requested_bytes\":" << bytes
            << ",\"alignment\":\"fftw_malloc\",\"plan_storage_bytes\":null"
            << ",\"native_execution_scratch_bytes\":null,\"native_execution_allocations\":null"
            << ",\"allocation_observation\":\"Wrapper allocates aligned arrays and plans during setup;"
            << " wrapper execution uses fixed arrays. FFTW buffered plans can allocate native scratch"
            << " during execute (3.3.10 dft/buffered.c and kernel/ifftw.h)."
            << " Native C allocation is not intercepted by the C++ audit."
            << " Planner global state may survive plan destruction until process exit.\""
            << ",\"real_plan\":" << plan_json(real_plan)
            << ",\"forward_plan\":" << plan_json(forward_plan)
            << ",\"inverse_plan\":" << plan_json(inverse_plan)
            << ",\"exported_wisdom\":" << wisdom_json
            << ",\"wisdom_scope\":\"Current process-global precision cache at metadata capture;"
            << " plan text is per instance\"}";
        return out.str();
    }
};
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_FFTW_HPP_
