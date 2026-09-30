# Build the standalone report without running experiments.
.PHONY: all check arxiv
.DEFAULT_GOAL := all
TEX_SOURCE := fourier.tex
PDF_NAME := paper
include ../latex/publication.mk

all: pdf

check:
	$(PYTHON) tools/check_paper.py

arxiv: pdf check
	$(PYTHON) tools/manuscript.py --archive $(BUILD)/fourier-arxiv-source.tar.gz
