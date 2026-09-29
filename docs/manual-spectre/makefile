.PHONY: default manual screenshot

# the default when running `make` in this directory
default: manual

# the source tex file basename (no `.tex`)
SOURCE=manual
# the build directory
BUILD=.build

# File-types to clean out from the build folder after pdf compilation
CLEAN=*.aux *.nav *.log *.out *.snm *.toc *.bbl *.blg *.brf *.swp *.nlo *.spl
# a command for running pdflatex with options applied
PDFLATEX=pdflatex -halt-on-error -interaction=nonstopmode -file-line-error -no-shell-escape
PYTHON ?= python3

# Explicit refresh only: ordinary PDF builds use the reviewed, checked-in PNG.
# Requires the Rack build environment, a graphical desktop, and Python/Pillow.
screenshot:
	$(MAKE) -C ../.. inspect-panels
	$(PYTHON) ../../scripts/export-spectre-screenshot.py ../../.build/test/rack/panel-live-0.ppm img/PanelLayout.png

# make the manual
manual:
	rm -rf ${BUILD}
	mkdir -p ${BUILD}
	cp -r img ${BUILD}
	cp *.tex ${BUILD}
	cp ArhythmeticUnits.sty ${BUILD}
	cd ${BUILD} && ${PDFLATEX} ${SOURCE}
	cd ${BUILD} && ${PDFLATEX} ${SOURCE}
	cd ${BUILD} && rm -f ${CLEAN}
