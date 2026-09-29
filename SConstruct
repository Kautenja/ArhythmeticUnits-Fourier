"""The compilation script for this project using SCons."""
import os
import fnmatch
import re
import shlex

# Instrumented tests never reuse ordinary objects or production flags.
INSTRUMENT = ARGUMENTS.get('INSTRUMENT', '')
INSTRUMENT_FLAGS = {
    '': [],
    # Emit inline bodies even when unused in a suite, so multi-binary LLVM
    # reports cannot prefer an empty mapping over another suite's execution.
    'coverage': ['-O0', '-g', '-fprofile-instr-generate', '-fcoverage-mapping',
                 '-femit-all-decls'],
    'asan-ubsan': ['-O1', '-g', '-fno-omit-frame-pointer',
                   '-fsanitize=address,undefined', '-fno-sanitize-recover=all'],
    'tsan': ['-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=thread'],
}
if INSTRUMENT not in INSTRUMENT_FLAGS:
    Exit('INSTRUMENT must be coverage, asan-ubsan, or tsan')
if INSTRUMENT == 'tsan' and COMMAND_LINE_TARGETS != ['test-mailbox']:
    Exit('TSan is restricted to the standalone test-mailbox target')
TEST_BUILD = ('build/instrumented/' + INSTRUMENT + '/standalone'
              if INSTRUMENT else 'build_test')
TEST_CXX = ARGUMENTS.get('CXX', 'clang++' if INSTRUMENT else 'g++')

# create a separate build directory
VariantDir('build_src', 'src/dsp', duplicate=0)
VariantDir('build_benchmark', 'benchmark', duplicate=0)
VariantDir(TEST_BUILD, 'test', duplicate=0)

# the compiler and linker flags for the production C++ environment
PROD_FLAGS = [
    '-std=c++11',
    '-pthread',
    '-O3',
    # '-march=native',
    '-pipe',
    '-pedantic',
    '-Wall'
]

# include for the production environment
INCLUDES = [
    '#src',
]

# the compiler and linker flags for the testing C++ environment
TEST_FLAGS = [
    '-std=c++14',
    '-pthread',
    # '-march=native',
    '-pipe',
    '-pedantic',
    '-Wall'
]

# include for the testing and benchmarking environment
TEST_INCLUDES = [
    '#dep/Catch2',
]

TESTING_ENV = Environment(
    ENV=os.environ,
    CXX=TEST_CXX,
    CPPFLAGS=['-Wno-unused-value', '-Wall', '-Wextra'],
    CXXFLAGS=TEST_FLAGS + INSTRUMENT_FLAGS[INSTRUMENT],
    LINKFLAGS=TEST_FLAGS + INSTRUMENT_FLAGS[INSTRUMENT],
    CPPPATH=INCLUDES + TEST_INCLUDES,
)

BENCHMARK_ENV = Environment(
    ENV=os.environ,
    CXX=ARGUMENTS.get('CXX', 'g++'),
    CPPFLAGS=['-Wno-unused-value'],
    CXXFLAGS=PROD_FLAGS + ['-std=c++14'],
    LINKFLAGS=PROD_FLAGS + ['-std=c++14'],
    CPPPATH=INCLUDES + TEST_INCLUDES,
)

PRODUCTION_ENV = Environment(
    ENV=os.environ,
    CXX='g++',
    CPPFLAGS=['-Wno-unused-value'],
    CXXFLAGS=PROD_FLAGS,
    LINKFLAGS=PROD_FLAGS,
    CPPPATH=INCLUDES,
)


# Compile Catch2's implementation and main once per flag/instrumentation set.
CATCH_SOURCE = 'dep/Catch2/catch_amalgamated.cpp'
TEST_CATCH = TESTING_ENV.Object(TEST_BUILD + '/catch_amalgamated', CATCH_SOURCE)
BENCHMARK_CATCH = BENCHMARK_ENV.Object('build_benchmark/catch_amalgamated', CATCH_SOURCE)


def find_source_files(src_dir, build_dir):
    """
    Find all the source files in the given directory.

    Args:
        src_dir: the source directory to search through
        build_dir: the build directory (to replace src_dir with)

    Returns:
        a list of paths to cpp files where src_dir is replaced by build_dir

    """
    files = []
    for root, dirnames, filenames in os.walk(src_dir):
        root = re.sub(src_dir, build_dir, root)
        for filename in fnmatch.filter(filenames, '*.cpp'):
            files.append(os.path.join(root, filename))
    return sorted(files)


# Locate all the C++ source files (TODO main CPP file for building library)
SRC = find_source_files('src/dsp', 'build_src')
# create separate object files for testing and production environments
TEST_SRC = [TESTING_ENV.Object(f.replace('.cpp', '') + '-test-' + (INSTRUMENT or 'plain'), f) for f in SRC]
PROD_SRC = [PRODUCTION_ENV.Object(f.replace('.cpp', '') + '-prod', f) for f in SRC]
BENCHMARK_SRC = [BENCHMARK_ENV.Object(f.replace('.cpp', '') + '-bench', f) for f in SRC]


# ----------------------------------------------------------------------------
# MARK: Unit Tests
# ----------------------------------------------------------------------------


# locate all the testing source files
TEST_FILES = find_source_files('test', TEST_BUILD)
# Rack integration tests use the SDK and are built by Make.
TEST_FILES = [file for file in TEST_FILES if not file.startswith(TEST_BUILD + '/rack/')]
UNIT_TEST_ALIASES = []
DSP_TEST_ALIASES = []
MAILBOX_TEST_ALIASES = []
for file in TEST_FILES:
    program = TESTING_ENV.Program(file.replace('.cpp', ''), [file] + TEST_SRC + TEST_CATCH)
    relative = file[len(TEST_BUILD) + 1:]
    alias = TESTING_ENV.Alias('test/' + relative, [program], program[0].path)
    AlwaysBuild(alias)
    UNIT_TEST_ALIASES.append(alias)
    if relative.startswith('dsp/'):
        DSP_TEST_ALIASES.append(alias)
    elif relative == 'threads/test_display_mailbox.cpp':
        MAILBOX_TEST_ALIASES.append(alias)

Alias('test', UNIT_TEST_ALIASES)
Alias('test-dsp', DSP_TEST_ALIASES)
Alias('test-mailbox', MAILBOX_TEST_ALIASES)


# ----------------------------------------------------------------------------
# MARK: Benchmarks
# ----------------------------------------------------------------------------


# Rack benchmarks require the SDK and are built separately by Make.
BENCHMARK_ALIASES = []
BENCHMARK_PROGRAMS = []
BENCHMARK_ARGS = ' '.join(shlex.quote(arg) for arg in
                          shlex.split(ARGUMENTS.get('BENCHMARK_ARGS', '')))
for benchmark in find_source_files('benchmark/dsp', 'build_benchmark/dsp'):
    program = BENCHMARK_ENV.Program(benchmark.replace('.cpp', ''), [benchmark] + BENCHMARK_SRC + BENCHMARK_CATCH)
    alias = BENCHMARK_ENV.Alias(benchmark.replace('build_', ''), [program],
                               program[0].path + ' ' + BENCHMARK_ARGS)
    AlwaysBuild(alias)
    # Allow parallel compilation without timing two suites concurrently.
    SideEffect('build_benchmark/timing-lock', alias)
    BENCHMARK_PROGRAMS.append(program)
    BENCHMARK_ALIASES.append(alias)

Alias('benchmark-build', BENCHMARK_PROGRAMS)
Alias('benchmark', BENCHMARK_ALIASES)


# ----------------------------------------------------------------------------
# MARK: DSP Library
# ----------------------------------------------------------------------------

# Create a shared library (it will add "lib" to the front automatically)
lib = PRODUCTION_ENV.SharedLibrary('_KautenjaDSP.so', SRC)
AlwaysBuild(lib)
