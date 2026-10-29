#!/usr/bin/env python3
"""Host-only MiRings feasibility probe. Does not install a UGen or touch hardware.

Pass --upstream pointing to mi-UGens commit
10f6824f68ad63475f67dfe3fe5016e2d2634f84 (including eurorack sources).
"""
import argparse,hashlib,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--upstream',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];up=a.upstream/'eurorack'
manifest=json.loads((root/'reference/mirings-probe-sources.json').read_text())
for rel,digest in manifest['sha256'].items():
 if hashlib.sha256((a.upstream/rel).read_bytes()).hexdigest()!=digest:raise SystemExit('Reference mismatch: '+rel)
sources=['rings/dsp/'+s+'.cc' for s in ['fm_voice','part','resonator','string','string_synth_part']]+['rings/resources.cc','stmlib/utils/random.cc','stmlib/dsp/atan.cc','stmlib/dsp/units.cc']
exe=root/'build/mirings-probe'
subprocess.run(['c++','-std=c++17','-DTEST','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(up),str(root/'tools/probe-mirings.cpp'),*[str(up/s) for s in sources],'-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True,check=True);result=json.loads(r.stdout)
result.update(commit=manifest['commit'],platform='host; size and audio smoke test only',hardware_tested=False)
a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
