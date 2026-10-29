#!/usr/bin/env python3
"""Bit-exact Mu-law oracle with identical inputs, independent of oscillators."""
import ctypes as C,json,random,subprocess
from test_engine import ROOT,Engine,node,synth,lib
sc=ROOT/'reference/sc-headers/include'
subprocess.run(['c++','-std=c++17','-O2','-fno-fast-math','-ffp-contract=off','-fPIC','-shared','-I'+str(sc/'plugin_interface'),'-I'+str(sc/'common'),str(ROOT/'tests/mimu_oracle.cpp'),'-o',str(ROOT/'build/mimu-oracle.dylib')],check=True)
o=C.CDLL(str(ROOT/'build/mimu-oracle.dylib'));o.oracle_mu.argtypes=[C.c_float,C.c_float];o.oracle_mu.restype=C.c_float
results=[]
for audio_gain in (False,True):
 data=synth([node('Control',rate=1,outputs=[1,1]),node('In',[(-1,0)],outputs=[2,2]),node('MiMu',[(1,0),(1,1) if audio_gain else (0,0),(0,1)]),node('Out',[(-1,0),(2,0)],outputs=[])],controls=[1,0],names=[('gain',0),('bypass',1)])
 e=Engine(data);assert e.ok,e.error;assert e.start(44100);rng=random.Random(42);peak=0;frames=0
 for block in range(8192):
  gain=C.c_float(rng.uniform(-10,10)).value;bypass=block%11==0
  e.set('gain',gain);e.set('bypass',int(bypass))
  values=[val for i in range(64) for val in ((block*64+i-262144)/65536 if block<4096 else rng.uniform(-8,8),rng.uniform(-10,10))]
  inp=(C.c_float*128)(*values);actual=(C.c_float*128)()
  lib.fm_render_input(e.g,e.v,e.b,actual,inp);assert not lib.test_fault(e.v)
  for i in range(64):
   expected=inp[2*i] if bypass else o.oracle_mu(inp[2*i],inp[2*i+1] if audio_gain else gain)
   peak=max(peak,abs(actual[2*i]-expected));assert actual[2*i+1]==0
  frames+=64
 results.append(dict(audio_gain=audio_gain,frames=frames,peak_error=peak,passed=peak==0))
(ROOT/'build/mimu-oracle-results.json').write_text(json.dumps(results,indent=2)+'\n')
print(results);assert all(r['passed'] for r in results)
