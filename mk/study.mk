# Offline research preparation and launch. Run has no build prerequisites.
STUDY_PACKAGE ?= .build/study-014/prepared
STUDY_RACK_DIR ?= ../..
STUDY_FFTW_PREFIX ?= .build/deps/fftw
STUDY_CXX ?= c++
SESSION ?=
.PHONY: benchmark-study-prepare benchmark-study-run benchmark-study-check
benchmark-study-prepare:
	python3 docs/whitepaper/benchmarks/bench.py study-prepare --package "$(STUDY_PACKAGE)" --rack-dir "$(STUDY_RACK_DIR)" --fftw-prefix "$(STUDY_FFTW_PREFIX)" --cxx "$(STUDY_CXX)"
benchmark-study-run:
	python3 docs/whitepaper/benchmarks/bench.py study-run --package "$(STUDY_PACKAGE)" --session "$(SESSION)"
benchmark-study-check:
	python3 docs/whitepaper/benchmarks/bench.py study-check --package "$(STUDY_PACKAGE)"
