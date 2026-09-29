// Basic mathematical functions.
//
// Copyright 2025 Arhythmetic Units
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_WESTERN_SCALE_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_WESTERN_SCALE_HPP_

#include <cmath>      // isfinite, log2, round
#include <complex>    // complex
#include <algorithm>  // max, min
#include <string>     // string
#include <iomanip>    // setprecision
#include <stdexcept>  // runtime_error
#include <sstream>    // ostringstream

/// @brief Music theoretical concepts.
namespace Fourier {

/// @brief Western scale exponentially spaced notes.
enum class Note {
    C = 0,
    CSharp,
    D,
    DSharp,
    E,
    F,
    FSharp,
    G,
    GSharp,
    A,
    ASharp,
    B
};

/// @brief Convert a note to its string representation.
/// @param note The note to concert to a string.
/// @returns The string representation of the string note.
inline const char* to_string(const Note& note) {
    switch (note) {
    case Note::C:      return "C";
    case Note::CSharp: return "C#";
    case Note::D:      return "D";
    case Note::DSharp: return "D#";
    case Note::E:      return "E";
    case Note::F:      return "F";
    case Note::FSharp: return "F#";
    case Note::G:      return "G";
    case Note::GSharp: return "G#";
    case Note::A:      return "A";
    case Note::ASharp: return "A#";
    case Note::B:      return "B";
    default: throw std::runtime_error("Invalid note " + std::to_string(static_cast<int>(note)));
    }
}

/// @brief A structure to hold note information.
struct TunedNote {
    /// The note.
    enum Note note;
    /// Octave number.
    int octave;
    /// Cents deviation.
    float cents;

    /// @brief Initialize a new tuned note to A4 +0 cents.
    TunedNote() : note(Note::A), octave(4), cents(0) { }

    /// @brief Initialize a musical note by frequency.
    /// @param freq The frequency in Hz; invalid input leaves A4 +0 cents.
    explicit TunedNote(float freq) : TunedNote() { set_frequency(freq); }

    /// @brief Convert frequency to a musical note.
    /// @param freq A positive, finite frequency in Hz.
    /// @returns 0 on success, or 1 for invalid input without changing the note.
    /// @details Half-semitone ties round away from A4. Cents are the signed
    /// deviation from the nearest note, in the range [-50, 50].
    inline int set_frequency(float freq) {
        if (!std::isfinite(freq) || freq <= 0.f) return 1;
        // Double intermediates keep even subnormal float frequencies valid.
        const double n = 12.0 * std::log2(static_cast<double>(freq) / 440.0);
        const int nearest_note = static_cast<int>(std::round(n));
        // Align to C, then use a nonnegative remainder for floor division.
        int note_index = (nearest_note + 9) % 12;
        if (note_index < 0) note_index += 12;
        octave = 4 + (nearest_note + 9 - note_index) / 12;
        note = static_cast<Note>(note_index);
        cents = static_cast<float>(100.0 * (n - nearest_note));
        return 0;
    }

    /// @brief Return a string representation of the note and octave.
    inline std::string note_string() const {
        return to_string(note) + std::to_string(octave);
    }

    /// @brief Return a string representation of the cents tuning.
    inline std::string tuning_string() const {
        std::ostringstream stream;
        stream << (cents >= 0 ? "+" : "");  // add a "+" for positive values.
        stream << std::fixed << std::setprecision(2) << cents << " cents";
        return stream.str();
    }
};

}  // namespace Fourier

#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_WESTERN_SCALE_HPP_
