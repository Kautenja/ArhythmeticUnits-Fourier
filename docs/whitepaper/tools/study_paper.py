#!/usr/bin/env python3
"""Derive publication assets from validated study evidence; never collect timings."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import csv
import hashlib
import io
import json
import math
from pathlib import Path
import statistics as stats

PAPER=Path(__file__).resolve().parents[1]
SOURCE=PAPER/'data/study-014'


def digest(path):
    with path.open('rb') as f: return hashlib.file_digest(f,'sha256').hexdigest()


def number(v):
    if v is None: return '--'
    if v == 0: return '0'
    if abs(v) >= 1000: return str(int(round(v,2-int(math.floor(math.log10(abs(v)))))))
    if abs(v) < .001:
        coefficient,exponent=f'{v:.2e}'.split('e')
        return '$'+coefficient+r'\times10^{'+str(int(exponent))+'}$'
    return f'{v:.3g}'


def table(headers,rows,caption,label,columns=None):
    return '\n'.join([r'\begin{table}[htbp]',r'\centering\small',
        r'\begin{tabular}{@{}'+(columns or 'l'+'r'*(len(headers)-1))+r'@{}}',r'\toprule',
        ' & '.join(headers)+r' \\',r'\midrule',
        *[' & '.join(map(str,r))+r' \\' for r in rows],r'\bottomrule',r'\end{tabular}',
        r'\caption{'+caption+'}',r'\label{'+label+'}',r'\end{table}',''])


def csv_text(rows):
    f=io.StringIO();w=csv.DictWriter(f,list(rows[0]),lineterminator='\n');w.writeheader();w.writerows(rows);return f.getvalue()


class Study:
    def __init__(self):
        receipt=json.loads((SOURCE/'receipt.json').read_text())
        for name,expected in receipt['files'].items():
            if digest(SOURCE/name)!=expected: raise ValueError('Changed evidence: '+name)
        if digest(PAPER/'tools/study_import.py')!=receipt['importer_sha256']:
            raise ValueError('Importer changed since evidence derivation')
        self.data=json.loads((SOURCE/'evidence.json').read_text())
        self.rows=self.data['processes']
        assert len(self.rows)==776
        self.sessions=[s['label'] for s in self.data['sessions']]
        assert self.sessions==['pilot-01','pilot-02']
        self.cache={}

    def cell(self,group,index):
        if (group,index) in self.cache: return self.cache[group,index]
        rr=[r for r in self.rows if r['group']==group and r['workload']==index]
        assert len(rr)==4 and {(r['session'],r['repeat'])for r in rr}=={(s,i)for s in self.sessions for i in (0,1)}
        get=lambda r,k: r['blocks'][k[6:]] if k.startswith('block_') else (
            (r['hop_peaks']or{}).get(k[4:]) if k.startswith('hop_') else r.get(k))
        keys=('compute_ns_per_sample','aggregate_cpu_ns_per_sample','block_p99','hop_median','hop_p95','hop_p99')
        session_values={s:{k:stats.mean(get(r,k)for r in rr if r['session']==s) if all(get(r,k)is not None for r in rr)else None for k in keys}for s in self.sessions}
        result={k:stats.mean(session_values[s][k]for s in self.sessions)if session_values[self.sessions[0]][k]is not None else None for k in keys}
        result.update(group=group,workload=index,config_id=rr[0]['config_id'],sessions=session_values,records=rr,
            maximum=max(r['blocks']['maximum']for r in rr),observations=sum(r['observations']for r in rr),
            compute_misses=sum(r['compute_budget_misses']for r in rr),release_misses=sum(r.get('release_misses',0)for r in rr))
        self.cache[group,index]=result;return result

    def peak_range(self,row):
        vals=[s['hop_median']/1000 for s in row['sessions'].values()]
        return number(row['hop_median']/1000)+' ['+number(min(vals))+'--'+number(max(vals))+']'

OUTPUT = PAPER / '.build/paper-study'


def generate():
    study = Study()
    out, macros = {}, {}
    def macro(name, value):
        macros[name] = number(value)
    names = ['Matched batch', 'Matched distributed', 'PFFFT batch', 'PFFFT hybrid',
             'vDSP batch', 'vDSP hybrid', 'FFTW batch', 'FFTW hybrid']
    for prefix,i in [('Core',4),('Batch',1),('PffftBatch',7),('PffftHybrid',10),('VdspBatch',13),('VdspHybrid',16)]:
        r=study.cell('Baselines',i);macro(prefix+'Cost',r['compute_ns_per_sample']);macro(prefix+'Peak',r['hop_median']/1000)
    macro('PlacementRatio',study.cell('Baselines',1)['hop_median']/study.cell('Baselines',4)['hop_median'])
    macro('PlacementCostRatio',study.cell('Baselines',4)['compute_ns_per_sample']/study.cell('Baselines',1)['compute_ns_per_sample'])
    signs=0;comparisons=0
    for family in range(4):
        for size in range(3):
            a=study.cell('Baselines',family*6+size);b=study.cell('Baselines',family*6+3+size)
            for session in study.sessions:
                comparisons+=1;signs+=b['sessions'][session]['hop_median']<a['sessions'][session]['hop_median']
    macros['PlacementSigns']=str(signs);macros['PlacementComparisons']=str(comparisons)
    macro('EmptyCpuCores',study.cell('Host',80)['aggregate_cpu_ns_per_sample']*48000/1e9)
    for prefix,i in [('HostCore',2),('HostBatch',15),('HostHybrid',28),('HostFftwBatch',41),('HostManyCore',4),('HostManyBatch',17),('HostStaggerBatch',18)]:
        r=study.cell('Host',i);macro(prefix+'Peak',r['hop_median']/1000);macro(prefix+'Cost',r['compute_ns_per_sample'])
        macro(prefix+'BudgetPercent',r['hop_median']/ (64*1e9/48000)*100)
        macros[prefix+'Misses']=str(r['release_misses'])
    for prefix,i in [('ModuleDefault',0),('ModuleExtreme',2),('ModuleBoth',7)]:
        r=study.cell('Modules',i);macro(prefix+'Peak',r['hop_median']/1000);macro(prefix+'BudgetPercent',r['hop_median']/(64*1e9/48000)*100)
        macros[prefix+'Misses']=str(r['release_misses']);macros[prefix+'ComputeMisses']=str(r['compute_misses'])
    for prefix,i in [('HorizonFull',21),('HorizonHalf',22),('HorizonQuarter',23),('SimdExtreme',31)]:
        r=study.cell('Granularity',i);macro(prefix+'Peak',r['hop_median']/1000);macro(prefix+'Cost',r['compute_ns_per_sample'])
    # Every process and cell remains accessible even when absent from a printed table.
    allcells=[];processes=[];sessions=[]
    for g,group in study.data['groups'].items():
        for i,c in enumerate(group['configs']):
            r=study.cell(g,i);w=c.get('workload',c)
            header=dict(group=g,workload=i,config_id=r['config_id'],backend=w['backend'],native=c.get('native',''),n=w['n'],hop=w['hop'],block=w['block'],regime=w['execution_regime'])
            allcells.append(dict(header,**{k:r[k]for k in ('compute_ns_per_sample','aggregate_cpu_ns_per_sample','hop_median','hop_p95','hop_p99','block_p99','maximum','compute_misses','release_misses','observations')}))
            for s,v in r['sessions'].items():sessions.append(dict(header,session=s,**v))
            for p in r['records']:
                processes.append(dict(header,session=p['session'],repeat=p['repeat'],raw=p['raw'],raw_sha256=p['raw_sha256'],compute_ns_per_sample=p['compute_ns_per_sample'],hop_median=(p['hop_peaks']or{}).get('median'),hop_count=(p['hop_peaks']or{}).get('count'),block_p99=p['blocks']['p99'],maximum=p['blocks']['maximum'],compute_misses=p['compute_budget_misses'],release_misses=p.get('release_misses'),observations=p['observations']))
    out['all-cells.csv']=csv_text(allcells);out['all-processes.csv']=csv_text(processes);out['all-sessions.csv']=csv_text(sessions)
    # Compact conference tables retain the same cells and aggregation as the
    # longer report. Only the displayed columns and captions differ.
    baseline_ids = [1, 4, 7, 10, 13, 16, 19, 22]
    out['conference-baselines.tex'] = table(
        ['Analysis path', 'Cost', 'Peak [sessions]'], [
            [name, number((row := study.cell('Baselines', index))['compute_ns_per_sample']),
             study.peak_range(row)]
            for name, index in zip(names, baseline_ids)],
        r'Scalar analysis, $N=4096$, $H=1024$, $D=64$, Hann, no smoothing. '
        r'Cost is instrumented ns/sample; peak is the typical hop peak in $\mu$s. '
        r'Brackets give the two session summaries. Each row contains four processes, '
        r'16,384 blocks, and 1,024 complete hops. No block exceeded the '
        r'1,333\,$\mu$s budget.', 'tab:conference-baselines')
    out['conference-horizons.tex'] = table(
        ['Finish after', 'Age (ms)', 'Cost', 'Peak'], [
            [label, number(age / 48),
             number((row := study.cell('Granularity', index))['compute_ns_per_sample']),
             number(row['hop_median'] / 1000)]
            for label, index, age in [('Batch', 36, 0), ('$H$', 21, 239),
                                     ('$H/2$', 22, 119), ('$H/4$', 23, 59)]],
        r'Four-channel vDSP analysis, $N=16384$, $H=240$, $D=64$, Hann. '
        r'Age is endpoint-to-publication delay; cost is ns/sample for all four '
        r'channels; peak is $\mu$s. Each row contains four processes and 3,840 '
        r'blocks. Only the completion interval changes between hybrid rows.',
        'tab:conference-horizons')
    out['conference-host.tex'] = table(
        ['Analysis', 'Cost', 'Peak [sessions]', 'Misses'], [
            [name, number((row := study.cell('Host', index))['compute_ns_per_sample']),
             study.peak_range(row), str(row['release_misses'])]
            for name, index in [('Distributed', 2), ('vDSP batch', 15), ('vDSP hybrid', 28)]],
        r'Actual Rack engine: one thread, four aligned four-channel Fourier '
        r'modules, $N=2048$, $H=1440$, $D=64$, flattop. Cost is block wall '
        r'ns/sample for the complete engine; peak is $\mu$s. Brackets give '
        r'session summaries. Misses are late synthetic releases across 16,384 '
        r'blocks per row, including wake lateness, not audio underruns.',
        'tab:conference-host')
    out['macros.tex']='% Generated by tools/study_paper.py; do not edit.\n'+''.join(r'\newcommand{\Study'+k+'}{'+v+'}\n'for k,v in sorted(macros.items()))
    out['provenance.json']=json.dumps(dict(schema=1,evidence_sha256=digest(SOURCE/'evidence.json'),receipt_sha256=digest(SOURCE/'receipt.json'),generator_sha256=digest(Path(__file__)),aggregation='two process means/medians within session; equal session weight; descriptive only',selections='all cells in CSV; printed selections in generator; no discarded observations',outputs={k:hashlib.sha256(v.encode()).hexdigest()for k,v in out.items()}),sort_keys=True,indent=2)+'\n'
    return out

def write_assets(output, files):
    """Write explicitly requested derived output, never measured evidence."""
    output.mkdir(parents=True, exist_ok=True)
    for name, content in files.items():
        (output / name).write_text(content)


def main():
    argparse.ArgumentParser(description=__doc__).parse_args()
    files = generate()
    write_assets(OUTPUT, files)
    print(f'Wrote {len(files)} study assets to {OUTPUT}')


if __name__ == '__main__':
    main()
