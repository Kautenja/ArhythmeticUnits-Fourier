"""The compilation script for this project using SCons."""
import os
import fnmatch
import re

# create a separate build directory
VariantDir('build_src', 'src/dsp', duplicate=0)
VariantDir('build_benchmark', 'benchmark', duplicate=0)
VariantDir('build_test', 'test', duplicate=0)

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
    '-std=c++11',
    '-pthread',
    # '-march=native',
    '-pipe',
    '-pedantic',
    '-Wall'
]

# include for the testing and benchmarking environment
TEST_INCLUDES = [
    '#dep/Catch2/single_include/catch2',
]

TESTING_ENV = Environment(
    ENV=os.environ,
    CXX='g++',
    CPPFLAGS=['-Wno-unused-value', '-Wall', '-Wextra'],
    CXXFLAGS=TEST_FLAGS,
    LINKFLAGS=TEST_FLAGS,
    CPPPATH=INCLUDES + TEST_INCLUDES,
)

BENCHMARK_ENV = Environment(
    ENV=os.environ,
    CXX='g++',
    CPPFLAGS=['-Wno-unused-value'],
    CXXFLAGS=PROD_FLAGS,
    LINKFLAGS=PROD_FLAGS,
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
TEST_SRC = [TESTING_ENV.Object(f.replace('.cpp', '') + '-test', f) for f in SRC]
PROD_SRC = [PRODUCTION_ENV.Object(f.replace('.cpp', '') + '-prod', f) for f in SRC]
BENCHMARK_SRC = [BENCHMARK_ENV.Object(f.replace('.cpp', '') + '-bench', f) for f in SRC]


# ----------------------------------------------------------------------------
# MARK: Unit Tests
# ----------------------------------------------------------------------------


# locate all the testing source files
TEST_FILES = find_source_files('test', 'build_test')
# create a list to store all the test target aliases in
UNIT_TEST_ALIASES = []
for file in TEST_FILES:  # iterate over all the test source files
    UNIT_TEST_PROGRAM = TESTING_ENV.Program(file.replace('.cpp', ''), [file] + TEST_SRC)
    UNIT_TEST_ALIASES.append(Alias('test/' + file.replace('build_test/', ''), [UNIT_TEST_PROGRAM], UNIT_TEST_PROGRAM[0].path))
    AlwaysBuild(UNIT_TEST_ALIASES[-1])

# create an alias to run all test suites
Alias("test", UNIT_TEST_ALIASES)


# ----------------------------------------------------------------------------
# MARK: Benchmarks
# ----------------------------------------------------------------------------


# create a list to store all the benchmark target aliases in
BENCHMARK_ALIASES = []
for benchmark in find_source_files('benchmark', 'build_benchmark'):
    BENCHMARK_PROGRAM = BENCHMARK_ENV.Program(benchmark.replace('.cpp', ''), [benchmark] + BENCHMARK_SRC)
    BENCHMARK_ALIASES.append(Alias(benchmark.replace('build_', ''), [BENCHMARK_PROGRAM], BENCHMARK_PROGRAM[0].path))
    AlwaysBuild(BENCHMARK_ALIASES[-1])

# create an alias to run all test suites
Alias("benchmark", BENCHMARK_ALIASES)


# ----------------------------------------------------------------------------
# MARK: DSP Library
# ----------------------------------------------------------------------------

# Create a shared library (it will add "lib" to the front automatically)
lib = PRODUCTION_ENV.SharedLibrary('_KautenjaDSP.so', SRC)
AlwaysBuild(lib)
