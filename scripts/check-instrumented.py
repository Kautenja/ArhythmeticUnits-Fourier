#!/usr/bin/env python3
"""Run isolated test instrumentation and produce first-party LLVM coverage."""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def llvm_tool(name):
    """Honor explicit tools, then PATH, then Xcode's bundled LLVM tools."""
    override = os.environ.get(name.upper().replace('-', '_'))
    if override:
        return override
    found = shutil.which(name)
    if found:
        return found
    if sys.platform == 'darwin':
        return subprocess.check_output(['xcrun', '--find', name], text=True).strip()
    raise RuntimeError(f'{name} is required; use tools from the same LLVM release as CXX')


def run(command, env, log, check=True):
    """Keep full diagnostics and fail on compiler, assertion, or runtime errors."""
    print('+ ' + shlex.join(map(str, command)), flush=True)
    with log.open('a') as output:
        output.write('\n+ ' + shlex.join(map(str, command)) + '\n')
        output.flush()
        result = subprocess.run(command, cwd=ROOT, env=env, stdout=output,
                                stderr=subprocess.STDOUT, timeout=900)
    if result.returncode and check:
        print(log.read_text(), file=sys.stderr)
        raise RuntimeError(f'command failed ({result.returncode}); see {log}')
    return result.returncode


def report_coverage(suite, binaries, report, env, log, test_status):
    profiles = sorted((report / 'raw').glob('*.profraw'))
    if not profiles or any(not binary.is_file() for binary in binaries):
        raise RuntimeError('missing coverage profiles or expected test executables')
    profdata = llvm_tool('llvm-profdata')
    cov = llvm_tool('llvm-cov')
    run([profdata, '--version'], env, log)
    run([cov, '--version'], env, log)
    merged = report / 'coverage.profdata'
    run([profdata, 'merge', '-sparse', *profiles, '-o', merged], env, log)
    # Whitelist first-party source paths: no Catch2, fixtures, SDK, or STL.
    # Rack's report includes the DSP templates instantiated by its modules.
    source_root = ROOT / 'src' / 'dsp' if suite == 'dsp' else ROOT / 'src'
    sources = sorted(p for p in source_root.rglob('*')
                     if p.suffix in ('.hpp', '.cpp'))
    common = [str(binaries[0]), f'-instr-profile={merged}']
    for binary in binaries[1:]:
        common += ['-object', str(binary)]
    common += ['-sources', *map(str, sources)]
    for filename, options in [
        ('summary.txt', ['report']),
        ('coverage.lcov', ['export', '-format=lcov']),
        ('coverage.json', ['export']),
    ]:
        with (report / filename).open('w') as output:
            subprocess.run([cov, *options, *common], cwd=ROOT, env=env,
                           stdout=output, check=True, timeout=120)
    run([cov, 'show', '-format=html', f'-output-dir={report / "html"}',
         *common], env, log)
    summary = report / 'summary.txt'
    result = 'PASS' if test_status == 0 else 'FAIL (coverage from a failed test run)'
    summary.write_text(f'Test run: {result}\n\n' + summary.read_text())
    print(summary.read_text())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('coverage', 'asan-ubsan', 'tsan'))
    parser.add_argument('suite', choices=('dsp', 'rack', 'mailbox'))
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--rack-dir', type=Path,
                        default=Path(os.environ.get('RACK_DIR', ROOT / '../..')))
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    if (args.mode == 'tsan') != (args.suite == 'mailbox'):
        parser.error('TSan runs only the mailbox suite; DSP/Rack use coverage or asan-ubsan')
    if sys.platform not in ('darwin', 'linux'):
        parser.error('instrumented checks currently support Linux and macOS')

    env = os.environ.copy()
    compiler = env.get('CXX', 'clang++')
    report = ROOT / '.build' / 'reports' / args.mode / args.suite
    # Old reports/profiles must never make a failed or partial rerun look valid.
    if report.exists():
        shutil.rmtree(report)
    report.mkdir(parents=True)
    log = report / 'run.log'
    run([compiler, '--version'], env, log)
    if args.mode == 'coverage':
        llvm_tool('llvm-profdata')
        llvm_tool('llvm-cov')
        (report / 'raw').mkdir()
        env['LLVM_PROFILE_FILE'] = str(report / 'raw' / '%m-%p.profraw')
    elif args.mode == 'asan-ubsan':
        env['ASAN_OPTIONS'] = 'halt_on_error=1'
        env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
    else:
        env['TSAN_OPTIONS'] = 'halt_on_error=1:exitcode=66'

    build = ROOT / '.build' / 'instrumented' / args.mode
    if args.suite == 'rack':
        rack_dir = args.rack_dir.resolve()
        if not (rack_dir / 'plugin.mk').is_file():
            raise RuntimeError(f'Rack SDK/tree missing at {rack_dir}; pass --rack-dir')
        # Force a fresh Rack instrumentation build for each report, without
        # touching normal plugin/test objects. Standalone builds use stamps.
        test_status = run(['make', '-B', '-k', f'-j{args.jobs}', 'test-rack', f'CXX={compiler}',
             f'RACK_DIR={rack_dir}', f'RACK_TEST_INSTRUMENT={args.mode}'], env, log, check=False)
        binaries = [build / 'rack' / p.stem
                    for p in sorted((ROOT / 'test/rack').glob('test_*.cpp'))]
    else:
        target = 'test-dsp' if args.suite == 'dsp' else 'test-mailbox'
        test_status = run(['make', '-k', f'-j{args.jobs}', f'CXX={compiler}',
             f'INSTRUMENT={args.mode}', target], env, log, check=False)
        test_dir = 'dsp' if args.suite == 'dsp' else 'threads'
        binaries = [build / 'standalone' / p.relative_to(ROOT / 'test').with_suffix('')
                    for p in sorted((ROOT / 'test' / test_dir).rglob('*.cpp'))]
    (report / 'test-status.json').write_text(json.dumps({
        'mode': args.mode, 'suite': args.suite, 'exit_code': test_status,
        'passed': test_status == 0,
    }, indent=2) + '\n')
    if args.mode == 'coverage':
        report_coverage(args.suite, binaries, report, env, log, test_status)
    if test_status:
        print(log.read_text(), file=sys.stderr)
        raise RuntimeError(f'test run failed ({test_status}); see {log}')
    print(f'{args.mode} {args.suite}: PASS; diagnostics: {log}')


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        sys.exit(str(error))
