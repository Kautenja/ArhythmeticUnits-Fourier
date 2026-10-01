#!/usr/bin/env python3
"""Generate the paper from validated spec-014 evidence, without running benchmarks."""
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
OUTPUT=PAPER/'generated/paper-study'


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


def generate():
    study=Study();out={};macros={}
    def macro(name,value):macros[name]=number(value)
    counts=[]
    for g in ['Baselines','Granularity','Scaling','Modules','Stress','Host','Lifecycle']:
        group=study.data['groups'][g];configs=group['configs'];c=[p.get('workload',p)for p in configs]
        regimes=sorted({x['execution_regime']for x in c})
        obs=sorted({p.get('blocks',p.get('callbacks'))for p in configs})
        counts.append([g,str(len(configs)),', '.join(regimes),str(min(obs)) if len(obs)==1 else f'{min(obs)}--{max(obs)}'])
    out['design.tex']=table(['Group','Cells','Execution','Blocks/process'],counts,
        'Predeclared pilot matrix: two sessions and two fresh processes per cell per session. '
        'Stream groups use 256 nominal hops, except Stress (128); host uses 262,144 engine samples. '
        'Lifecycle includes separate 65,536-sample event sequences and 1,048,576-sample decays. '
        'Each process has 16 warmup hops; host warmup rounds up to whole blocks. '
        'Three repeated bridge cells retain their group identities.', 'tab:study-design', 'lrll')
    names=['Matched batch','Matched distributed','PFFFT batch','PFFFT hybrid','vDSP batch','vDSP hybrid','FFTW batch','FFTW hybrid']
    rows=[]
    for name,i in zip(names,[1,4,7,10,13,16,19,22]):
        r=study.cell('Baselines',i)
        rows.append([name,number(r['compute_ns_per_sample']),study.peak_range(r),number(r['block_p99']/1000),number(r['maximum']/1000)])
    out['baselines.tex']=table(['Analysis path','Cost','Hop peak [sessions]','p99','Max'],rows,
        r'Scalar analysis, $N=4096,H=1024,D=64$, Hann, continuous execution. Cost is instrumented wall ns/engine sample; other values are $\mu$s. '
        r'Hop peak is the process median of complete-hop maxima, summarized equally over sessions. Brackets show the two session summaries, not confidence limits. '
        r'Each row contains four processes, 16,384 blocks and 1,024 complete hops. No block exceeded the 1,333\,$\mu$s compute budget.', 'tab:study-baselines')
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
    host_rows=[]
    for shape,indices in [('1 thread, 4 aligned',[2,15,28,41,54]),('4 threads, 16 aligned',[4,17,30,43,56]),('4 threads, 16 staggered',[5,18,31,44,57])]:
        for name,i in zip(['Production','vDSP batch','vDSP hybrid','FFTW batch','FFTW hybrid'],indices):
            r=study.cell('Host',i)
            host_rows.append([shape,name,number(r['compute_ns_per_sample']),study.peak_range(r),str(r['release_misses']),number(r['maximum']/1000)])
    out['host.tex']=table(['Engine graph','Analysis','Cost','Hop peak [sessions]','Misses','Max'],host_rows,
        r'Actual paced Rack engine, Fourier defaults ($N=2048,H=1440,D=64$), no background or consumer. '
        r'Cost is block wall ns/engine sample; peak and maximum are $\mu$s. '
        r'Misses are synthetic release-deadline exceedances across 16,384 blocks per row; wake lateness is included. '
        r'Engine overhead remains included. These counts are not independent Bernoulli trials or audio underruns.', 'tab:study-host','llrrrr')
    accounting=[]
    for name,i in [('Empty, 1 thread',77),('Empty, 4 threads',80),('Production, 1 thread',2),
                   ('Production, 4 threads',3),('vDSP batch, 1 thread',15),('vDSP batch, 4 threads',16)]:
        r=study.cell('Host',i)
        accounting.append([name,number(r['aggregate_cpu_ns_per_sample']/1000),number(r['block_p99']/1000),
            number(stats.mean(p['wake_lateness']['p99']for p in r['records'])/1000),
            f"{r['compute_misses']} / {r['release_misses']}"])
    out['host-accounting.tex']=table(['Graph','CPU/sample','Block p99','Wake p99','Compute / release'],accounting,
        r'Paced engine controls at $D=64$, 48\,kHz. Nonempty graphs contain four aligned Fourier modules; no background or consumer. '
        r'All time columns are $\mu$s. CPU/sample includes process-wide CPU during waits and worker synchronization; '
        r'block p99 excludes wake lateness. Counts span 16,384 blocks per row. The empty engine is a measured control, not a subtracted overhead estimate.',
        'tab:study-host-accounting')
    macro('EmptyCpuCores',study.cell('Host',80)['aggregate_cpu_ns_per_sample']*48000/1e9)
    for prefix,i in [('HostCore',2),('HostBatch',15),('HostHybrid',28),('HostFftwBatch',41),('HostManyCore',4),('HostManyBatch',17),('HostStaggerBatch',18)]:
        r=study.cell('Host',i);macro(prefix+'Peak',r['hop_median']/1000);macro(prefix+'Cost',r['compute_ns_per_sample'])
        macro(prefix+'BudgetPercent',r['hop_median']/ (64*1e9/48000)*100)
        macros[prefix+'Misses']=str(r['release_misses'])
    module_names=['Fourier defaults','Spectre defaults','Fourier extreme','Extreme, 1 port','Extreme, 4 voices/port','Extreme, octave mean','Extreme, time mean','Extreme, both means','Spectre, both means']
    out['modules.tex']=table(['Module configuration','Cost','Hop peak','p99','Compute / release'],[
        [name,number((r:=study.cell('Modules',i))['compute_ns_per_sample']),number(r['hop_median']/1000),number(r['block_p99']/1000),f"{r['compute_misses']} / {r['release_misses']}"]for i,name in enumerate(module_names)],
        r'Paced direct module calls, $D=64$, 48\,kHz. Cost is ns/engine sample; peak and p99 are $\mu$s. '
        r'Extreme means $N=16384,H=240$, Hann, four active ports and one voice/port unless overridden. '
        r'Octave/time means use $1/3$ octave and 0.1\,s. Defaults use their flattop window. '
        r'Denominators are 23,040 blocks for Fourier defaults, 16,384 for each Spectre row, and 3,840 per extreme row. '
        r'These calls include module processing but not Rack worker barriers; they are not matched native extreme comparisons.', 'tab:study-modules')
    for prefix,i in [('ModuleDefault',0),('ModuleExtreme',2),('ModuleBoth',7)]:
        r=study.cell('Modules',i);macro(prefix+'Peak',r['hop_median']/1000);macro(prefix+'BudgetPercent',r['hop_median']/(64*1e9/48000)*100)
        macros[prefix+'Misses']=str(r['release_misses']);macros[prefix+'ComputeMisses']=str(r['compute_misses'])
    horizon_rows=[]
    for provider,base,batch in [('vDSP four-channel',21,36),('FFTW four-channel',27,38)]:
        for label,i,age in [('batch',batch,0),('$H$',base,239),('$H/2$',base+1,119),('$H/4$',base+2,59)]:
            r=study.cell('Granularity',i)
            horizon_rows.append([provider,label,number(age/48),number(r['compute_ns_per_sample']),study.peak_range(r)])
    out['horizons.tex']=table(['Native pipeline','Horizon','Age (ms)','Cost','Hop peak [sessions]'],horizon_rows,
        r'Four-channel controlled analysis at $N=16384,H=240,D=64$, continuous execution. '
        r'Age is logical endpoint-to-publication delay; cost is ns/engine sample for all four channels, peak is $\mu$s. '
        r'Both providers use batched native four-channel plans. Each row has 3,840 blocks across four processes. '
        r'The same-shape SIMD production core has the values stated in the text.', 'tab:study-horizons')
    for prefix,i in [('HorizonFull',21),('HorizonHalf',22),('HorizonQuarter',23),('SimdExtreme',31)]:
        r=study.cell('Granularity',i);macro(prefix+'Peak',r['hop_median']/1000);macro(prefix+'Cost',r['compute_ns_per_sample'])
    consumer_rows=[]
    for name,ids in [('Production',[10,11,12]),('vDSP batch',[23,24,25]),('vDSP hybrid',[36,37,38])]:
        for mode,i in zip(['30 Hz','60 Hz','60 Hz + stalls'],ids):
            rr=study.cell('Host',i)['records']
            lo=stats.mean(r['consumed_age_lower']['median']for r in rr)/48
            hi=stats.mean(r['consumed_age_upper']['median']for r in rr)/48
            consumer_rows.append([name,mode,number(lo)+'--'+number(hi),str(sum(r['skipped_publications']for r in rr))])
    out['consumer.tex']=table(['Analysis','Consumer','Median age bounds (ms)','Unconsumed'],consumer_rows,
        r'Fourier defaults, four threads and four staggered modules, $D=64$. '
        r'Age is relative to the newest input sample and uses the logical engine clock. '
        r'Stalls skip two polls every tenth poll. Unconsumed counts retain publications not observed even by the final drain; '
        r'age summaries exclude that drain. The consumer only copies/checksums data; no repaint or perceptual threshold is measured.', 'tab:study-consumer','llrr')
    audit_rows=[]
    for label,predicate in [('Stream float',lambda r:r['family']=='stream' and '-double' not in study.data['groups'][r['group']]['configs'][r['workload']]['backend']),('Stream double',lambda r:r['family']=='stream' and '-double' in study.data['groups'][r['group']]['configs'][r['workload']]['backend']),('Rack ordinary',lambda r:r['family']=='engine' and not any(a['policy']=='module-decay-ftz-v1'for a in r['audits'])),('Rack decay',lambda r:any(a['policy']=='module-decay-ftz-v1'for a in r['audits']))]:
        aa=[a for r in study.rows if predicate(r)for a in r['audits']]
        audit_rows.append([label,str(sum(a['vectors']for a in aa)),number(max(a['max_relative_l2']for a in aa)),number(max(a['max_relative_linf']for a in aa)),str(sum(a.get('tail',{}).get('vectors',0)for a in aa))])
    out['numerical.tex']=table(['Audit scope','Vectors',r'Max rel. $\ell_2$',r'Max rel. $\ell_\infty$','Flagged tail'],audit_rows,
        r'All retained replay vectors, including engine pre-roll/warmup and repeat audits. '
        r'Counts indicate coverage, not independent inputs. Decay relative maxima exclude the flagged FTZ/display tail, '
        r'which remains subject to its absolute floor. Float/double budgets are $3\times10^{-4}$/$10^{-10}$.', 'tab:study-numerical')
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
    # Process-level tradeoff figure: every retained point at the declared reference shape.
    colors=['black','ink','accent','green!50!black']
    plot=[r'\begin{figure}[htbp]',r'\centering',r'\begin{tikzpicture}',r'\begin{axis}[width=.94\linewidth,height=6cm,xlabel={Instrumented wall cost (ns/engine sample)},ylabel={Median hop peak ($\mu$s)},legend style={font=\scriptsize,at={(.5,-.24)},anchor=north,legend columns=2,column sep=8pt},grid=major]']
    for k,(a,b) in enumerate([(1,4),(7,10),(13,16),(19,22)]):
        for j,i in enumerate((a,b)):
            rr=study.cell('Baselines',i)['records'];coords=' '.join(f"({r['compute_ns_per_sample']:.8g},{r['hop_peaks']['median']/1000:.8g})"for r in rr)
            plot += [r'\addplot[only marks,color='+colors[k]+',mark='+('square*'if j==0 else 'triangle*')+'] coordinates {'+coords+'};',r'\addlegendentry{'+names[k*2+j]+'}']
    plot += [r'\end{axis}',r'\end{tikzpicture}',r'\caption{Placement and kernel choice at the scalar reference configuration in Table~\ref{tab:study-baselines}. Each point is one fresh process; all four per implementation are shown. Squares denote immediate batch and triangles distributed/hybrid execution. Points are not confidence regions.}',r'\label{fig:study-tradeoff}',r'\end{figure}','']
    out['tradeoff.tex']='\n'.join(plot)
    plot=[r'\begin{figure}[htbp]',r'\centering',r'\begin{tikzpicture}',r'\begin{axis}[width=.92\linewidth,height=5.4cm,xlabel={Engine threads},ylabel={Median hop peak ($\mu$s)},xtick={1,4},xmin=.8,xmax=4.2,legend style={font=\scriptsize,at={(.5,-.40)},anchor=north,legend columns=3,column sep=8pt},grid=major]']
    for name,ids,col,mark in [('Production',[2,3],'ink','*'),('vDSP batch',[15,16],'accent','square*'),('vDSP hybrid',[28,29],'green!50!black','triangle*')]:
        coords=' '.join(f"({t},{study.cell('Host',i)['hop_median']/1000:.8g})"for t,i in zip([1,4],ids))
        plot += [r'\addplot[color='+col+',mark='+mark+'] coordinates {'+coords+'};',r'\addlegendentry{'+name+'}']
        # Show both independent session summaries without adding legend entries.
        coords=' '.join(f"({t},{s['hop_median']/1000:.8g})"for t,i in zip([1,4],ids)for s in study.cell('Host',i)['sessions'].values())
        plot += [r'\addplot[only marks,mark=o,color='+col+',forget plot] coordinates {'+coords+'};']
    plot += [r'\end{axis}',r'\end{tikzpicture}',r'\caption{Full paced Rack blocks with four aligned Fourier modules and no background or consumer. Filled points summarize the two sessions; open points show each session. Lines connect only the tested one- and four-thread cases. Convergence under four threads is consistent with shared host overhead; this experiment does not isolate barrier cost or core placement.}',r'\label{fig:study-host}',r'\end{figure}','']
    out['host-figure.tex']='\n'.join(plot)
    out['macros.tex']='% Generated by tools/study_paper.py; do not edit.\n'+''.join(r'\newcommand{\Study'+k+'}{'+v+'}\n'for k,v in sorted(macros.items()))
    out['provenance.json']=json.dumps(dict(schema=1,evidence_sha256=digest(SOURCE/'evidence.json'),receipt_sha256=digest(SOURCE/'receipt.json'),generator_sha256=digest(Path(__file__)),aggregation='two process means/medians within session; equal session weight; descriptive only',selections='all cells in CSV; printed selections in generator; no discarded observations',outputs={k:hashlib.sha256(v.encode()).hexdigest()for k,v in out.items()}),sort_keys=True,indent=2)+'\n'
    return out


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    files=generate()
    if args.check:
        assert {p.name for p in OUTPUT.iterdir()if p.is_file()}==set(files),'Unexpected generated assets'
        for name,content in files.items():
            if not (OUTPUT/name).exists()or (OUTPUT/name).read_text()!=content:raise ValueError('Stale publication asset: '+name)
        print('Verified 776 processes, 194 cells, 388 session summaries and all publication assets.')
    else:
        OUTPUT.mkdir(parents=True,exist_ok=True)
        for name,content in files.items():(OUTPUT/name).write_text(content)
        print('Generated spec-014 tables, full-grid CSVs, macros and vector figures.')


if __name__=='__main__':main()
