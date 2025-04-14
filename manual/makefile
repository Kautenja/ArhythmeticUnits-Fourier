
all: manual

build:
	mkdir -p build

mFourier: build
	make -C Fourier
	cp Fourier/build/manual.pdf build/Fourier.pdf

mSpectre: build
	make -C Spectre
	cp Spectre/build/manual.pdf build/Spectre.pdf

manual: mFourier mSpectre
