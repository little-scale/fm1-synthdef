#!/usr/bin/env python3
"""Compare independent model arenas against pinned upstream mono Rings."""
from pathlib import Path
import subprocess,sys,json,hashlib
ROOT=Path(__file__).resolve().parents[1];TOP=ROOT.parent;device='--device' in sys.argv
up=ROOT/'reference/mi-rings';manifest=json.loads((up/'sources.json').read_text())
for rel,sha in manifest['sha256'].items():assert hashlib.sha256((up/rel).read_bytes()).hexdigest()==sha,rel
src=up/'eurorack';suffix='device' if device else 'host';obj=ROOT/'build'/f'rings-probe-{suffix}.o';exe=ROOT/'build'/f'rings-oracle-{suffix}'
subprocess.run(['cc','-std=c99','-O2','-fno-fast-math','-ffp-contract=off','-DPLAT_HOST',*(['-I'+str(TOP/'fm1-synth/firmware/device_include'),'-I'+str(TOP/'fm1-base/include')] if device else []),'-I'+str(ROOT/'src'),'-c',str(ROOT/'tests/rings_probe.c'),'-o',str(obj)],check=True)
sources=['rings/dsp/'+s+'.cc' for s in ['fm_voice','part','resonator','string']]+['rings/resources.cc','stmlib/utils/random.cc','stmlib/dsp/atan.cc','stmlib/dsp/units.cc']
subprocess.run(['c++','-std=c++17','-DTEST','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(src),str(ROOT/'tests/rings_oracle.cpp'),str(obj),*[str(src/s) for s in sources],'-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True);(ROOT/'build'/f'rings-oracle-{suffix}-results.json').write_text(r.stdout);print(r.stdout,r.stderr);raise SystemExit(r.returncode)
