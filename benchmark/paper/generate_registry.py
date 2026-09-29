#!/usr/bin/env python3
"""Compile the shared capability registry into the optional benchmark executable."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import sys

from contracts import load_registry


def generate(destination):
    registry = load_registry()
    fields = list(next(iter(registry.values())))
    def literal(value):
        return json.dumps(value)
    def ctype(value):
        return 'bool' if type(value) is bool else 'size_t' if type(value) is int else 'const char*'
    lines = ['// Generated from benchmark/paper/backends.json; do not edit.', 'struct BackendDescriptor {']
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
    generate(sys.argv[1])
