#!/usr/bin/env python3
"""Render every reference fixture with the actual freestanding float backend."""
import importlib.util,json,math,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];ENGINE=ROOT/'synthdef-engine';OUT=ROOT/'fm1-synth/build'
spec=importlib.util.spec_from_file_location('reference',ENGINE/'tests/compare_reference.py');ref=importlib.util.module_from_spec(spec);spec.loader.exec_module(ref)
host='--host' in sys.argv
exe=OUT/('host-fast-render' if host else 'device-math-render')
limits={r['name']:r for r in json.loads((ENGINE/'build/reference-results.json').read_text())}
subprocess.run(['cc','-O2','-std=c99','-fno-fast-math','-ffp-contract=off','-DPLAT_HOST',*([] if host else ['-I'+str(ROOT/'fm1-synth/firmware/device_include')]),'-I'+str(ROOT/'fm1-base/include'),'-I'+str(ENGINE/'src'),str(ENGINE/'src/fm_load.c'),str(ENGINE/'src/fm_render.c'),str(ENGINE/'tools/render.c'),*(['-lm'] if host else []),'-o',str(exe)],check=True)
results=[]
if set(limits)!={p.stem for p in (ENGINE/'fixtures').glob('*.scsyndef')}:
 raise RuntimeError('Run the complete engine reference suite before backend comparisons.')
for p in sorted((ENGINE/'fixtures').glob('*.scsyndef')):
 name=p.stem;blocks=ref.fixture_blocks(name);dest=OUT/('host-fast.f32' if host else 'device-math.f32')
 events=ref.EVENTS.get(name,[(0.25,0)] if name.startswith('env_') else [])
 if p.with_suffix('.events.json').exists():events=json.loads(p.with_suffix('.events.json').read_text())
 args=[str(exe),str(p),str(dest),str(blocks)]
 for t,v in events:args += [str(int(t*44100)//64),str(v)]
 run=subprocess.run(args,capture_output=True,text=True)
 if run.returncode:results.append(dict(name=name,passed=False,error=run.stderr));continue
 a=ref.samples(dest);b=ref.samples(ENGINE/'build'/f'{name}-reference.wav')[:len(a)]
 peak=max(abs(x-y) for x,y in zip(a,b));rms=math.sqrt(sum((x-y)**2 for x,y in zip(a,b))/len(a))
 # Explicit target-backend tolerance, separate from the stricter desktop oracle.
 peak_limit=limits[name]['peak_limit'] if host else 0.001
 rms_limit=limits[name]['rms_limit'] if host else 0.0001
 timing=ref.impulse_timing(a,b) if name=='m6_Impulse_ar_fm' else {}
 stochastic=ref.gendy_comparison(name,a,b)
 passed=all(math.isfinite(x) for x in a) and len(a)==len(b) and stochastic.get('stochastic_passed',timing.get('timing_passed',peak<peak_limit and rms<rms_limit))
 results.append(dict(**stochastic,**timing,name=name,peak=peak,rms=rms,passed=passed))
(OUT/('host-fast-results.json' if host else 'device-math-results.json')).write_text(json.dumps(results,indent=2)+'\n')
for r in results:
 if not r['passed']:print('FAIL',r)
print(sum(r['passed'] for r in results),'/',len(results),'device-backend reference comparisons passed')
raise SystemExit(0 if all(r['passed'] for r in results) else 1)
