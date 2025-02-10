// Test cases for IO::AudioBuffer structure.
//
// Copyright (c) 2020 Christian Kauten
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "dsp/io/audio_buffer.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: constructor
// ---------------------------------------------------------------------------

TEST_CASE("IO::AudioBuffer constructor should assign default values") {
    IO::AudioBuffer buffer;
    REQUIRE(1 == buffer.getChannels());
    REQUIRE(1 == buffer.getNumSamples());
    REQUIRE(buffer.isMono());
    REQUIRE_FALSE(buffer.isStereo());
    REQUIRE(44100 == buffer.getSampleRate());
    REQUIRE(32 == buffer.getBitDepth());
    REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
}

// ---------------------------------------------------------------------------
// MARK: loadWAV
// ---------------------------------------------------------------------------

SCENARIO("WAV files need to be loaded.") {
    GIVEN("an invalid path.") {
        IO::AudioBuffer buffer;
        std::string path = "./foo.wav";
        WHEN("the file is loaded") {
            auto error = buffer.loadWAV(path);
            THEN("an error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::InvalidFile == error);
            }
            THEN("the buffer is not updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(1 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
                REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
            }
        }
    }
    GIVEN("a path to a 44100kHz 8-bit Mono wav file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/44100-8bit-unsigned-int/pulse/0.wav";
        WHEN("the file is loaded") {
            auto error = buffer.loadWAV(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(100 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(8 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 44100kHz 16-bit Mono wav file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/44100-16bit-signed-int/pulse/0.wav";
        WHEN("the file is loaded") {
            auto error = buffer.loadWAV(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(100 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 44100kHz 32-bit Mono wav file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/44100-32bit-float/pulse/0.wav";
        WHEN("the file is loaded") {
            auto error = buffer.loadWAV(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(100 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 44100kHz 64-bit Mono wav file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/44100-64bit-float/pulse/0.wav";
        WHEN("the file is loaded") {
            auto error = buffer.loadWAV(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(100 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(64 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 88200kHz 32-bit Mono wav file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/88200-32bit-float/pulse/0.wav";
        WHEN("the file is loaded") {
            auto error = buffer.loadWAV(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(200 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(88200 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 176400kHz 32-bit Mono wav file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/176400-32bit-float/pulse/0.wav";
        WHEN("the file is loaded") {
            auto error = buffer.loadWAV(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(400 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(176400 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 352800kHz 32-bit Mono wav file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/352800-32bit-float/pulse/0.wav";
        WHEN("the file is loaded") {
            auto error = buffer.loadWAV(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(800 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(352800 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: loadFLAC
// ---------------------------------------------------------------------------

SCENARIO("FLAC files need to be loaded.") {
    GIVEN("an invalid path.") {
        IO::AudioBuffer buffer;
        std::string path = "./foo.flac";
        WHEN("the file is loaded") {
            auto error = buffer.loadFLAC(path);
            THEN("an error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::InvalidFile == error);
            }
            THEN("the buffer is not updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(1 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
                REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
            }
        }
    }
    GIVEN("a path to a 44100kHz 16-bit Mono FLAC file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/flacs/44100/pulse/0.flac";
        WHEN("the file is loaded") {
            auto error = buffer.loadFLAC(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(100 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 88200kHz 16-bit Mono FLAC file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/flacs/88200/pulse/0.flac";
        WHEN("the file is loaded") {
            auto error = buffer.loadFLAC(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(200 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(88200 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 176400kHz 16-bit Mono FLAC file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/flacs/176400/pulse/0.flac";
        WHEN("the file is loaded") {
            auto error = buffer.loadFLAC(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(400 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(176400 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("a path to a 352800kHz 16-bit Mono FLAC file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/flacs/352800/pulse/0.flac";
        WHEN("the file is loaded") {
            auto error = buffer.loadFLAC(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(800 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(352800 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: loadMP3
// ---------------------------------------------------------------------------

SCENARIO("MP3 files need to be loaded.") {
    GIVEN("an invalid path.") {
        IO::AudioBuffer buffer;
        std::string path = "./foo.mp3";
        WHEN("the file is loaded") {
            auto error = buffer.loadMP3(path);
            THEN("an error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::InvalidFile == error);
            }
            THEN("the buffer is not updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(1 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
                REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
            }
        }
    }
    GIVEN("a path to a 44100kHz 16-bit Mono MP3 file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/mp3/44100/pulse/0.mp3";
        WHEN("the file is loaded") {
            auto error = buffer.loadMP3(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                // number of samples is changed by pydub on write. it zero pads.
                REQUIRE(44100+3132 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: load
// ---------------------------------------------------------------------------

SCENARIO("files need to be loaded with arbitrary extensions.") {
    GIVEN("an invalid extension.") {
        IO::AudioBuffer buffer;
        std::string path = "./foo.txt";
        WHEN("the file is not loaded") {
            auto error = buffer.load(path);
            THEN("an error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::InvalidExtension == error);
            }
            THEN("the buffer is not updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(1 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
                REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
            }
        }
    }
    GIVEN("an invalid WAV path.") {
        IO::AudioBuffer buffer;
        std::string path = "./foo.wav";
        WHEN("the file is loaded") {
            auto error = buffer.load(path);
            THEN("an error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::InvalidFile == error);
            }
            THEN("the buffer is not updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(1 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
                REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
            }
        }
    }
    GIVEN("a path to a 44100kHz 16-bit Mono wav file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/44100-16bit-signed-int/pulse/0.wav";
        WHEN("the file is loaded") {
            auto error = buffer.load(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(100 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("an invalid FLAC path.") {
        IO::AudioBuffer buffer;
        std::string path = "./foo.flac";
        WHEN("the file is loaded") {
            auto error = buffer.load(path);
            THEN("an error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::InvalidFile == error);
            }
            THEN("the buffer is not updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(1 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
                REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
            }
        }
    }
    GIVEN("a path to a 44100kHz 16-bit Mono FLAC file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/flacs/44100/pulse/0.flac";
        WHEN("the file is loaded") {
            auto error = buffer.load(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(100 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
    GIVEN("an invalid MP3 path.") {
        IO::AudioBuffer buffer;
        std::string path = "./foo.mp3";
        WHEN("the file is loaded") {
            auto error = buffer.load(path);
            THEN("an error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::InvalidFile == error);
            }
            THEN("the buffer is not updated.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(1 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
                REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
            }
        }
    }
    GIVEN("a path to a 44100kHz 16-bit Mono MP3 file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/mp3/44100/pulse/0.mp3";
        WHEN("the file is loaded") {
            auto error = buffer.load(path);
            THEN("no error is returned") {
                REQUIRE(IO::AudioBuffer::LoadStatus::Success == error);
            }
            THEN("the buffer is updated.") {
                REQUIRE(1 == buffer.getChannels());
                // number of samples is changed by pydub on write. it zero pads.
                REQUIRE(44100+3132 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(16 == buffer.getBitDepth());
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: unload
// ---------------------------------------------------------------------------

SCENARIO("Files need to be unloaded.") {
    GIVEN("a buffer with a loaded WAV file.") {
        IO::AudioBuffer buffer;
        std::string path = "./test/dsp/io/waves/176400-32bit-float/pulse/0.wav";
        WHEN("the file is unloaded") {
            buffer.load(path);
            buffer.unload();
            THEN("the buffer is cleared.") {
                REQUIRE(1 == buffer.getChannels());
                REQUIRE(1 == buffer.getNumSamples());
                REQUIRE(buffer.isMono());
                REQUIRE_FALSE(buffer.isStereo());
                REQUIRE(44100 == buffer.getSampleRate());
                REQUIRE(32 == buffer.getBitDepth());
                REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: removeDC
// ---------------------------------------------------------------------------

// SCENARIO("DC Offsets need to be removed.") {
//     GIVEN("an unloaded IO::AudioBuffer") {
//         IO::AudioBuffer buffer;
//         WHEN("removeDC() is called") {
//             buffer.removeDC();
//             THEN("the buffer does not change") {
//                 REQUIRE(0 == buffer.getSample(IO::AudioBuffer::Channel::Mono, 0));
//             }
//         }
//     }
//     // TODO:
// }
