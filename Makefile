FLAGS += \
	-DTEST \
	-Wno-unused-local-typedefs

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res presets

RACK_DIR ?= ../..
include $(RACK_DIR)/plugin.mk

# Exercise the actual module with a headless Rack engine.
.PHONY: test-serialization
test-serialization: build/test/rack/test_serialization
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $<

build/test/rack/test_serialization: build/test/rack/test_serialization.cpp.o
	$(CXX) $(CXXFLAGS) -o $@ $< -L$(RACK_DIR) -lRack

build/test/rack/test_serialization.cpp.o: CXXFLAGS += -Idep/Catch2/single_include/catch2
-include build/test/rack/test_serialization.cpp.d

# Exercise Spectre's real display with a headless NanoVG texture backend.
.PHONY: test-display-lifecycle
test-display-lifecycle: build/test/rack/test_display_lifecycle
	DYLD_LIBRARY_PATH="$(abspath $(RACK_DIR))" LD_LIBRARY_PATH="$(abspath $(RACK_DIR))" $<

build/test/rack/test_display_lifecycle: build/test/rack/test_display_lifecycle.cpp.o
	$(CXX) $(CXXFLAGS) -o $@ $< -L$(RACK_DIR) -lRack

build/test/rack/test_display_lifecycle.cpp.o: CXXFLAGS += -Idep/Catch2/single_include/catch2
-include build/test/rack/test_display_lifecycle.cpp.d

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
