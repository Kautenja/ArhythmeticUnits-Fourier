// An Attack-Decay (AD) envelope generator.
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

#ifndef ENVELOPE_AD_HPP
#define ENVELOPE_AD_HPP

#include <cstdint>
#include <algorithm>
#include "../math.hpp"
#include "_common.hpp"

/// @brief Structures for generating functions.
namespace FunctionGenerator {

/// @brief An Attack-Decay (AD) envelope generator.
struct AD {
 public:
    /// the different stages in the envelope.
    enum class Stage { Stopped = 0, Attack, Hold, Decay };

    /// the events that output from the envelope generator at the end of stages
    enum class Event { None = 0, EndOfAttack, EndOfHold, EndOfDecay };

 private:
    /// the amount of seconds in the attack stage
    float attack = MIN_STAGE_TIME;
    /// the amount of seconds in the hold stage
    float hold = MIN_STAGE_TIME;
    /// the amount of seconds in the decay stage
    float decay = MIN_STAGE_TIME;

    /// the exponential shape of the attack curve
    float attackShape = 1.0f;
    /// the exponential shape of the decay curve
    float decayShape = 1.0f;

    /// the current stage that the envelope is in
    Stage stage = Stage::Stopped;
    /// the progress in the current stage of the envelope generator
    float stageProgress = 0.0f;
    /// the output value from the envelope generator
    float value = 0.0f;

 public:
    /// a flag controlling whether the envelope is looping
    bool isLooping = false;

    /// @brief Reset the envelope generator to the first stage.
    ///
    /// @param trigger whether to trigger the generator after resetting
    /// @param hard whether to reset the envelope hard (true) or soft (false)
    ///
    void reset(bool trigger = false, bool hard = true) {
        // map the trigger to a stage. false=Stopped, true=Attack
        stage = static_cast<Stage>(trigger);
        // reset the stage's progress to 0
        stageProgress = 0.0f;
        // reset the output value to 0
        if (hard) value = 0.0f;
    }

    /// @brief Set the attack length to a new value.
    ///
    /// @param seconds the number of seconds in the attack stage
    /// @details
    /// The minimal attack is 1ms.
    ///
    inline void setAttack(float seconds) {
        attack = std::max(seconds, MIN_STAGE_TIME);
    }

    /// @brief Return the attack stage length in seconds.
    inline float getAttack() const { return attack; }

    /// @brief Set the hold length to a new value.
    ///
    /// @param seconds the number of seconds in the hold stage
    /// @details
    /// The minimal hold is 1ms.
    ///
    inline void setHold(float seconds) {
        hold = std::max(seconds, MIN_STAGE_TIME);
    }

    /// @brief Return the hold stage length in seconds.
    inline float getHold() const { return hold; }

    /// @brief Set the decay length to a new value.
    ///
    /// @param seconds the number of seconds in the decay stage
    /// @details
    /// The minimal decay is 1ms.
    ///
    inline void setDecay(float seconds) {
        decay = std::max(seconds, MIN_STAGE_TIME);
    }

    /// @brief Return the decay stage length in seconds.
    inline float getDecay() const { return decay; }

    /// @brief Set the attack shape to a new value.
    ///
    /// @param shape the new value for the exponential attack shape
    ///
    inline void setAttackShape(float shape) { attackShape = shape; }

    /// @brief Return the attack shape exponent.
    inline float getAttackShape() const { return attackShape; }

    /// @brief Set the decay shape to a new value.
    ///
    /// @param shape the new value for the exponential decay shape
    ///
    inline void setDecayShape(float shape) { decayShape = shape; }

    /// @brief Return the decay shape exponent.
    inline float getDecayShape() const { return decayShape; }

    /// @brief Return the total duration of the envelope generator.
    inline float getDuration() const { return attack + hold + decay; }

    /// @brief Return the current stage of the ADSR.
    inline Stage getStage() const { return stage; }

    /// @brief Return the current output value from the envelope.
    inline float getValue() const { return value; }

    /// @brief Process a sample through the envelope generator.
    ///
    /// @param deltaTime the amount of time between this sample and the last
    /// @returns the event that occurred during processing, if any
    ///
    Event process(float deltaTime) {
        auto event = Event::None;
        switch (stage) {
        case Stage::Stopped: break;
        case Stage::Attack:
            stageProgress += deltaTime;
            value = std::min(1.0f, stageProgress / attack);
            value = pow(value, attackShape);
            // update stage and stage progress
            if (value >= 1.f || stageProgress >= attack) {
                stage = Stage::Hold;
                stageProgress = 0.0f;
                event = Event::EndOfAttack;
            }
            break;
        case Stage::Hold:
            stageProgress += deltaTime;
            // update stage and stage progress
            if (stageProgress >= hold) {
                stage = Stage::Decay;
                stageProgress = 0.0f;
                event = Event::EndOfHold;
            }
            break;
        case Stage::Decay:
            stageProgress += deltaTime;
            value = std::min(1.0f, stageProgress / decay);
            value = 1.f - pow(value, decayShape);
            // update stage and stage progress
            if (stageProgress >= decay) {
                // if looping, jump around to attack, else go to stop
                stage = isLooping ? Stage::Attack : Stage::Stopped;
                stageProgress = 0.0f;
                event = Event::EndOfDecay;
            }
            break;
        }
        // clip the value within the valid range
        value = Math::clip(value, 0.f, 1.f);
        return event;
    }
};

}  // namespace FunctionGenerator

#endif  // ENVELOPE_AD_HPP
