// Regression tests for frequency-to-note conversion.
//
// Copyright 2026 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include "dsp/western_scale.hpp"
#include <cmath>
#include <limits>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("Reported low C frequencies have the correct octave") {
    const float frequency = GENERATE(130.813f, 65.4064f);
    CAPTURE(frequency);
    const Fourier::TunedNote note(frequency);
    REQUIRE(note.note == Fourier::Note::C);
    REQUIRE(note.octave == (frequency > 100.f ? 3 : 2));
    REQUIRE(note.note_string() == (frequency > 100.f ? "C3" : "C2"));
    // The supplied frequencies are rounded to at most six significant digits.
    REQUIRE(note.cents == Approx(0.f).margin(0.01f));
}

TEST_CASE("Every chromatic note retains its pitch class and octave") {
    const char* names[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    // Include negative octaves and every note below the A4 reference.
    for (int octave = -2; octave <= 9; ++octave) {
        for (int pitch = 0; pitch < 12; ++pitch) {
            const int midi_note = 12 * (octave + 1) + pitch;
            const float frequency = 440.0 * std::exp2((midi_note - 69) / 12.0);
            CAPTURE(octave, pitch, frequency);
            Fourier::TunedNote note;
            REQUIRE(note.set_frequency(frequency) == 0);
            REQUIRE(note.note == static_cast<Fourier::Note>(pitch));
            REQUIRE(note.octave == octave);
            REQUIRE(note.note_string() == names[pitch] + std::to_string(octave));
            // Float frequency rounding is much smaller than 0.001 cent.
            REQUIRE(note.cents == Approx(0.f).margin(0.001f));
        }
    }
}

TEST_CASE("Cents offsets select the nearest note across octave boundaries") {
    for (int octave = -1; octave <= 8; ++octave) {
        const double c_frequency = 440.0 * std::exp2((12 * (octave - 4) - 9) / 12.0);
        for (float offset : {-51.f, -49.f, -25.f, 0.f, 25.f, 49.f, 51.f}) {
            const float frequency = c_frequency * std::exp2(offset / 1200.0);
            CAPTURE(octave, offset, frequency);
            const Fourier::TunedNote note(frequency);
            const auto expected_note = offset < -50.f ? Fourier::Note::B :
                offset > 50.f ? Fourier::Note::CSharp : Fourier::Note::C;
            const float expected_cents = offset < -50.f ? offset + 100.f :
                offset > 50.f ? offset - 100.f : offset;
            REQUIRE(note.note == expected_note);
            REQUIRE(note.octave == (offset < -50.f ? octave - 1 : octave));
            REQUIRE(note.cents == Approx(expected_cents).margin(0.001f));
        }
    }
}

TEST_CASE("Hover tuning strings preserve the sign and two decimal places") {
    const Fourier::TunedNote reference;
    REQUIRE(reference.note_string() == "A4");
    REQUIRE(reference.tuning_string() == "+0.00 cents");
    for (float offset : {-25.f, 25.f}) {
        const Fourier::TunedNote note(220.0 * std::exp2(offset / 1200.0));
        REQUIRE(note.note_string() == "A3");
        REQUIRE(note.cents == Approx(offset).margin(0.001f));
        REQUIRE(note.tuning_string() == (offset < 0 ? "-25.00 cents" : "+25.00 cents"));
    }
}

TEST_CASE("Invalid frequencies preserve an initialized note") {
    const float invalid = GENERATE(0.f, -0.f, -1.f, -440.f,
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN());
    CAPTURE(invalid);
    Fourier::TunedNote note(220.0 * std::exp2(25.0 / 1200.0));
    const Fourier::TunedNote previous = note;
    REQUIRE(note.set_frequency(invalid) == 1);
    REQUIRE(note.note == previous.note);
    REQUIRE(note.octave == previous.octave);
    REQUIRE(note.cents == previous.cents);
    REQUIRE(note.note_string() == "A3");
    REQUIRE(note.tuning_string() == "+25.00 cents");
    REQUIRE(note.set_frequency(440.f) == 0);
    REQUIRE(note.note_string() == "A4");
    REQUIRE(note.cents == 0.f);
}

TEST_CASE("Invalid construction falls back to A4 with zero cents") {
    const float invalid = GENERATE(0.f, -0.f, -1.f,
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN());
    CAPTURE(invalid);
    const Fourier::TunedNote note(invalid);
    REQUIRE(note.note == Fourier::Note::A);
    REQUIRE(note.octave == 4);
    REQUIRE(note.cents == 0.f);
    REQUIRE(note.note_string() == "A4");
    REQUIRE(note.tuning_string() == "+0.00 cents");
}

TEST_CASE("Positive finite float extremes produce finite tuning") {
    const float frequency = GENERATE(std::numeric_limits<float>::denorm_min(),
        std::numeric_limits<float>::min(), std::numeric_limits<float>::max());
    CAPTURE(frequency);
    Fourier::TunedNote note;
    REQUIRE(note.set_frequency(frequency) == 0);
    REQUIRE(static_cast<int>(note.note) >= 0);
    REQUIRE(static_cast<int>(note.note) < 12);
    REQUIRE(std::isfinite(note.cents));
    REQUIRE(std::abs(note.cents) <= 50.f);
    // Reconstruct in double so subnormal/maximum float inputs cannot
    // underflow/overflow the reference calculation.
    const int midi_note = 12 * (note.octave + 1) + static_cast<int>(note.note);
    const double reconstructed = 440.0 *
        std::exp2((midi_note - 69) / 12.0 + note.cents / 1200.0);
    REQUIRE(reconstructed == Approx(static_cast<double>(frequency)).epsilon(1e-6));
}

TEST_CASE("Note names reject identifiers outside the chromatic scale") {
    for (int value : {-1, 12})
        CHECK_THROWS_AS(Fourier::to_string(static_cast<Fourier::Note>(value)),
                        std::runtime_error);
}
