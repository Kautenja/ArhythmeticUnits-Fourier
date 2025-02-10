// An integrator for slew limiting / lag processing.
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

#ifndef ENVELOPE_INTEGRATOR_HPP
#define ENVELOPE_INTEGRATOR_HPP

#include <algorithm>
#include <limits>
#include "../math.hpp"
#include "_common.hpp"

/// @brief Structures for generating functions.
namespace FunctionGenerator {

/// @brief An integrator for slew limiting / lag processing.
/// @tparam T the data type for processing and representing instance parameters
template<typename T>
struct Integrator {
    static_assert(std::is_floating_point<T>::value, "T must be floating point!");

 protected:
    /// the number of seconds in the rise stage
    T rise = 0.0001;
    /// the number of seconds in the fall stage
    T fall = 0.0001;

    /// the exponential shape of the rise curve
    /// >0: logarithmic
    /// =0: linear
    /// <0: exponential
    T riseShape = 0;
    /// the exponential shape of the fall curve
    /// >0: logarithmic
    /// =0: linear
    /// <0: exponential
    T fallShape = 0;

    /// the output value from the integrator
    T value = 0;

 public:
    /// Brief Initialize a new integrator with given parameters.
    ///
    /// @param rise the number of seconds in the rise stage
    /// @param fall the number of seconds in the fall stage
    /// @param riseShape the exponential shape of the rise curve
    /// @param fallShape the exponential shape of the fall curve
    ///
    explicit Integrator(
        T rise = 0.0001,
        T fall = 0.0001,
        T riseShape = 0,
        T fallShape = 0
    ) :
        rise(rise),
        fall(fall),
        riseShape(riseShape),
        fallShape(fallShape) { }

    /// @brief Set the rise length to a new value.
    ///
    /// @param time the number of seconds in the rise stage
    ///
    inline void setRise(const T& time) {
        // limit the rise time to prevent division by zero in processing loop
        rise = std::max(static_cast<T>(0.0001), time);
    }

    /// @brief Return the amount of time in the rise stage.
    inline T getRise() const { return rise; }

    /// @brief Set the fall length to a new value.
    ///
    /// @param time the number of seconds in the fall stage
    ///
    inline void setFall(const T& time) {
        // limit the fall time to prevent division by zero in processing loop
        fall = std::max(static_cast<T>(0.0001), time);
    }

    /// @brief Return the amount of time in the fall stage.
    inline T getFall() const { return fall; }

    /// @brief Set the rise shape to a new value.
    ///
    /// @param shape the new value for the exponential rise shape
    ///         >0: logarithmic
    ///         =0: linear
    ///         <0: exponential
    ///
    inline void setRiseShape(const T& shape) { riseShape = shape; }

    /// @brief Return the shape of the rise stage.
    inline T getRiseShape() const { return riseShape; }

    /// @brief Set the fall shape to a new value.
    ///
    /// @param shape the new value for the exponential fall shape
    ///         >0: logarithmic
    ///         =0: linear
    ///         <0: exponential
    ///
    inline void setFallShape(const T& shape) { fallShape = shape; }

    /// @brief Return the shape of the rise stage.
    inline T getFallShape() const { return fallShape; }

    /// @brief Return the current output value from the integrator.
    inline T getValue() const { return value; }

    /// @brief Hard reset the value of the integrator to 0.
    inline void reset() { value = 0; }

    /// @brief Process a sample through the integrator.
    ///
    /// @param signal the input signal to integrate
    /// @param deltaTime the amount of time between this sample and the last
    ///
    inline void process(const T& signal, const T& deltaTime) {
        // Calculate (where time is measured in seconds):
        //
        //     change in time per sample
        //    ---------------------------
        //     change in time per stage
        //
        const T dStage_dSample = deltaTime / (signal > value ? rise : -fall);
        const T shape = signal > value ? riseShape : fallShape;
        // Calculate the difference in magnitude between value and target.
        const T dV = abs(signal - value);
        // Ensure the change is significantly large w.r.t. number type system.
        // This guarantees that the edge case 0^0 = 1 does not occur in the
        // integration for cases where the voltage is 0. It also ensures that
        // the integrator stably converges to the target value according to the
        // constraints of the number system.
        if (dV <= std::numeric_limits<T>::epsilon())
            value = signal;
        else {  // integrate the change
            // Calculate the amount of signal to integrate.
            const T integration = pow(dV, shape) * dStage_dSample;
            // If the integration exceeds the total change, limit the change
            // to the target to prevent over-integration.
            value = abs(integration) >= dV ? signal : value + integration;
        }
    }
};

}  // namespace FunctionGenerator

#endif  // ENVELOPE_INTEGRATOR_HPP
