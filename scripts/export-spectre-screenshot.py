"""Compatibility entry point for the original Spectre screenshot command."""

import sys

from export_manual_screenshot import main


if __name__ == "__main__":
    main(["--module", "spectre", *sys.argv[1:]])
