#!/usr/bin/env python3
"""Compare pinned SC Gendy distributions and identical-state transitions."""
from pathlib import Path
import subprocess,sys,hashlib,json
ROOT=Path(__file__).resolve().parents[1];TOP=ROOT.parent;device='--device' in sys.argv
expected=json.loads((ROOT/'reference/sources.json').read_text())['sha256']['GendynUGens.cpp']
assert hashlib.sha256((ROOT/'reference/GendynUGens.cpp').read_bytes()).hexdigest()==expected,'Pinned Gendy source changed'
from compare_reference import gendy_comparison
reference=[-0.1,0.1]*512
assert gendy_comparison('m10_Gendy2_defaults',reference,reference)['stochastic_passed']
for invalid in ([0.0]*1024,[x*10 for x in reference],[x+1 for x in reference]):
 assert not gendy_comparison('m10_Gendy2_defaults',invalid,reference)['stochastic_passed']
obj=ROOT/'build'/('gendy-probe-device.o' if device else 'gendy-probe.o')
exe=ROOT/'build'/('gendy-oracle-device' if device else 'gendy-oracle')
sc=ROOT/'reference/sc-headers/include'
subprocess.run(['cc','-std=c99','-O2','-fno-fast-math','-ffp-contract=off','-DPLAT_HOST',*(['-I'+str(TOP/'fm1-synth/firmware/device_include'),'-I'+str(TOP/'fm1-base/include')] if device else []),'-I'+str(ROOT/'src'),'-c',str(ROOT/'tests/gendy_probe.c'),'-o',str(obj)],check=True)
subprocess.run(['c++','-std=c++17','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(sc/'plugin_interface'),'-I'+str(sc/'common'),'-I'+str(ROOT/'src'),str(ROOT/'tests/gendy_oracle.cpp'),str(obj),'-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True);print(r.stdout,r.stderr)
(ROOT/'build'/('gendy-oracle-device-results.json' if device else 'gendy-oracle-results.json')).write_text(r.stdout)
raise SystemExit(r.returncode)
