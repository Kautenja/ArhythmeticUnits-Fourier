#!/usr/bin/env python3
"""Exercise the real Make rules with tiny C++ fixtures, without a Rack SDK."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BuildTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='fourier-build-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        shutil.copy(ROOT / 'Makefile', self.root)
        shutil.copytree(ROOT / 'mk', self.root / 'mk')
        for directory in ('src', 'test/dsp', 'test/threads', 'benchmark/dsp',
                          'dep/Catch2', 'sdk'):
            (self.root / directory).mkdir(parents=True, exist_ok=True)
        (self.root / 'dep/Catch2/catch_amalgamated.cpp').write_text('int catch_fixture;\n')
        (self.root / 'src/value.hpp').write_text('#define VALUE 0\n')
        source = '''#include "value.hpp"
#ifdef RACK_FIXTURE
#error Rack flags leaked into a standalone suite
#endif
int main() { return VALUE; }
'''
        for name in ('test/dsp/test_one.cpp', 'test/dsp/test_two.cpp',
                     'test/threads/test_display_mailbox.cpp'):
            (self.root / name).write_text(source)
        # mkdir is atomic, so concurrently timed suites reliably fail.
        benchmark = '''#include <fstream>
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <direct.h>
#define LOCK() _mkdir("timing-lock")
#define UNLOCK() _rmdir("timing-lock")
#else
#include <sys/stat.h>
#include <unistd.h>
#define LOCK() mkdir("timing-lock", 0700)
#define UNLOCK() rmdir("timing-lock")
#endif
int main() {
    if (LOCK()) return 42;
    std::ofstream("runs", std::ios::app) << "timed\\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return UNLOCK();
}
'''
        for name in ('one', 'two'):
            (self.root / f'benchmark/dsp/{name}.cpp').write_text(benchmark)
        (self.root / 'src/plugin.cpp').write_text('int main() { return 0; }\n')
        # Model the SDK's hardcoded build paths and global flag mutations.
        (self.root / 'sdk/plugin.mk').write_text('''CXXFLAGS += -std=c++11 -ffast-math -DRACK_FIXTURE
TARGET := plugin.so
OBJECTS := $(patsubst %,build/%.o,$(SOURCES))
DEPENDENCIES := $(patsubst %,build/%.d,$(SOURCES))
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^
-include $(DEPENDENCIES)
clean:
	rm -rf build $(TARGET) dist
.DEFAULT_GOAL := all
''')

    def make(self, *args, success=True):
        env = os.environ.copy()
        # A parent make check-build must not inject its jobserver/goals.
        for name in ('MAKEFLAGS', 'MFLAGS', 'MAKELEVEL'):
            env.pop(name, None)
        result = subprocess.run(['make', '-j4', 'RACK_DIR=missing-sdk', *args],
                                cwd=self.root, env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if success:
            self.assertEqual(result.returncode, 0, result.stdout)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout)
        return result.stdout

    def test_sdk_free_and_incremental_rebuilds(self):
        self.make('test')
        output = self.make('test')
        self.assertNotIn(' -c ', output)
        self.assertIn('.build/test/standalone/dsp/test_one', output)
        # Older filesystems/Make versions compare timestamps at second precision.
        time.sleep(1.1)
        (self.root / 'src/value.hpp').write_text('#define VALUE 0\n// changed\n')
        output = self.make('test')
        self.assertIn('test_one.cpp', output)
        self.assertNotIn('catch_amalgamated.cpp', output)
        output = self.make('test', 'CXXFLAGS=-DCHANGED')
        self.assertIn('catch_amalgamated.cpp', output)
        self.assertNotIn(' -c ', self.make('test', 'CXXFLAGS=-DCHANGED'))
        self.assertFalse((self.root / 'build').exists())

    def test_failure_propagation(self):
        (self.root / 'src/value.hpp').write_text('#define VALUE 1\n')
        self.make('-k', 'test', success=False)

    def test_benchmark_build_and_serial_execution(self):
        self.make('benchmark-build')
        self.assertFalse((self.root / 'runs').exists())
        self.make('benchmark/dsp/one', 'benchmark/dsp/two')
        self.assertEqual((self.root / 'runs').read_text().splitlines(), ['timed'] * 2)
        self.make('benchmark')
        self.assertEqual(len((self.root / 'runs').read_text().splitlines()), 4)

    def test_rack_default_and_mixed_goals(self):
        self.make('RACK_DIR=sdk')
        self.assertTrue((self.root / 'plugin.so').is_file())
        self.assertTrue((self.root / '.build/src/plugin.cpp.o').is_file())
        self.assertFalse((self.root / 'build').exists())
        self.make('RACK_DIR=sdk', 'all', 'test')
        self.make('clean')
        self.assertFalse((self.root / '.build').exists())
        self.assertFalse((self.root / 'plugin.so').exists())

    def test_instrumentation_validation_and_paths(self):
        self.make('-n', 'test', 'INSTRUMENT=invalid', success=False)
        self.make('-n', 'test', 'INSTRUMENT=tsan', success=False)
        output = self.make('-n', 'test-mailbox', 'INSTRUMENT=tsan')
        self.assertIn('.build/instrumented/tsan/standalone/', output)
        self.assertIn('-fsanitize=thread', output)
        self.assertNotIn('test_one.cpp', output)


if __name__ == '__main__':
    unittest.main()
