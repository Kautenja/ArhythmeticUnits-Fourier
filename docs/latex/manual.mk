# Shared build rules. Invoked from docs/manual-{fourier,spectre}/.
.PHONY: manual clean screenshot
.DEFAULT_GOAL := manual
BUILD ?= .build
LATEXMK ?= latexmk
PYTHON ?= python3
# Source-relative inputs; no copies of TeX or artwork in the output directory.
export TEXINPUTS := ../latex//:$(TEXINPUTS):

manual:
	$(LATEXMK) -pdf -pdflatex='pdflatex -no-shell-escape %O %S' -interaction=nonstopmode -halt-on-error -file-line-error -outdir=$(BUILD) -jobname=manual manual.tex
	$(LATEXMK) -c -outdir=$(BUILD) -jobname=manual manual.tex

clean:
	$(LATEXMK) -C -outdir=$(BUILD) -jobname=manual manual.tex

# Explicit refresh only; ordinary builds use the reviewed cover image.
screenshot:
	$(MAKE) -C ../.. inspect-panels
	$(PYTHON) ../../scripts/export_manual_screenshot.py --module $(MODULE) ../../.build/test/rack/panel-live-0.ppm img/PanelLayout.png
