"""Repository paths shared by benchmark tools and their tests."""

from pathlib import Path

BENCHMARKS = Path(__file__).resolve().parents[1]
ROOT = BENCHMARKS.parents[2]
HISTORY = BENCHMARKS / "history"
