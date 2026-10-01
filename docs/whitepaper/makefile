# Typeset the self-contained paper; evidence validation is a separate target.
.PHONY: all generate check source arxiv clean-assets
.DEFAULT_GOAL := all
TEX_SOURCE := fourier.tex
PDF_NAME := paper
include ../latex/publication.mk

all: pdf

generate:
	$(PYTHON) tools/study_paper.py

check:
	$(PYTHON) tools/check_paper.py

source:
	mkdir -p $(BUILD)
	COPYFILE_DISABLE=1 tar -czf $(BUILD)/fourier-arxiv-source.tar.gz fourier.tex

arxiv: pdf check source

clean: clean-assets

clean-assets:
	rm -rf .build/paper-study
	rm -f $(BUILD)/fourier-arxiv-source.tar.gz
