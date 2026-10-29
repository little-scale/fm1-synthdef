#!/usr/bin/env python3
"""Independent SC chaos transition and exact integer-random arithmetic checks."""
from pathlib import Path
import subprocess,sys,hashlib,json
ROOT=Path(__file__).resolve().parents[1];TOP=ROOT.parent;device='--device' in sys.argv
expected=json.loads((ROOT/'reference/sources.json').read_text())['sha256']['ChaosUGens.cpp']
assert hashlib.sha256((ROOT/'reference/ChaosUGens.cpp').read_bytes()).hexdigest()==expected
obj=ROOT/'build'/('chaos-probe-device.o' if device else 'chaos-probe.o')
exe=ROOT/'build'/('chaos-oracle-device' if device else 'chaos-oracle')
sc=ROOT/'reference/sc-headers/include'
subprocess.run(['cc','-std=c99','-O2','-fno-fast-math','-ffp-contract=off','-DPLAT_HOST',*(['-I'+str(TOP/'fm1-synth/firmware/device_include'),'-I'+str(TOP/'fm1-base/include')] if device else []),'-I'+str(ROOT/'src'),'-c',str(ROOT/'tests/chaos_probe.c'),'-o',str(obj)],check=True)
subprocess.run(['c++','-std=c++17','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(sc/'plugin_interface'),'-I'+str(sc/'common'),'-I'+str(ROOT/'src'),str(ROOT/'tests/chaos_oracle.cpp'),str(obj),'-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True);print(r.stdout,r.stderr)
(ROOT/'build'/('chaos-oracle-device-results.json' if device else 'chaos-oracle-host-results.json')).write_text(r.stdout)
if r.returncode:raise SystemExit(r.returncode)
noise=ROOT/'build'/('noise-oracle-device' if device else 'noise-oracle-host')
subprocess.run(['c++','-std=c++17','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(sc/'plugin_interface'),'-I'+str(sc/'common'),str(ROOT/'tests/noise_oracle.cpp'),str(obj),'-o',str(noise)],check=True)
r=subprocess.run([str(noise)],capture_output=True,text=True);print(r.stdout,r.stderr)
(ROOT/'build'/('noise-oracle-device-results.json' if device else 'noise-oracle-host-results.json')).write_text(r.stdout)
raise SystemExit(r.returncode)
