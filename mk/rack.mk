# Headless suites share build flags; instrumentation stays out of the plugin.
RACK_TEST_INSTRUMENT ?=
RACK_TEST_BUILD := .build/test/rack
# Catch2 supplies main(), not the SDK's Windows Unicode entry point.
RACK_TEST_FLAGS := $(filter-out -std=% -municode,$(CXXFLAGS)) -std=c++14 -pthread -Idep/Catch2
ifneq ($(RACK_TEST_INSTRUMENT),)
RACK_TEST_BUILD := .build/instrumented/$(RACK_TEST_INSTRUMENT)/rack
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
# MinGW emits .exe files; name the actual targets to avoid needless rebuilds.
RACK_TEST_SUFFIX := $(if $(ARCH_WIN),.exe)
RACK_TEST_BINARIES := $(addprefix $(RACK_TEST_BUILD)/,$(addsuffix $(RACK_TEST_SUFFIX),$(RACK_TEST_NAMES)))
.PHONY: test-rack test-serialization test-display-lifecycle test-spectrum-points test-dc-blocker-simd test-module-amplitudes

test-rack: test-serialization test-display-lifecycle test-spectrum-points test-dc-blocker-simd test-module-amplitudes
test-serialization: $(RACK_TEST_BUILD)/test_serialization$(RACK_TEST_SUFFIX)
test-display-lifecycle: $(RACK_TEST_BUILD)/test_display_lifecycle$(RACK_TEST_SUFFIX)
test-spectrum-points: $(RACK_TEST_BUILD)/test_spectrum_points$(RACK_TEST_SUFFIX)
test-dc-blocker-simd: $(RACK_TEST_BUILD)/test_dc_blocker$(RACK_TEST_SUFFIX)
test-module-amplitudes: $(RACK_TEST_BUILD)/test_module_amplitudes$(RACK_TEST_SUFFIX)

test-serialization test-display-lifecycle test-spectrum-points test-dc-blocker-simd test-module-amplitudes:
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $<

$(RACK_TEST_BINARIES): $(RACK_TEST_BUILD)/%$(RACK_TEST_SUFFIX): $(RACK_TEST_BUILD)/%.cpp.o $(RACK_TEST_BUILD)/catch_amalgamated.cpp.o
	$(CXX) $(RACK_TEST_FLAGS) -o $@ $^ $(if $(filter test_dc_blocker,$*),,-L$(RACK_DIR) -lRack)

$(addprefix $(RACK_TEST_BUILD)/,$(addsuffix .cpp.o,$(RACK_TEST_NAMES))): $(RACK_TEST_BUILD)/%.cpp.o: test/rack/%.cpp Makefile mk/rack.mk
	@mkdir -p $(@D)
	$(CXX) $(RACK_TEST_FLAGS) -c -o $@ $<

# Keep instrumented Catch2 objects separate from ordinary tests and benchmarks.
$(RACK_TEST_BUILD)/catch_amalgamated.cpp.o: dep/Catch2/catch_amalgamated.cpp Makefile mk/rack.mk
	@mkdir -p $(@D)
	$(CXX) $(RACK_TEST_FLAGS) -c -o $@ $<

-include $(addprefix $(RACK_TEST_BUILD)/,$(addsuffix .cpp.d,$(RACK_TEST_NAMES))) $(RACK_TEST_BUILD)/catch_amalgamated.cpp.d

# CPU-side display preparation only; the instrumented renderer does not use GL.
.PHONY: benchmark-display
benchmark-display: .build/benchmark/rack/display$(RACK_TEST_SUFFIX)
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $<

.build/benchmark/rack/display$(RACK_TEST_SUFFIX): .build/benchmark/rack/display.cpp.o
	$(CXX) $(filter-out -municode,$(CXXFLAGS)) -o $@ $< -L$(RACK_DIR) -lRack

.build/benchmark/rack/display.cpp.o: CXXFLAGS += $(DISPLAY_BENCHMARK_FLAGS)
-include .build/benchmark/rack/display.cpp.d

# Catch2 Rack benchmarks use SDK optimization and floating-point flags.
BENCHMARK_ARGS ?=
RACK_BENCHMARK_FLAGS = $(filter-out -std=% -municode,$(CXXFLAGS)) -std=c++14 -pthread -Idep/Catch2
RACK_BENCHMARK_NAMES := dsp coordinates graphics modules
RACK_BENCHMARK_OBJECTS := $(addprefix .build/benchmark/rack/,$(addsuffix .cpp.o,$(RACK_BENCHMARK_NAMES)))
RACK_BENCHMARK_BINARIES := $(addprefix .build/benchmark/rack/,$(addsuffix $(RACK_TEST_SUFFIX),$(RACK_BENCHMARK_NAMES)))
.PHONY: benchmark-dsp benchmark-coordinates benchmark-graphics benchmark-modules benchmark-rack-build
benchmark-rack-build: $(RACK_BENCHMARK_BINARIES)
benchmark-dsp: .build/benchmark/rack/dsp$(RACK_TEST_SUFFIX)
benchmark-coordinates: .build/benchmark/rack/coordinates$(RACK_TEST_SUFFIX)
benchmark-graphics: .build/benchmark/rack/graphics$(RACK_TEST_SUFFIX)
benchmark-modules: .build/benchmark/rack/modules$(RACK_TEST_SUFFIX)

benchmark-dsp benchmark-coordinates benchmark-graphics benchmark-modules:
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $< $(BENCHMARK_ARGS)

$(RACK_BENCHMARK_BINARIES): .build/benchmark/rack/%$(RACK_TEST_SUFFIX): .build/benchmark/rack/%.cpp.o .build/benchmark/rack/catch_amalgamated.cpp.o
	$(CXX) $(RACK_BENCHMARK_FLAGS) -o $@ $^ -L$(RACK_DIR) -lRack

$(RACK_BENCHMARK_OBJECTS): .build/benchmark/rack/%.cpp.o: benchmark/rack/%.cpp Makefile mk/rack.mk
	@mkdir -p $(@D)
	$(CXX) $(RACK_BENCHMARK_FLAGS) -c -o $@ $<

.build/benchmark/rack/catch_amalgamated.cpp.o: dep/Catch2/catch_amalgamated.cpp Makefile mk/rack.mk
	@mkdir -p $(@D)
	$(CXX) $(RACK_BENCHMARK_FLAGS) -c -o $@ $<

-include $(RACK_BENCHMARK_OBJECTS:.o=.d) .build/benchmark/rack/catch_amalgamated.cpp.d

# Optional native OpenGL inspection; requires a graphical desktop session.
ifdef ARCH_MAC
DISPLAY_GL_LIBS = -framework OpenGL
else ifdef ARCH_WIN
DISPLAY_GL_LIBS = -lopengl32
else
DISPLAY_GL_LIBS = -lGL
endif
.PHONY: inspect-displays
inspect-displays: .build/test/rack/inspect_displays$(RACK_TEST_SUFFIX)
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $< "$(abspath $(RACK_DIR))" "$(CURDIR)" "$(abspath .build/test/rack/display)"

.build/test/rack/inspect_displays$(RACK_TEST_SUFFIX): .build/test/rack/inspect_displays.cpp.o
	$(CXX) $(filter-out -municode,$(CXXFLAGS)) -o $@ $< -L$(RACK_DIR) -lRack $(DISPLAY_GL_LIBS)

-include .build/test/rack/inspect_displays.cpp.d

.PHONY: inspect-panels
PANEL_INSPECT_BINARY := .build/test/rack/inspect_panels$(if $(ARCH_WIN),.exe)
inspect-panels: $(PANEL_INSPECT_BINARY)
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $< "$(abspath $(RACK_DIR))" "$(CURDIR)" "$(abspath .build/test/rack/panel)"

$(PANEL_INSPECT_BINARY): .build/test/rack/inspect_panels.cpp.o
	$(CXX) $(filter-out -municode,$(CXXFLAGS)) -o $@ $< -L$(RACK_DIR) -lRack $(DISPLAY_GL_LIBS)

-include .build/test/rack/inspect_panels.cpp.d

# Raw paper observations, separate from Catch2's batched mean estimator.
.PHONY: benchmark-paper-build
benchmark-paper-build: .build/benchmark/rack/paper$(RACK_TEST_SUFFIX) .build/benchmark/rack/paper-audit$(RACK_TEST_SUFFIX)
benchmark-rack-build: benchmark-paper-build

.build/benchmark/rack/paper$(RACK_TEST_SUFFIX): .build/benchmark/rack/paper.cpp.o
	$(CXX) $(filter-out -municode,$(CXXFLAGS)) -o $@ $< -L$(RACK_DIR) -lRack

-include .build/benchmark/rack/paper.cpp.d

.build/benchmark/registry.generated.hpp: benchmark/paper/backends.json benchmark/paper/generate_registry.py benchmark/paper/contracts.py
	python3 benchmark/paper/generate_registry.py $@

.build/benchmark/rack/paper.cpp.o: benchmark/rack/paper.cpp .build/benchmark/registry.generated.hpp Makefile mk/rack.mk
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I.build/benchmark -c -o $@ $<

.build/benchmark/rack/paper-audit.cpp.o: benchmark/rack/paper.cpp .build/benchmark/registry.generated.hpp Makefile mk/rack.mk .build/rack-config
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I.build/benchmark -DPAPER_ALLOCATION_AUDIT -c -o $@ $<

.build/benchmark/rack/paper-audit$(RACK_TEST_SUFFIX): .build/benchmark/rack/paper-audit.cpp.o
	$(CXX) $(filter-out -municode,$(CXXFLAGS)) -o $@ $< -L$(RACK_DIR) -lRack

-include .build/benchmark/rack/paper-audit.cpp.d

# Rebuild when the compiler, SDK path, or effective flags change. Stamps
# never enter a link command; only the objects depend on them.
.build/rack-config: PRIVATE_CONFIG := $(CXX) $(CXXFLAGS) $(LDFLAGS) $(DISPLAY_BENCHMARK_FLAGS) $(abspath $(RACK_DIR))
$(RACK_TEST_BUILD)/config: PRIVATE_CONFIG := $(CXX) $(RACK_TEST_FLAGS) $(abspath $(RACK_DIR))
.build/benchmark/rack/config: PRIVATE_CONFIG := $(CXX) $(RACK_BENCHMARK_FLAGS) $(abspath $(RACK_DIR))
.build/rack-config $(RACK_TEST_BUILD)/config .build/benchmark/rack/config: FORCE
	@mkdir -p $(@D)
	@printf '%s\n' $(call shell-quote,$(PRIVATE_CONFIG)) > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

$(OBJECTS) .build/benchmark/rack/display.cpp.o .build/benchmark/rack/paper.cpp.o .build/test/rack/inspect_displays.cpp.o .build/test/rack/inspect_panels.cpp.o: .build/rack-config
$(addprefix $(RACK_TEST_BUILD)/,$(addsuffix .cpp.o,$(RACK_TEST_NAMES))) $(RACK_TEST_BUILD)/catch_amalgamated.cpp.o: $(RACK_TEST_BUILD)/config
$(RACK_BENCHMARK_OBJECTS) .build/benchmark/rack/catch_amalgamated.cpp.o: .build/benchmark/rack/config
