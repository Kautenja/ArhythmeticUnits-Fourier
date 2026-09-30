#!/usr/bin/env python3
"""Compile the shared capability registry into the optional benchmark executable."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import argparse

from contracts import load_registry


def generate(destination, features=()):
    registry = load_registry(features=features)
    fields = list(next(iter(registry.values())))
    def literal(value):
        return json.dumps(value)
    def ctype(value):
        return 'bool' if type(value) is bool else 'size_t' if type(value) is int else 'const char*'
    lines = ['// Generated from docs/whitepaper/benchmarks/backends.json; do not edit.', 'struct BackendDescriptor {']
    first = next(iter(registry.values()))
    lines += [f'    {ctype(first[key])} {key};' for key in fields]
    lines += ['};', 'static const BackendDescriptor backend_registry[] = {']
    for item in registry.values():
        lines.append('    {'+', '.join(literal(item[key]) for key in fields)+'},')
    lines += ['};', 'static const char* registry_json = '+literal(json.dumps(registry, sort_keys=True))+';', '']
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    content = '\n'.join(lines)
    if not destination.exists() or destination.read_text() != content:
        destination.write_text(content)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination")
    parser.add_argument("--features", default="")
    args = parser.parse_args()
    generate(args.destination, args.features.split() if args.features else ())
