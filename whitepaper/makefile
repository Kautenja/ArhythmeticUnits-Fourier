# Build the standalone report without running experiments.
.PHONY: all pdf check arxiv
all: pdf

pdf:
	latexmk -pdf -interaction=nonstopmode -halt-on-error -file-line-error -outdir=build -jobname=Fourier-Technical-Report fourier.tex

check:
	python3 experiments/check.py

arxiv: pdf check
	COPYFILE_DISABLE=1 tar -czf build/fourier-arxiv-source.tar.gz fourier.tex
