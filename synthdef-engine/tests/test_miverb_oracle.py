#!/usr/bin/env python3
"""Identical-input oracle: isolates DSP accuracy from upstream oscillator error."""
import ctypes as C,json,math,random,subprocess
from test_engine import ROOT,Engine,node,synth,lib
mi=ROOT/'reference/mi-ugens'
subprocess.run(['c++','-std=c++17','-DTEST','-O2','-fno-fast-math','-ffp-contract=off','-fPIC','-shared','-I'+str(mi/'projects/MiVerb/include'),'-I'+str(mi/'eurorack'),str(ROOT/'tests/miverb_oracle.cpp'),'-o',str(ROOT/'build/miverb-oracle.dylib')],check=True)
oracle=C.CDLL(str(ROOT/'build/miverb-oracle.dylib'))
oracle.mi_create.argtypes=[C.c_float];oracle.mi_create.restype=C.c_void_p
oracle.mi_destroy.argtypes=[C.c_void_p];oracle.mi_process.argtypes=[C.c_void_p]*4
params=[.7,.8,.5,.05,0,.625];names=['time','wet','damp','hp','freeze','diff']
data=synth([node('Control',rate=1,outputs=[1]*6),node('In',[(-1,0)],outputs=[2,2]),node('MiVerb',[(0,i) for i in range(6)]+[(1,0),(1,1)],outputs=[2,2]),node('Out',[(-1,0),(2,0),(2,1)],outputs=[])],controls=params,names=[(n,i) for i,n in enumerate(names)])
results=[]
for sr in [8000,44100,48000,192000]:
 for amplitude in [.2,4.0]:
  e=Engine(data);assert e.ok,e.error;assert e.start(sr);handle=oracle.mi_create(sr);rng=random.Random(42);peak=energy=0;total=0
  try:
   for block in range(1600):
    values=([.7,.8,.5,.05,0,.625] if block<200 else [1.2,.9,.1,.1,0,.8] if block<500 else [.8,1,.4,0,1,.625] if block<900 else [.3,.4,1,1,0,0] if block<1200 else [.9,.9,.3,.02,0,.95])
    for name,value in zip(names,values):assert e.set(name,value)
    inp=(C.c_float*128)(*[rng.uniform(-amplitude,amplitude) if block<1100 else 0 for _ in range(128)])
    actual=(C.c_float*128)();expected=(C.c_float*128)();par=(C.c_float*6)(*values)
    lib.fm_render_input(e.g,e.v,e.b,actual,inp);oracle.mi_process(handle,inp,expected,par)
    assert not lib.test_fault(e.v)
    for a,b in zip(actual,expected):
     err=abs(a-b);peak=max(peak,err);energy+=err*err;total+=1
   rms=math.sqrt(energy/total);ok=peak<5e-6 and rms<1e-6
   results.append(dict(sample_rate=sr,input_amplitude=amplitude,frames=1600*64,peak_error=peak,rms_error=rms,passed=ok))
  finally:oracle.mi_destroy(handle)
(ROOT/'build/miverb-oracle-results.json').write_text(json.dumps(results,indent=2)+'\n')
for r in results:print(r)
assert all(r['passed'] for r in results)
