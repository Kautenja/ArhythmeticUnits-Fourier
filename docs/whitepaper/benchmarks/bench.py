#!/usr/bin/env python3
"""One entry point for the whitepaper's measurement and evidence workflow."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "lib"))
from workflow import main

if __name__ == "__main__":
    raise SystemExit(main())
