"""Repository paths shared by benchmark tools and their tests."""

from pathlib import Path

BENCHMARKS = Path(__file__).resolve().parents[1]
ROOT = BENCHMARKS.parents[2]
ARCHIVE = ROOT / "docs/latex/deprecated/whitepaper"
HISTORY = ARCHIVE / "benchmarks/history"
