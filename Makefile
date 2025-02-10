FLAGS += \
	-DTEST \
	-Wno-unused-local-typedefs

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/modules/*.cpp)
SOURCES += $(wildcard src/modules/MakeNoise/*.cpp)
SOURCES += $(wildcard src/modules/MinBlep/*.cpp)
SOURCES += $(wildcard src/modules/SignalAnalysis/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../..
include $(RACK_DIR)/plugin.mk
