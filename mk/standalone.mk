# SDK-free Catch2 suites. Keep these flags independent of Rack's globals.
INSTRUMENT ?=
ifneq ($(filter-out coverage asan-ubsan tsan,$(INSTRUMENT)),)
$(error INSTRUMENT must be coverage, asan-ubsan, or tsan)
endif
ifeq ($(INSTRUMENT),tsan)
ifneq ($(MAKECMDGOALS),test-mailbox)
$(error TSan is restricted to the standalone test-mailbox target)
endif
endif

STANDALONE_CXX := $(CXX)
ifeq ($(origin CXX),default)
STANDALONE_CXX := $(if $(INSTRUMENT),clang++,g++)
endif
STANDALONE_SUFFIX := $(if $(filter Windows_NT,$(OS)),.exe)
STANDALONE_CPPFLAGS := $(CPPFLAGS) -Isrc -Idep/Catch2 -Wno-unused-value
STANDALONE_TEST_FLAGS := $(CXXFLAGS) -std=c++14 -pthread -pipe -pedantic -Wall -Wextra
STANDALONE_BENCHMARK_FLAGS := $(CXXFLAGS) -std=c++14 -pthread -O3 -pipe -pedantic -Wall
STANDALONE_LDFLAGS := $(LDFLAGS)
ifeq ($(INSTRUMENT),coverage)
STANDALONE_TEST_FLAGS += -O0 -g -fprofile-instr-generate -fcoverage-mapping -femit-all-decls
else ifeq ($(INSTRUMENT),asan-ubsan)
STANDALONE_TEST_FLAGS += -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all
else ifeq ($(INSTRUMENT),tsan)
STANDALONE_TEST_FLAGS += -O1 -g -fno-omit-frame-pointer -fsanitize=thread
endif

STANDALONE_TEST_BUILD := $(if $(INSTRUMENT),.build/instrumented/$(INSTRUMENT)/standalone,.build/test/standalone)
STANDALONE_BENCHMARK_BUILD := .build/benchmark/standalone
STANDALONE_TEST_SOURCES := $(sort $(shell find test -name '*.cpp' ! -path 'test/rack/*'))
STANDALONE_BENCHMARK_SOURCES := $(sort $(shell find benchmark/dsp -name '*.cpp'))
STANDALONE_TEST_ALIASES := $(STANDALONE_TEST_SOURCES:.cpp=)
STANDALONE_BENCHMARK_ALIASES := $(STANDALONE_BENCHMARK_SOURCES:.cpp=)
STANDALONE_TEST_BINARIES := $(patsubst test/%.cpp,$(STANDALONE_TEST_BUILD)/%$(STANDALONE_SUFFIX),$(STANDALONE_TEST_SOURCES))
STANDALONE_BENCHMARK_BINARIES := $(patsubst benchmark/%.cpp,$(STANDALONE_BENCHMARK_BUILD)/%$(STANDALONE_SUFFIX),$(STANDALONE_BENCHMARK_SOURCES))
STANDALONE_TEST_OBJECTS := $(patsubst test/%.cpp,$(STANDALONE_TEST_BUILD)/%.o,$(STANDALONE_TEST_SOURCES))
STANDALONE_BENCHMARK_OBJECTS := $(patsubst benchmark/%.cpp,$(STANDALONE_BENCHMARK_BUILD)/%.o,$(STANDALONE_BENCHMARK_SOURCES))
STANDALONE_TEST_CATCH := $(STANDALONE_TEST_BUILD)/catch_amalgamated.o
STANDALONE_BENCHMARK_CATCH := $(STANDALONE_BENCHMARK_BUILD)/catch_amalgamated.o
TEST_ARGS ?=
BENCHMARK_ARGS ?=

.PHONY: test test-dsp test-mailbox test-build benchmark benchmark-build
.PHONY: $(STANDALONE_TEST_ALIASES) $(STANDALONE_BENCHMARK_ALIASES)
test: $(STANDALONE_TEST_ALIASES)
test-dsp: $(filter test/dsp/%,$(STANDALONE_TEST_ALIASES))
test-mailbox: test/threads/test_display_mailbox
test-build: $(STANDALONE_TEST_BINARIES)
benchmark-build: $(STANDALONE_BENCHMARK_BINARIES)

$(STANDALONE_TEST_ALIASES): test/%: $(STANDALONE_TEST_BUILD)/%$(STANDALONE_SUFFIX)
	$< $(TEST_ARGS)

# A single runner serializes timed suites even with -j or multiple aliases.
# Compilation of all selected binaries finishes before any timing starts.
STANDALONE_BENCHMARK_SELECTED := $(if $(filter benchmark,$(MAKECMDGOALS)),$(STANDALONE_BENCHMARK_BINARIES),$(patsubst benchmark/%,$(STANDALONE_BENCHMARK_BUILD)/%$(STANDALONE_SUFFIX),$(filter $(STANDALONE_BENCHMARK_ALIASES),$(MAKECMDGOALS))))
.PHONY: run-standalone-benchmarks
benchmark $(STANDALONE_BENCHMARK_ALIASES): run-standalone-benchmarks
run-standalone-benchmarks: $(STANDALONE_BENCHMARK_SELECTED)
	@set -e; for suite in $(STANDALONE_BENCHMARK_SELECTED); do "$$suite" $(BENCHMARK_ARGS); done

$(STANDALONE_TEST_BINARIES): $(STANDALONE_TEST_BUILD)/%$(STANDALONE_SUFFIX): $(STANDALONE_TEST_BUILD)/%.o $(STANDALONE_TEST_CATCH)
	$(STANDALONE_CXX) $(STANDALONE_TEST_FLAGS) -o $@ $^ $(STANDALONE_LDFLAGS)

$(STANDALONE_BENCHMARK_BINARIES): $(STANDALONE_BENCHMARK_BUILD)/%$(STANDALONE_SUFFIX): $(STANDALONE_BENCHMARK_BUILD)/%.o $(STANDALONE_BENCHMARK_CATCH)
	$(STANDALONE_CXX) $(STANDALONE_BENCHMARK_FLAGS) -o $@ $^ $(STANDALONE_LDFLAGS)

$(STANDALONE_TEST_OBJECTS): $(STANDALONE_TEST_BUILD)/%.o: test/%.cpp
$(STANDALONE_BENCHMARK_OBJECTS): $(STANDALONE_BENCHMARK_BUILD)/%.o: benchmark/%.cpp
$(STANDALONE_TEST_CATCH) $(STANDALONE_BENCHMARK_CATCH): dep/Catch2/catch_amalgamated.cpp

$(STANDALONE_TEST_OBJECTS) $(STANDALONE_TEST_CATCH): $(STANDALONE_TEST_BUILD)/config Makefile mk/standalone.mk
	@mkdir -p $(@D)
	$(STANDALONE_CXX) $(STANDALONE_CPPFLAGS) $(STANDALONE_TEST_FLAGS) -MMD -MP -c -o $@ $(filter %.cpp,$^)

$(STANDALONE_BENCHMARK_OBJECTS) $(STANDALONE_BENCHMARK_CATCH): $(STANDALONE_BENCHMARK_BUILD)/config Makefile mk/standalone.mk
	@mkdir -p $(@D)
	$(STANDALONE_CXX) $(STANDALONE_CPPFLAGS) $(STANDALONE_BENCHMARK_FLAGS) -MMD -MP -c -o $@ $(filter %.cpp,$^)

# Unlike SCons, Make does not automatically track command changes. Compare a
# configuration stamp without changing its mtime on an ordinary incremental run.
shell-quote = '$(subst ','"'"',$(1))'
.PHONY: FORCE
FORCE:
$(STANDALONE_TEST_BUILD)/config: PRIVATE_CONFIG := $(STANDALONE_CXX) $(STANDALONE_CPPFLAGS) $(STANDALONE_TEST_FLAGS) $(STANDALONE_LDFLAGS)
$(STANDALONE_BENCHMARK_BUILD)/config: PRIVATE_CONFIG := $(STANDALONE_CXX) $(STANDALONE_CPPFLAGS) $(STANDALONE_BENCHMARK_FLAGS) $(STANDALONE_LDFLAGS)
$(STANDALONE_TEST_BUILD)/config $(STANDALONE_BENCHMARK_BUILD)/config: FORCE
	@mkdir -p $(@D)
	@printf '%s\n' $(call shell-quote,$(PRIVATE_CONFIG)) > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

-include $(STANDALONE_TEST_OBJECTS:.o=.d) $(STANDALONE_TEST_CATCH:.o=.d)
-include $(STANDALONE_BENCHMARK_OBJECTS:.o=.d) $(STANDALONE_BENCHMARK_CATCH:.o=.d)
