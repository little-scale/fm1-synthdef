#!/usr/bin/env python3
"""Unscaled operator-grid checks against pinned SC formulas on both math backends."""
from pathlib import Path
import subprocess,sys,hashlib,json
ROOT=Path(__file__).resolve().parents[1];TOP=ROOT.parent;device='--device' in sys.argv
sc=ROOT/'reference/sc-headers/include'
manifest=json.loads((ROOT/'reference/sc-headers/sources.json').read_text())['sha256']
for name,digest in manifest.items():assert hashlib.sha256((ROOT/'reference/sc-headers'/name).read_bytes()).hexdigest()==digest,'Pinned SC headers changed'

obj=ROOT/'build'/('operators-device.o' if device else 'operators-host.o');exe=obj.with_suffix('')
subprocess.run(['cc','-std=c99','-O2','-fno-fast-math','-ffp-contract=off','-DPLAT_HOST',*(['-I'+str(TOP/'fm1-synth/firmware/device_include'),'-I'+str(TOP/'fm1-base/include')] if device else []),'-I'+str(ROOT/'src'),'-c',str(ROOT/'tests/operators_probe.c'),'-o',str(obj)],check=True)
subprocess.run(['c++','-std=c++17','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(sc/'plugin_interface'),'-I'+str(sc/'common'),str(ROOT/'tests/operators_oracle.cpp'),str(obj),'-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True);print(r.stdout,r.stderr)
(ROOT/'build'/('operators-oracle-device-results.json' if device else 'operators-oracle-host-results.json')).write_text(r.stdout)
raise SystemExit(r.returncode)
