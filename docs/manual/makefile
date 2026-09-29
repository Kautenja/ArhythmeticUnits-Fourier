
.PHONY: all manual mFourier mSpectre

all: manual

build:
	mkdir -p build

mFourier: build
	$(MAKE) -C Fourier
	cp Fourier/build/manual.pdf build/Fourier.pdf

mSpectre: build
	$(MAKE) -C Spectre
	cp Spectre/build/manual.pdf build/Spectre.pdf

manual: mFourier mSpectre
