// An Attack-Decay-Hold-Release (ADHR) envelope generator.
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

#ifndef ENVELOPE_ADHR_HPP
#define ENVELOPE_ADHR_HPP

#include <cstdint>
#include <algorithm>
#include "../math.hpp"
#include "_common.hpp"

/// @brief Structures for generating functions.
namespace FunctionGenerator {

/// @brief An Attack-Decay-Hold-Release (ADHR) envelope generator.
struct ADHR {
 public:
    /// the different stages in the envelope.
    enum class Stage { Stopped = 0, Attack, Decay, Hold, Release, NumStages };

    /// the events that output from the envelope generator at the end of stages
    enum class Event { None = 0, EndOfAttack, EndOfDecay, EndOfHold, EndOfRelease };

 private:
    /// the amount of seconds in the attack stage
    float attack = MIN_STAGE_TIME;
    /// the amount of seconds in the decay stage
    float decay = MIN_STAGE_TIME;
    /// the level of the hold stage
    float holdLevel = 0.0f;
    /// the amount of seconds in the hold stage
    float hold = MIN_STAGE_TIME;
    /// the amount of seconds in the release stage
    float release = MIN_STAGE_TIME;

    /// the exponential shape of the attack curve
    float attackShape = 1.0f;
    /// the exponential shape of the decay curve
    float decayShape = 1.0f;
    /// the exponential shape of the release curve
    float releaseShape = 1.0f;

    /// the current stage that the envelope is in
    Stage stage = Stage::Stopped;
    /// the progress in the current stage of the envelope generator
    float stageProgress = 0.0f;
    /// the output value from the envelope generator
    float value = 0.0f;

 public:
    /// a flag controlling whether the envelope is looping
    bool isLooping = false;

    /// @brief Set the attack stage length to a new value.
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

    /// @brief Set the decay stage length to a new value.
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

    /// @brief Set the hold stage level to a new value.
    ///
    /// @param level the level of the hold in [0, 1]
    ///
    inline void setHoldLevel(float level) {
        holdLevel = Math::clip(level, 0.f, 1.f);
    }

    /// @brief Return the hold level in units.
    inline float getHoldLevel() const { return holdLevel; }

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

    /// @brief Set the release stage length to a new value.
    ///
    /// @param seconds the number of seconds in the release stage
    /// @details
    /// The minimal release is 1ms.
    ///
    inline void setRelease(float seconds) {
        release = std::max(seconds, MIN_STAGE_TIME);
    }

    /// @brief Return the release stage length in seconds.
    inline float getRelease() const { return release; }

    /// @brief Set the attack shape to a new value.
    ///
    /// @param shape the new value for the exponential attack shape
    /// @details
    /// Good values are in the range \f$\in [0.1, 10]\f$
    ///
    inline void setAttackShape(float shape) { attackShape = shape; }

    /// @brief Return the attack shape exponent.
    inline float getAttackShape() const { return attackShape; }

    /// @brief Set the decay shape to a new value.
    ///
    /// @param shape the new value for the exponential decay shape
    /// @details
    /// Good values are in the range \f$\in [0, 10]\f$
    ///
    inline void setDecayShape(float shape) { decayShape = shape; }

    /// @brief Return the decay shape exponent.
    inline float getDecayShape() const { return decayShape; }

    /// @brief Set the release shape to a new value.
    ///
    /// @param shape the new value for the exponential release shape
    /// @details
    /// Good values are in the range \f$\in [0, 10]\f$
    ///
    inline void setReleaseShape(float shape) { releaseShape = shape; }

    /// @brief Return the release shape exponent.
    inline float getReleaseShape() const { return releaseShape; }

    /// @brief Return the total duration of the envelope generator.
    inline float getDuration() const { return attack + hold + decay + release; }

    /// @brief Return the current stage of the ADHR.
    inline Stage getStage() const { return stage; }

    /// @brief Return true if the envelope generator has been triggered.
    inline bool getIsTriggered() const {
        return static_cast<int>(stage) > static_cast<int>(Stage::Stopped);
    }

    /// @brief Return a boolean gate determining the end of curve (EOC).
    /// @details
    /// EOC gate goes high at the end of curve, i.e., when the stage is
    /// stopped. the gate extends through the attack phase allowing it to act
    /// as a gate signal for an AD LFO.
    inline bool isEOC() const {
        return static_cast<int>(stage) <= static_cast<int>(Stage::Attack);
    }

    /// @brief Return a boolean gate determining the end of onset (EON).
    /// @details
    /// EON goes high everywhere that EOC is low, i.e., it is low during the
    /// stopped / attack stage, and high in all other stages.
    inline bool isEON() const { return !isEOC(); }

    /// @brief Return the current output value from the envelope.
    inline float getValue() const { return value; }

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

    /// @brief Process a sample through the envelope generator.
    ///
    /// @param deltaTime the amount of time between this sample and the last
    /// @returns the event that occurred during processing, if any
    ///
    Event process(float deltaTime) {
        if (stage == Stage::Stopped) return Event::None;
        // increment the stage progress
        stageProgress += deltaTime;
        // calculate the value of the attack function (exponential)
        const auto attackValue = pow(std::min(1.0f, stageProgress / attack), attackShape);
        // calculate the value of the decay function (inverted exponential)
        const auto decayValue = 1.f - (1.f - holdLevel) * pow(std::min(1.0f, stageProgress / decay), decayShape);
        // calculate the value of the release function (inverted exponential)
        const auto releaseValue = holdLevel - holdLevel * pow(std::min(1.0f, stageProgress / release), releaseShape);

        // NOTE: this commented out implementation is slower, but easier to
        //       work into a linear algebra framework like eigen.hpp
        // // get the value based on current stage
        // value =
        //     (stage == Stage::Attack) * attackValue +
        //     (stage == Stage::Decay) * decayValue +
        //     (stage == Stage::Hold) * holdLevel +
        //     (stage == Stage::Release) * releaseValue;
        // // get the stage time based on current stage and extra conditions:
        // // - attack time is ignored if value is equal to or above 1
        // // - decay time is ignored if value is equal to or below 0
        // // - decay time is ignored if value is equal to or below hold level
        // // - sustain time is arbitrarily set to stageProgress + 1s
        // // - sustain time is ignored when the gate is low (key released)
        // // - release time is ignored if the value is equal to or below 0
        // const auto time =
        //     (stage == Stage::Attack) * attack * (value < 1.f) +
        //     (stage == Stage::Decay) * decay * (value > 0.f) * (value > sustain) +
        //     (stage == Stage::Sustain) * (stageProgress + 1.f) * isGateHigh +
        //     (stage == Stage::Release) * release * (value > 0.f);

        // get the value and time based on the current stage
        float time = 0.f;
        switch (stage) {
        case Stage::Attack:
            value = attackValue;
            // attack time is ignored if value is equal to or above 1
            if (value < 1.f) time = attack;
            break;
        case Stage::Decay:
            value = decayValue;
            // decay time is ignored if value is equal to or below 0
            // decay time is ignored if value is equal to or below sustain level
            if (value > holdLevel && value > 0.f) time = decay;
            break;
        case Stage::Hold:
            value = holdLevel;
            if (value > 0.f) time = hold;
            break;
        case Stage::Release:
            value = releaseValue;
            // release time is ignored if the value is equal to or below 0
            if (value > 0.f) time = release;
            break;
        default: break;
        }

        // if the stage progress exceeds the time, jump to the next stage
        auto event = Event::None;
        if (stageProgress >= time) {
            // get the end of stage event from the current stage
            event = static_cast<Event>(stage);
            // increment to the next stage; circle back around to stopped
            stage = static_cast<Stage>((static_cast<int>(stage) + 1) % static_cast<int>(Stage::NumStages));
            if (isLooping && stage == Stage::Stopped) stage = Stage::Attack;
            // reset the stage progress back to 0 for the next stage
            stageProgress = 0.f;
        }
        // clamp the amplitude within legal values
        value = Math::clip(value, 0.f, 1.f);
        // return the event that may have occurred from this call to process
        return event;
    }
};

}  // namespace FunctionGenerator

#endif  // ENVELOPE_ADHR_HPP
