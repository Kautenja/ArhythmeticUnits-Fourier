# Keep Rack's public build/package interface and SDK-free developer targets.
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

include mk/standalone.mk

# Only explicit standalone goals can bypass the SDK. Mixed invocations still
# load Rack, while standalone flags were captured before plugin.mk modifies them.
SDK_FREE_GOALS := test test-dsp test-mailbox test-build benchmark benchmark-build clean clean-local check-build
SDK_FREE_GOALS += $(STANDALONE_TEST_ALIASES) $(STANDALONE_BENCHMARK_ALIASES)
SDK_FREE_GOALS += $(filter .build/test/standalone/% .build/benchmark/standalone/%,$(MAKECMDGOALS))
SDK_FREE_GOALS += $(foreach goal,$(filter .build/instrumented/%,$(MAKECMDGOALS)),$(if $(findstring /standalone/,$(goal)),$(goal)))
ifneq ($(strip $(filter-out $(SDK_FREE_GOALS),$(or $(MAKECMDGOALS),all))),)
FLAGS += -DTEST -Wno-unused-local-typedefs
SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += LICENSE LICENSING.md res presets
RACK_DIR ?= ../..

# Rack's compile.mk hardcodes build/. Supply its object/dependency variables
# before inclusion so the SDK's link, dist and install recipes remain intact.
override OBJECTS := $(patsubst %,.build/%.o,$(SOURCES)) $(OBJECTS) $(patsubst %,.build/%.bin.o,$(BINARIES))
override DEPENDENCIES := $(patsubst %,.build/%.d,$(SOURCES))
include $(RACK_DIR)/plugin.mk

.build/%.cpp.o: %.cpp Makefile mk/rack.mk
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

include mk/rack.mk
clean: clean-local
else
.PHONY: clean
clean: clean-local
	rm -f plugin.so plugin.dylib plugin.dll
	rm -rf dist
endif

.PHONY: clean-local check-build
clean-local:
	rm -rf .build

check-build:
	python3 scripts/test-build.py
