# Shared build rules. Invoked from docs/manual-{fourier,spectre}/.
.PHONY: manual screenshot
.DEFAULT_GOAL := manual
TEX_SOURCE := manual.tex
PDF_NAME := manual
include ../latex/publication.mk
# Source-relative inputs; no copies of TeX or artwork in the output directory.
export TEXINPUTS := ../latex//:$(TEXINPUTS):

manual: pdf

# Explicit refresh only; ordinary builds use the reviewed cover image.
screenshot:
	$(MAKE) -C ../.. inspect-panels
	$(PYTHON) ../../scripts/export_manual_screenshot.py --module $(MODULE) ../../.build/test/rack/panel-live-0.ppm img/PanelLayout.png
