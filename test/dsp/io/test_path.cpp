// Test cases for IO::Path package.
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

#include "dsp/io/path.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// ---------------------------------------------------------------------------
// MARK: IO::Path::getExtension
// ---------------------------------------------------------------------------

SCENARIO("Extensions need to be extracted from paths") {
    GIVEN("a basic filename") {
        std::string path = "filename.txt";
        WHEN("the extension is parsed") {
            std::string extension = IO::Path::getExtension(path);
            THEN("the extension should be \"txt\"") {
                REQUIRE(0 == extension.compare("txt"));
            }
        }
    }
    GIVEN("a compound filename") {
        std::string path = "foo.filename.txt";
        WHEN("the extension is parsed") {
            std::string extension = IO::Path::getExtension(path);
            THEN("the extension should be \"txt\"") {
                REQUIRE(0 == extension.compare("txt"));
            }
        }
    }
    GIVEN("a POSIX path") {
        std::string path = "folder/filename.txt";
        WHEN("the extension is parsed") {
            std::string extension = IO::Path::getExtension(path);
            THEN("the extension should be \"txt\"") {
                REQUIRE(0 == extension.compare("txt"));
            }
        }
    }
    GIVEN("a compound POSIX path") {
        std::string path = "folder/foo.filename.txt";
        WHEN("the extension is parsed") {
            std::string extension = IO::Path::getExtension(path);
            THEN("the extension should be \"txt\"") {
                REQUIRE(0 == extension.compare("txt"));
            }
        }
    }
    GIVEN("a Windows path") {
        std::string path = "C:\\folder\\filename.txt";
        WHEN("the extension is parsed") {
            std::string extension = IO::Path::getExtension(path);
            THEN("the extension should be \"txt\"") {
                REQUIRE(0 == extension.compare("txt"));
            }
        }
    }
    GIVEN("a compound Windows path") {
        std::string path = "C:\\folder\\foo.filename.txt";
        WHEN("the extension is parsed") {
            std::string extension = IO::Path::getExtension(path);
            THEN("the extension should be \"txt\"") {
                REQUIRE(0 == extension.compare("txt"));
            }
        }
    }
    GIVEN("a filename with a dot but no extension") {
        std::string path = "filename.";
        WHEN("the extension is parsed") {
            std::string extension = IO::Path::getExtension(path);
            THEN("the extension should be \"\"") {
                REQUIRE(0 == extension.compare(""));
            }
        }
    }
    GIVEN("a filename with no dot and no extension") {
        std::string path = "filename";
        WHEN("the extension is parsed") {
            std::string extension = IO::Path::getExtension(path);
            THEN("the extension should be \"\"") {
                REQUIRE(0 == extension.compare(""));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: IO::Path::getBasename
// ---------------------------------------------------------------------------

SCENARIO("Basenames need to be extracted from paths") {
    GIVEN("a basic filename") {
        std::string path = "filename.txt";
        WHEN("the basename is parsed") {
            std::string basename = IO::Path::getBasename(path);
            THEN("the basename should be the full path") {
                REQUIRE(0 == basename.compare(path));
            }
        }
    }
    GIVEN("a compound filename") {
        std::string path = "foo.filename.txt";
        WHEN("the basename is parsed") {
            std::string basename = IO::Path::getBasename(path);
            THEN("the basename should be the full path") {
                REQUIRE(0 == basename.compare(path));
            }
        }
    }
    GIVEN("a POSIX path") {
        std::string path = "folder/filename.txt";
        WHEN("the basename is parsed") {
            std::string basename = IO::Path::getBasename(path);
            THEN("the basename should be \"filename.txt\"") {
                REQUIRE(0 == basename.compare("filename.txt"));
            }
        }
    }
    GIVEN("a compound POSIX path") {
        std::string path = "folder/foo.filename.txt";
        WHEN("the basename is parsed") {
            std::string basename = IO::Path::getBasename(path);
            THEN("the basename should be \"foo.filename.txt\"") {
                REQUIRE(0 == basename.compare("foo.filename.txt"));
            }
        }
    }
    GIVEN("a Windows path") {
        std::string path = "C:\\folder\\filename.txt";
        WHEN("the basename is parsed") {
            std::string basename = IO::Path::getBasename(path);
            THEN("the basename should be \"filename.txt\"") {
                REQUIRE(0 == basename.compare("filename.txt"));
            }
        }
    }
    GIVEN("a compound Windows path") {
        std::string path = "C:\\folder\\foo.filename.txt";
        WHEN("the basename is parsed") {
            std::string basename = IO::Path::getBasename(path);
            THEN("the basename should be \"foo.filename.txt\"") {
                REQUIRE(0 == basename.compare("foo.filename.txt"));
            }
        }
    }
    GIVEN("a filename with a POSIX delimiter but no basename") {
        std::string path = "folder/";
        WHEN("the basename is parsed") {
            std::string basename = IO::Path::getBasename(path);
            THEN("the basename should be \"\"") {
                REQUIRE(0 == basename.compare(""));
            }
        }
    }
    GIVEN("a filename with a Windows delimiter but no basename") {
        std::string path = "C:\\folder\\";
        WHEN("the basename is parsed") {
            std::string basename = IO::Path::getBasename(path);
            THEN("the basename should be \"\"") {
                REQUIRE(0 == basename.compare(""));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// MARK: IO::Path::getDirname
// ---------------------------------------------------------------------------

SCENARIO("Directory names need to be extracted from paths") {
    GIVEN("a basic filename") {
        std::string path = "filename.txt";
        WHEN("the directory name is parsed") {
            std::string dirname = IO::Path::getDirname(path);
            THEN("the directory name should be \"\"") {
                REQUIRE(0 == dirname.compare(""));
            }
        }
    }
    GIVEN("a compound filename") {
        std::string path = "foo.filename.txt";
        WHEN("the directory name is parsed") {
            std::string dirname = IO::Path::getDirname(path);
            THEN("the directory name should be \"\"") {
                REQUIRE(0 == dirname.compare(""));
            }
        }
    }
    GIVEN("a POSIX path") {
        std::string path = "folder/filename.txt";
        WHEN("the directory name is parsed") {
            std::string dirname = IO::Path::getDirname(path);
            THEN("the directory name should be \"folder\"") {
                REQUIRE(0 == dirname.compare("folder"));
            }
        }
    }
    GIVEN("a compound POSIX path") {
        std::string path = "folder/foo.filename.txt";
        WHEN("the directory name is parsed") {
            std::string dirname = IO::Path::getDirname(path);
            THEN("the directory name should be \"folder\"") {
                REQUIRE(0 == dirname.compare("folder"));
            }
        }
    }
    GIVEN("a Windows path") {
        std::string path = "C:\\folder\\filename.txt";
        WHEN("the directory name is parsed") {
            std::string dirname = IO::Path::getDirname(path);
            THEN("the directory name should be \"filename.txt\"") {
                REQUIRE(0 == dirname.compare("C:\\folder"));
            }
        }
    }
    GIVEN("a compound Windows path") {
        std::string path = "C:\\folder\\foo.filename.txt";
        WHEN("the directory name is parsed") {
            std::string dirname = IO::Path::getDirname(path);
            THEN("the directory name should be \"foo.filename.txt\"") {
                REQUIRE(0 == dirname.compare("C:\\folder"));
            }
        }
    }
    GIVEN("a filename with a POSIX delimiter but no directory name") {
        std::string path = "folder/";
        WHEN("the directory name is parsed") {
            std::string dirname = IO::Path::getDirname(path);
            THEN("the directory name should be the input path without terminal delimiter") {
                REQUIRE(0 == dirname.compare("folder"));
            }
        }
    }
    GIVEN("a filename with a Windows delimiter but no directory name") {
        std::string path = "C:\\folder\\";
        WHEN("the directory name is parsed") {
            std::string dirname = IO::Path::getDirname(path);
            THEN("the directory name should be the input path without terminal delimiter") {
                REQUIRE(0 == dirname.compare("C:\\folder"));
            }
        }
    }
}
