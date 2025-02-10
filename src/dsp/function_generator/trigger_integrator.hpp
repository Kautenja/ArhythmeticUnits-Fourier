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

#ifndef ENVELOPE_TRIGGER_INTEGRATOR_HPP
#define ENVELOPE_TRIGGER_INTEGRATOR_HPP

#include <iostream>
#include <algorithm>
#include "../math.hpp"
#include "_common.hpp"

/// @brief Structures for generating functions.
namespace FunctionGenerator {

/// @brief An integrator for slew limiting / lag processing.
template<typename T>
struct TriggerIntegrator {
 protected:
    /// the number of seconds in the rise stage (1ms default)
    T rise = MIN_STAGE_TIME;
    /// the number of seconds in the fall stage (1m default)
    T fall = MIN_STAGE_TIME;

    /// the exponential shape of the rise curve (linear default)
    /// >0: logarithmic
    /// =0: linear
    /// <0: exponential
    T riseShape = 0.f;
    /// the exponential shape of the fall curve (linear default)
    /// >0: logarithmic
    /// =0: linear
    /// <0: exponential
    T fallShape = 0.f;

    /// whether the limiter has reached the end of the rise phase
    bool isEndOfRise = false;

    /// the output value from the slew limiter
    T value = 0.0f;

    /// whether the slew limiter is being triggered as an A-D envelope
    bool isTriggered = false;

 public:
    /// Whether the slew limiter is cycling as an Attack-Decay LFO
    bool isCycling = false;

    /// Brief Initialize a new integrator with given parameters.
    ///
    /// @param rise the number of seconds in the rise stage
    /// @param fall the number of seconds in the fall stage
    /// @param riseShape the exponential shape of the rise curve
    /// @param fallShape the exponential shape of the fall curve
    /// @param isCycling whether the integrator is cycling as an AD LFO
    ///
    explicit TriggerIntegrator(
        T rise = MIN_STAGE_TIME,
        T fall = MIN_STAGE_TIME,
        T riseShape = 0,
        T fallShape = 0,
        bool isCycling = false
    ) :
        rise(rise),
        fall(fall),
        riseShape(riseShape),
        fallShape(fallShape),
        isCycling(isCycling) { }

    /// @brief Return true if the slew limiter has been triggered.
    inline bool getIsTriggered() const { return isTriggered; }

    /// @brief Set the rise length to a new value.
    ///
    /// @param time the number of seconds in the rise stage
    ///
    inline void setRise(const T& time) {
        rise = std::max(time, MIN_STAGE_TIME);
    }

    /// @brief Return the amount of time in the rise stage.
    inline T getRise() const { return rise; }

    /// @brief Set the fall length to a new value.
    ///
    /// @param time the number of seconds in the fall stage
    ///
    inline void setFall(const T& time) {
        fall = std::max(time, MIN_STAGE_TIME);
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

    /// @brief Return a boolean gate determining the end of rise (EOR).
    inline bool isEOR() const { return isEndOfRise; }

    /// @brief Return a boolean gate determining the end of fall (EOF).
    inline bool isEOF() const { return !isEndOfRise; }

    /// @brief Return the current output value from the slew limiter.
    inline T getValue() const { return value; }

    /// @brief Hard reset the value of the slew limiter to 0.
    /// @details
    /// This will only reset the instance data of the slew limiter; it will not
    /// affect parameters, i.e., those assigned through `set...`.
    ///
    inline void reset() {
        value = 0;
        isEndOfRise = false;
        isTriggered = false;
    }

    /// @brief Trigger the slew limiter.
    /// TODO: hard reset introduces pops, introduce a trigger stage?
    inline void trigger() {
        reset();
        isTriggered = true;
    }

    /// @brief Process a sample through the slew limiter.
    ///
    /// @param signal the input signal to slew
    /// @param deltaTime the amount of time between this sample and the last
    ///
    void process(T signal, const T& deltaTime) {
        // If cycling or trigger, reset signal to a gate based on stage.
        if (isCycling || isTriggered) signal = static_cast<T>(isEOF());
        // Calculate (where time is measured in seconds):
        //
        //     change in time per sample
        //    ---------------------------
        //     change in time per stage
        //
        const bool is_rise = signal > value;
        const T dStage_dSample = deltaTime / (is_rise ? rise : -fall);
        const T shape = signal > value ? riseShape : fallShape;
        // Calculate the difference in magnitude between value and target.
        T dV = abs(signal - value);
        // Ensure the change is significantly large w.r.t. number type system.
        // This guarantees that the edge case 0^0 = 1 does not occur in the
        // integration for cases where the voltage is 0. It also ensures that
        // the integrator stably converges to the target value according to the
        // constraints of the number system.
        if (dV <= std::numeric_limits<T>::epsilon()) {
            value = signal;
            return;
        } else {  // integrate the change
            // Calculate the amount of signal to integrate.
            const T integration = pow(dV, shape) * dStage_dSample;
            // If the integration exceeds the total change, limit the change
            // to the target to prevent over-integration.
            value = abs(integration) >= dV ? signal : value + integration;
        }
        // Calculate the end of rise signal.
        //
        // | is rise | is converged | is end of rise |
        // |:--------|:-------------|:---------------|
        // | true    | true         | true           |
        // | false   | true         | false          |
        // | true    | false        | false          |
        // | false   | false        | true           |
        //
        // Calculate the remaining change in value to the target value.
        dV = abs(signal - value);
        const bool is_converged = dV <= std::numeric_limits<T>::epsilon();
        isEndOfRise = !(is_rise ^ is_converged);
        // Set the trigger gate based on its state and the current stage.
        isTriggered = isTriggered && isEOF();
    }
};

}  // namespace FunctionGenerator

#endif  // ENVELOPE_TRIGGER_INTEGRATOR_HPP
