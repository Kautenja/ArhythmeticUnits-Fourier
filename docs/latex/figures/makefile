# Standalone vector exports; ordinary manual builds do not need this target.
.PHONY: all
PYTHON ?= python3
PDFLATEX ?= pdflatex
PDFTOCAIRO ?= pdftocairo
MODULE ?= all

all:
	$(PYTHON) ../../../scripts/export_panel_drawings.py --module $(MODULE) --pdflatex "$(PDFLATEX)" --pdftocairo "$(PDFTOCAIRO)"
