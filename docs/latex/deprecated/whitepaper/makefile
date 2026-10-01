# The extended historical report is independent of the active paper build.
.PHONY: all prepare check clean-assets
.DEFAULT_GOAL := all
TEX_SOURCE := fourier.tex
PDF_NAME := report-v4
include ../../publication.mk

all: pdf

prepare:
	$(PYTHON) tools/report_assets.py

pdf check: prepare

check:
	$(PYTHON) tools/check_history.py

clean: clean-assets

clean-assets:
	rm -rf .build/paper-study .build/paper-comparison
