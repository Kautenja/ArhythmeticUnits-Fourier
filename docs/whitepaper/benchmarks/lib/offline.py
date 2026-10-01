"""Prepare and dispatch spec-014 through the maintained serial campaign runner."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import copy
import datetime as dt
import fcntl
import json
import io
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tarfile
from types import SimpleNamespace
from contextlib import contextmanager
from paths import ROOT, BENCHMARKS
import run
from contracts import load_registry, resolve_contract
from dependencies import fftw_inputs
from execution import SleepProtection, Gate, POLICY, ENVIRONMENT, host_snapshot, validate_campaign
from prepared import seal, validate, snapshot, sdk_inputs, assert_unchanged
from study_plan import resolve, summary, identity

DEFAULT = ROOT/'.build/study-014/prepared'
OUTPUT = Path.home()/'Fourier-benchmarks/spec014'
RESTRICTED = {'paper.bin', 'paper-audit.bin', 'dependencies.tar.gz', 'external-dependencies.tar.gz'}
CHECKLIST = ('Connect AC; disable Low Power Mode; keep the lid open. Disconnect Wi-Fi, Ethernet and other '
             'network links; turn Bluetooth off. Stop agents and close unnecessary applications. Use the built-in '
             'keyboard/trackpad or wired controls. Launch in a standalone terminal with Codex closed; leave the '
             'machine alone. These are your declarations, not proof that OS activity has ceased.')


class UntimedProgress:
    def switch(self, *args, **kwargs): pass


def environment(rack):
    env = dict(os.environ, DYLD_LIBRARY_PATH=str(rack), LD_LIBRARY_PATH=str(rack))
    for name in (*ENVIRONMENT, 'PAPER_RUNTIME_PATH', 'PAPER_TRANSITION_PATH', 'PAPER_HOST_EXECUTION_PATH',
                 'PAPER_DIAGNOSTIC_EXECUTION_PATH', 'PAPER_GUARD_TOKEN', 'PAPER_SLEEP_OWNER'):
        env.pop(name, None)
    return env


def prepare(directory, rack=ROOT/'../..', fftw=ROOT/'.build/deps/fftw', cxx='c++'):
    """Only builds, allocation probes, inventory and untimed correctness replays."""
    directory, rack, fftw = Path(directory).resolve(), Path(rack).resolve(), Path(fftw).resolve()
    if not (rack/'include/rack.hpp').is_file() or not (rack/'libRack.dylib').is_file():
        raise ValueError('Local macOS Rack source/library required; configure RACK_DIR before preparation')
    if run.capture(['git', 'rev-parse', 'HEAD'], rack) != __import__('engine_host').RACK_REVISION:
        raise ValueError('Rack revision differs from the reviewed engine pin')
    external = fftw_inputs(fftw)  # Actionable missing-input error; never fetch here or at launch.
    plan = resolve(); registry = load_registry(features=plan['design']['required_features'])
    directory.mkdir(parents=True, exist_ok=False)
    all_inputs = snapshot(run.source_inputs()+sdk_inputs(rack)+list(external.values()))
    configs = list({identity(c):c for g in plan['groups'] if g['family'] == 'stream' for c in g['configs']}.values())
    base = directory/'base'; base.mkdir()
    args = SimpleNamespace(rack_dir=rack, fftw_prefix=fftw, enable_vdsp=True, cxx=cxx, freeze=None,
        seed=plan['design']['sessions']['pilot-01'], repeats=2, notes='Preparation only; no measured evidence',
        session_id='prepared', host_id=__import__('platform').node(), config=None, execution_policy=dict(POLICY),
        sleep_guard=SimpleNamespace(pid=0), prepare_only=True, engine_study=True)
    metadata = {}
    try:
        run.run_campaign(args, base, configs, plan['design']['required_features'], external, registry,
                         'pilot', None, UntimedProgress(), metadata)
        from check import validate_resources
        for filename in metadata['resources'].values():
            resource = json.loads((base/filename).read_text()); validate_resources(resource)
            if any(item['timing_policy'] != 'untimed-v1' for item in resource.values()):
                raise ValueError('Preparation unexpectedly collected resource timings')
        engine = directory/'engine'; engine.mkdir()
        env = environment(rack)
        for group in plan['groups']:
            if group['family'] != 'engine': continue
            for i, profile in enumerate(group['configs']):
                stem = f"{group['name']}-{i:04d}"
                source = engine/(stem+'.json'); run.save(source, profile)
                print('Untimed replay:', stem, flush=True)
                with (engine/(stem+'.verification.json')).open('x') as out, (engine/(stem+'.stderr')).open('x') as err:
                    subprocess.run([str(base/'paper.bin'), '--engine-verify', str(source)], env=env,
                                   stdout=out, stderr=err, check=True)
                with (engine/(stem+'.resources.json')).open('x') as out, (engine/(stem+'.resources.stderr')).open('x') as err:
                    subprocess.run([str(base/'paper-audit.bin'), '--engine-resources', str(source)], env=env,
                                   stdout=out, stderr=err, check=True)
        assert_unchanged(all_inputs)
        run.save(directory/'plan-summary.json', summary(plan))
        (directory/'README.md').write_text('# Prepared Offline Pilot Package\n\nNo performance measurements are present.\n'
            'Run each declared session separately with benchmark-study-run after the quiet-host checklist.\n'
            'A changed source, dependency, host, profile or artifact requires new preparation.\n')
        manifest = seal(directory, plan, all_inputs, ROOT, rack_dir=str(rack), fftw_prefix=str(fftw),
                        external_inputs={k:str(v) for k,v in external.items()}, cxx=cxx)
        validate(directory)
        print(json.dumps(dict(prepared=str(directory), manifest_id=manifest['manifest_id'], **summary(plan)), indent=2))
        return manifest
    except BaseException as error:
        run.save(directory/'failure.json', dict(status='failed', kind='preparation-only', error=str(error)))
        raise


@contextmanager
def serial_lock(file=None):
    file = Path(file or f'/tmp/fourier-study-{os.getuid()}.lock')
    with file.open('a+') as stream:
        try: fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error: raise ValueError('Another Fourier study session is active') from error
        try: yield
        finally: fcntl.flock(stream, fcntl.LOCK_UN)


def stage_groups(package, output, manifest, session, guard):
    """Stage every byte before the shared stabilization gate, never between groups."""
    base = json.loads((package/'base/metadata.json').read_text()); registry = load_registry(features=base['build_features'])
    result = []
    indices = {identity(c):i for i,c in enumerate(base['configs'])}
    for group in manifest['plan']['groups']:
        destination = output/group['name']; shutil.copytree(package/'base', destination)
        m = copy.deepcopy(base)
        m.update(status='incomplete', family=group['family'], configs=group['configs'], runs=[],
                 seed=manifest['plan']['design']['sessions'][session], repeats=group['repeats'],
                 session_id=session, host_id=manifest['host']['host'], execution_policy=group['execution_policy'],
                 sleep_protection=guard.record, prepared_manifest_id=manifest['manifest_id'],
                 started_utc=dt.datetime.now(dt.timezone.utc).isoformat(), notes='User-declared quiet host; see session.json')
        m.pop('artifact_sha256', None)
        if group['family'] == 'stream':
            m['contracts'] = {str(i):resolve_contract(c, registry) for i,c in enumerate(group['configs'])}
            m['resources'] = {str(i):base['resources'][str(indices[identity(c)])] for i,c in enumerate(group['configs'])}
            m['matrix_inventory'] = run.campaign_inventory(group['configs'], registry)
        else:
            m.pop('matrix_inventory', None); m['contracts'] = {}; m['resources'] = {}
            for i,p in enumerate(group['configs']):
                run.save(destination/f'profile-{i:04d}.json', p)
                stem = f"{group['name']}-{i:04d}"
                for suffix in ('verification.json', 'resources.json', 'stderr'):
                    shutil.copy2(package/'engine'/(stem+'.'+suffix), destination/(f'preflight-{i:04d}.'+suffix))
        run.save(destination/'metadata.json', m)
        result.append((group, destination, m))
    return result, registry


def package_handback(output, session):
    """Preserve failed attempts too; no charts, favorable selection or SDK redistribution."""
    (output/'README.md').write_text('# Fourier Spec 014 Handback\n\n'
        +f"Session: {session['session']}. Status: {session['status']}.\n\n"
        +'All attempted raw observations, logs, numerical audits and source identities are retained. '
        'Failed/interrupted sessions are incomplete evidence. No automatic retry or confirmation promotion occurred.\n\n'
        +'The handback omits executable and SDK/provider archives; their checksums remain in metadata. '
        'Keep the full local directory. Return this archive and its SHA256 file after reopening agents.\n')
    files = {str(p.relative_to(output)):run.digest(p) for p in output.rglob('*') if p.is_file() and p.name != 'checksums.json'}
    run.save(output/'checksums.json', files)
    archive = output.with_name(output.name+'.handback.tar.gz')
    with tarfile.open(archive, 'x:gz') as tar:
        for p in sorted(output.rglob('*')):
            if p.is_file() and p.name not in RESTRICTED: tar.add(p, arcname=str(p.relative_to(output)), recursive=False)
        for directory in sorted(p for p in output.iterdir() if p.is_dir() and (p/'metadata.json').is_file()):
            omitted = sorted(name for name in RESTRICTED if (directory/name).is_file())
            content = json.dumps(dict(schema=1, metadata_sha256=run.digest(directory/'metadata.json'), files=omitted)).encode()
            member = tarfile.TarInfo(directory.name+'/bundle-omissions.json'); member.size = len(content)
            tar.addfile(member, io.BytesIO(content))
    checksum = run.digest(archive)
    with archive.with_suffix(archive.suffix+'.sha256').open('x') as stream: stream.write(checksum+'  '+archive.name+'\n')
    return archive


def check_engine_group(directory, metadata, registry):
    from engine_host import validate as validate_engine
    if metadata['status'] != 'complete': raise ValueError('Incomplete engine group')
    expected = {(r,i) for r in range(metadata['repeats']) for i in range(len(metadata['configs']))}
    observed = [(r['repeat'],r['workload']) for r in metadata['runs']]
    if len(set(observed)) != len(observed) or set(observed) != expected: raise ValueError('Missing/duplicate engine process')
    proxy = dict(metadata, configs=[p['workload'] for p in metadata['configs']])
    validate_campaign(proxy)
    for name, sha in metadata['artifact_sha256'].items():
        if Path(name).name != name or run.digest(directory/name) != sha: raise ValueError('Changed engine artifact')
    for record in metadata['runs']:
        document = json.loads((directory/record['raw']).read_text())
        if document['profile'] != metadata['configs'][record['workload']]: raise ValueError('Engine profile changed')
        if validate_engine(document, registry) != record['summary']: raise ValueError('Engine summary changed')
        run.validate_engine_execution(json.loads((directory/record['execution']).read_text()),
                                     metadata['execution_policy'], document, metadata['sleep_protection'].get('pid', 0))


def launch(package, session_label, output_root=OUTPUT, quiet_declared=False):
    if not quiet_declared: raise ValueError('Record the quiet-host declaration before launching')
    package, output_root = Path(package).resolve(), Path(output_root).expanduser().resolve()
    output = output_root/session_label
    if output == ROOT or ROOT in output.parents:
        raise ValueError('Keep session results outside the repository and its clean/build directories')
    # Resolve labels before filesystem writes, including before interpreting them as paths.
    preliminary = json.loads((package/'manifest.json').read_text()) if (package/'manifest.json').is_file() else {}
    if session_label not in preliminary.get('plan', {}).get('design', {}).get('sessions', {}):
        raise ValueError('Missing preparation or undeclared session label; run benchmark-study-prepare')
    if output.exists() or output.with_name(output.name+'.handback.tar.gz').exists():
        raise ValueError('Session already exists; preserve it. No overwrite, retry or relabeling is automatic')
    session = dict(schema=1, kind='fourier-offline-session-v1', status='incomplete', session=session_label,
                   declaration=dict(quiet_host=True, checklist=CHECKLIST, utc=dt.datetime.now(dt.timezone.utc).isoformat()), groups=[])
    groups = []; output_created = False
    with serial_lock():
        try:
            with SleepProtection() as guard:
                session['sleep_protection'] = guard.record
                m = validate(package, ROOT)
                output.mkdir(parents=True, exist_ok=False); output_created = True
                print('Result directory:', output, flush=True)
                session.update(fixture=bool(m.get('fixture')), manifest_id=m['manifest_id'], plan_id=m['plan']['plan_id'], seed=m['plan']['design']['sessions'][session_label])
                run.save(output/'prepared-manifest.json', m)
                run.save(output/'session.json', session)
                groups, registry = stage_groups(package, output, m, session_label, guard)
                session['host_before'] = host_snapshot(guard.pid)
                gate = Gate(POLICY, guard); gate.prepared(package/'base/paper.bin', m['artifacts']['base/paper.bin'])
                session['execution_events'] = gate.events
                print('Preparation checked. Settling for 180 seconds; then all groups run serially.', flush=True)
                gate.settle(); gate.launch()
                env = environment(Path(m['rack_dir'])); env['PAPER_SLEEP_OWNER'] = str(guard.pid)
                for group, directory, metadata in groups:
                    session['active_group'] = group['name']; run.save(output/'session.json', session)
                    metadata['host_before'] = session['host_before']
                    args = SimpleNamespace(rack_dir=Path(m['rack_dir']), fftw_prefix=Path(m['fftw_prefix']),
                        external_inputs={k:Path(v) for k,v in m['external_inputs'].items()}, sleep_guard=guard,
                        execution_policy=group['execution_policy'], repeats=group['repeats'], seed=session['seed'])
                    print('Group:', group['name'], flush=True)
                    run.execute_campaign(args, directory, group['configs'], registry, UntimedProgress(), metadata,
                                         env, gate, defer_integrity=True)
                    session['groups'].append(group['name'])
                session['host_after'] = host_snapshot(guard.pid)
                if session['host_before']['power'] != session['host_after']['power']: raise ValueError('Power state changed during session')
                validate(package, ROOT)
            # Full integrity and packaging occur after timing and after caffeinate lifetime is finalized.
            from check import check
            for group, directory, metadata in groups:
                metadata.update(status='complete', host_after=session['host_after'], finished_utc=dt.datetime.now(dt.timezone.utc).isoformat())
                metadata['artifact_sha256'] = {p.name:run.digest(p) for p in directory.iterdir() if p.is_file() and p.name != 'metadata.json'}
                run.save(directory/'metadata.json', metadata)
                if group['family'] == 'stream': check(directory)
                else: check_engine_group(directory, metadata, registry)
            session.pop('active_group', None); session['status'] = 'complete'
        except BaseException as error:
            session.update(status='interrupted' if isinstance(error, KeyboardInterrupt) else 'failed', error=str(error), error_type=type(error).__name__)
            for _, directory, metadata in groups:
                if metadata['status'] != 'complete':
                    metadata.update(status='invalid', failure=session['error']); run.save(directory/'metadata.json', metadata)
            raise
        finally:
            if output_created:
                session['finished_utc'] = dt.datetime.now(dt.timezone.utc).isoformat(); run.save(output/'session.json', session)
                (output/session['status'].upper()).write_text(session['status']+'\n')
                archive = package_handback(output, session)
                print('Session status:', session['status'], '\nResults:', output, '\nReturn:', archive, flush=True)
    return output


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('prepare','run','check'))
    parser.add_argument('--package', type=Path, default=DEFAULT)
    parser.add_argument('--rack-dir', type=Path, default=ROOT/'../..')
    parser.add_argument('--fftw-prefix', type=Path, default=ROOT/'.build/deps/fftw')
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--session', default='')
    parser.add_argument('--output-root', type=Path, default=OUTPUT)
    parser.add_argument('--quiet-host-confirmed', action='store_true')
    args = parser.parse_args(argv)
    if args.action == 'prepare': return prepare(args.package, args.rack_dir, args.fftw_prefix, args.cxx)
    if args.action == 'check':
        for pattern in ('test_offline.py', 'test_execution.py', 'test_study_policies.py'):
            subprocess.run([sys.executable, '-m', 'unittest', 'discover', '-s', str(BENCHMARKS/'tests'), '-p', pattern], check=True)
        m = validate(args.package, ROOT); print('Prepared manifest verified:', m['manifest_id']); return m
    print(CHECKLIST, flush=True)
    if not args.quiet_host_confirmed:
        args.quiet_host_confirmed = input('Type READY to record this declaration and launch the session: ').strip() == 'READY'
    def interrupted(signum, frame): raise KeyboardInterrupt('Termination requested')
    previous = signal.signal(signal.SIGTERM, interrupted)
    try: return launch(args.package, args.session, args.output_root, args.quiet_host_confirmed)
    finally: signal.signal(signal.SIGTERM, previous)
