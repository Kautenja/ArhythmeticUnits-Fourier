FLAGS += \
	-DTEST \
	-Wno-unused-local-typedefs

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res presets

RACK_DIR ?= ../..
include $(RACK_DIR)/plugin.mk

# Headless suites share build flags; instrumentation stays out of the plugin.
RACK_TEST_INSTRUMENT ?=
RACK_TEST_BUILD := build/test/rack
RACK_TEST_FLAGS := $(CXXFLAGS) -pthread -Idep/Catch2/single_include/catch2
ifdef ARCH_LIN
RACK_TEST_FLAGS += -DCATCH_CONFIG_NO_POSIX_SIGNALS
endif
ifneq ($(RACK_TEST_INSTRUMENT),)
RACK_TEST_BUILD := build/instrumented/$(RACK_TEST_INSTRUMENT)/rack
# Clang does not support GCC's -fno-gnu-unique. Preserve SDK ABI and
# floating-point flags while keeping useful sanitizer stacks.
RACK_TEST_FLAGS := $(filter-out -fno-gnu-unique,$(RACK_TEST_FLAGS)) -g -fno-omit-frame-pointer
ifeq ($(RACK_TEST_INSTRUMENT),coverage)
# Keep complete inline mappings across the separate test executables.
RACK_TEST_FLAGS += -fprofile-instr-generate -fcoverage-mapping -femit-all-decls
else ifeq ($(RACK_TEST_INSTRUMENT),asan-ubsan)
RACK_TEST_FLAGS := $(filter-out -O%,$(RACK_TEST_FLAGS)) -O1 -fsanitize=address,undefined -fno-sanitize-recover=all
else
$(error RACK_TEST_INSTRUMENT must be coverage or asan-ubsan)
endif
endif

RACK_TEST_NAMES := test_serialization test_display_lifecycle test_spectrum_points test_dc_blocker test_module_amplitudes
RACK_TEST_BINARIES := $(addprefix $(RACK_TEST_BUILD)/,$(RACK_TEST_NAMES))
.PHONY: test-rack test-serialization test-display-lifecycle test-spectrum-points test-dc-blocker-simd test-module-amplitudes

test-rack: test-serialization test-display-lifecycle test-spectrum-points test-dc-blocker-simd test-module-amplitudes
test-serialization: $(RACK_TEST_BUILD)/test_serialization
test-display-lifecycle: $(RACK_TEST_BUILD)/test_display_lifecycle
test-spectrum-points: $(RACK_TEST_BUILD)/test_spectrum_points
test-dc-blocker-simd: $(RACK_TEST_BUILD)/test_dc_blocker
test-module-amplitudes: $(RACK_TEST_BUILD)/test_module_amplitudes

test-serialization test-display-lifecycle test-spectrum-points test-dc-blocker-simd test-module-amplitudes:
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $<

$(RACK_TEST_BINARIES): $(RACK_TEST_BUILD)/%: $(RACK_TEST_BUILD)/%.cpp.o
	$(CXX) $(RACK_TEST_FLAGS) -o $@ $< $(if $(filter test_dc_blocker,$*),,-L$(RACK_DIR) -lRack)

$(addsuffix .cpp.o,$(RACK_TEST_BINARIES)): $(RACK_TEST_BUILD)/%.cpp.o: test/rack/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(RACK_TEST_FLAGS) -c -o $@ $<

-include $(addsuffix .cpp.d,$(RACK_TEST_BINARIES))

# CPU-side display preparation only; the instrumented renderer does not use GL.
.PHONY: benchmark-display
benchmark-display: build/benchmark/rack/display
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $<

build/benchmark/rack/display: build/benchmark/rack/display.cpp.o
	$(CXX) $(CXXFLAGS) -o $@ $< -L$(RACK_DIR) -lRack

build/benchmark/rack/display.cpp.o: CXXFLAGS += $(DISPLAY_BENCHMARK_FLAGS)
-include build/benchmark/rack/display.cpp.d

# Optional native OpenGL inspection; requires a graphical desktop session.
ifdef ARCH_MAC
DISPLAY_GL_LIBS = -framework OpenGL
else ifdef ARCH_WIN
DISPLAY_GL_LIBS = -lopengl32
else
DISPLAY_GL_LIBS = -lGL
endif
.PHONY: inspect-displays
inspect-displays: build/test/rack/inspect_displays
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $< "$(abspath $(RACK_DIR))" "$(CURDIR)" "$(abspath build/test/rack/display)"

build/test/rack/inspect_displays: build/test/rack/inspect_displays.cpp.o
	$(CXX) $(CXXFLAGS) -o $@ $< -L$(RACK_DIR) -lRack $(DISPLAY_GL_LIBS)

-include build/test/rack/inspect_displays.cpp.d
