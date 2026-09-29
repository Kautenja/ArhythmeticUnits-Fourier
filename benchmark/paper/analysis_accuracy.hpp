// Per-spectrum numerical acceptance and retained pointwise diagnostics.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_ANALYSIS_ACCURACY_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_ANALYSIS_ACCURACY_HPP_
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "references.hpp"

namespace Paper {
/// @brief Untimed processed-spectrum checks; no per-bin relative-error guarantee.
/// @details Use the existing analysis tolerances for both relative
/// L2 and Linf errors, independently for each published channel. Never divide
/// by a different frame's scale or floor small signals at one. A zero reference
/// requires exact zero. Preserve the former pointwise test as a diagnostic.
struct AnalysisAccuracy {
    size_t vectors = 0, values = 0, zero_vectors = 0, pointwise_failures = 0;
    double tolerance = 0, maximum_l2 = 0, maximum_linf = 0;
    double maximum_pointwise = 0, maximum_error = 0, maximum_reference = 0;
    size_t worst_endpoint = 0, worst_bin = 0, worst_channel = 0;
    double worst_actual = 0, worst_reference = 0;

    template<typename T, typename Getter>
    void compare(const std::vector<T>& expected, Getter actual, size_t endpoint, size_t channel = 0) {
        const double limit = sizeof(T) == 4 ? 3e-4 : 1e-10;
        if (expected.empty() || (vectors && tolerance != limit))
            throw std::runtime_error("Invalid analysis accuracy vector/precision");
        tolerance = limit;
        double error_l2 = 0, reference_l2 = 0, error_linf = 0, reference_linf = 0;
        for (size_t k = 0; k < expected.size(); ++k) {
            const double a = actual(k), b = expected[k], error = std::abs(a-b), scale = std::abs(b);
            if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(error))
                throw std::runtime_error("Non-finite analysis output/reference");
            error_l2 = std::hypot(error_l2, error); reference_l2 = std::hypot(reference_l2, scale);
            error_linf = std::max(error_linf, error); reference_linf = std::max(reference_linf, scale);
            const double pointwise = error/std::max(1., scale);
            pointwise_failures += pointwise > (sizeof(T) == 4 ? 3e-4 : 1e-10);
            if (pointwise > maximum_pointwise) {
                maximum_pointwise = pointwise; worst_endpoint = endpoint; worst_bin = k;
                worst_channel = channel; worst_actual = a; worst_reference = b;
            }
        }
        if (reference_linf == 0 && error_linf != 0)
            throw std::runtime_error("Nonzero analysis output for zero reference");
        const double l2 = reference_l2 ? error_l2/reference_l2 : 0;
        const double linf = reference_linf ? error_linf/reference_linf : 0;
        if (!std::isfinite(l2) || !std::isfinite(linf) || l2 > limit || linf > limit) {
            std::ostringstream message; message.precision(17);
            message << "Independent analysis spectrum norms differ: L2=" << l2 << ", Linf=" << linf
                << ", endpoint=" << endpoint << ", channel=" << channel;
            throw std::runtime_error(message.str());
        }
        maximum_l2 = std::max(maximum_l2, l2); maximum_linf = std::max(maximum_linf, linf);
        maximum_error = std::max(maximum_error, error_linf);
        maximum_reference = std::max(maximum_reference, reference_linf);
        ++vectors; values += expected.size(); zero_vectors += reference_linf == 0;
    }

    std::string json() const {
        std::ostringstream out; out.precision(17);
        out << "{\"policy\":\"spectrum-norms-v1\",\"tolerance\":" << tolerance
            << ",\"vectors\":" << vectors << ",\"values\":" << values << ",\"zero_vectors\":" << zero_vectors
            << ",\"max_relative_l2\":" << maximum_l2 << ",\"max_relative_linf\":" << maximum_linf
            << ",\"legacy_pointwise_failures\":" << pointwise_failures
            << ",\"max_legacy_scaled_error\":" << maximum_pointwise
            << ",\"worst_pointwise\":{\"endpoint\":" << worst_endpoint << ",\"bin\":" << worst_bin
            << ",\"channel\":" << worst_channel << ",\"actual\":" << worst_actual
            << ",\"reference\":" << worst_reference << "}}";
        return out.str();
    }
};
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_ANALYSIS_ACCURACY_HPP_
