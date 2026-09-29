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
