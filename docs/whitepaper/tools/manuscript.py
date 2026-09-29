#!/usr/bin/env python3
"""Expand the manuscript's literal input lines for checks and portable export.

Paths are relative to the whitepaper directory, just as in its LaTeX build.
This deliberately supports only standalone \\input{path.tex} lines, not TeX
macro expansion. Keep input directives out of comments and code listings.
"""
import argparse
import io
from pathlib import Path
import re
import tarfile


PAPER = Path(__file__).resolve().parents[1]
INPUT = re.compile(r'^\\input\{([^}]+)\}\n?', re.MULTILINE)


def read_manuscript(directory=PAPER):
    """Return the complete source, preserving the included text verbatim."""
    root = directory.resolve()

    def expand(path, parents=()):
        path = path.resolve()
        if not path.is_relative_to(root) or path.suffix != '.tex':
            raise ValueError(f'Input must be a TeX source inside {root}: {path}')
        if path in parents:
            raise ValueError(f'Cyclic manuscript input: {path}')
        source = path.read_text()
        # Reject unsupported directives instead of silently exporting a partial
        # manuscript. All project input directives occupy their own line.
        for line in source.splitlines():
            if line.lstrip().startswith(r'\input') and not re.fullmatch(r'\\input\{[^}]+\}', line):
                raise ValueError(f'Input must occupy its own line in {path}: {line}')
        return INPUT.sub(lambda match: expand(root / match[1], (*parents, path)), source)

    return expand(root / 'fourier.tex')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True,
                        help='Write a gzip archive containing standalone fourier.tex')
    args = parser.parse_args()
    source = read_manuscript().encode()
    args.archive.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(args.archive, 'w:gz') as archive:
        member = tarfile.TarInfo('fourier.tex')
        member.size = len(source)
        member.mode = 0o644
        archive.addfile(member, io.BytesIO(source))
    print(f'Wrote standalone manuscript: {args.archive}')


if __name__ == '__main__':
    main()
