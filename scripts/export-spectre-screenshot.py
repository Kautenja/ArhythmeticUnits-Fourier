"""Crop Spectre from inspect_panels' light-theme live scenario (requires Pillow)."""

import argparse
from pathlib import Path

from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="panel-live-0.ppm from inspect-panels")
    parser.add_argument("destination", type=Path, help="output PNG")
    args = parser.parse_args()
    if args.source.name != "panel-live-0.ppm":
        parser.error("use the unzoomed light-theme live scenario, panel-live-0.ppm")
    if args.destination.suffix.lower() != ".png":
        parser.error("destination must be a PNG file")

    # Keep this geometry aligned with test/rack/inspect_panels.cpp: a
    # 1280 x 410 canvas, Spectre at (740, 15), with a 525 x 380 panel.
    # Preserve native pixels; never resize, repaint, or synthesize UI content.
    with Image.open(args.source) as source:
        density, remainder = divmod(source.width, 1280)
        if (source.format != "PPM" or source.mode != "RGB" or density < 1
                or remainder or source.height != 410 * density):
            parser.error("unexpected inspector canvas; review the capture geometry")
        bounds = tuple(value * density for value in (740, 15, 1265, 395))
        screenshot = source.crop(bounds)
        screenshot.save(args.destination, format="PNG")
    print(f"Saved {args.destination} ({screenshot.width} x {screenshot.height})")


if __name__ == "__main__":
    main()
