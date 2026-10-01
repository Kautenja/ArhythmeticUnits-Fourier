# Common PDF build lifecycle; publication sources and styles stay independent.
# Callers set TEX_SOURCE and PDF_NAME before including this file.
.PHONY: pdf clean
BUILD ?= .build
LATEXMK ?= latexmk
PYTHON ?= python3

pdf:
	$(LATEXMK) -pdf -pdflatex='pdflatex -no-shell-escape %O %S' -interaction=nonstopmode -halt-on-error -file-line-error -outdir=$(BUILD) -jobname=$(PDF_NAME) $(TEX_SOURCE)
	$(LATEXMK) -c -outdir=$(BUILD) -jobname=$(PDF_NAME) $(TEX_SOURCE)

# Remove only this publication's generated files, preserving experiment output.
clean:
	$(LATEXMK) -C -outdir=$(BUILD) -jobname=$(PDF_NAME) $(TEX_SOURCE)
