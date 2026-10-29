#!/usr/bin/env python3
"""Graph admission, allocation, trigger and long-run checks for synthesis batch."""
import unittest,math,ctypes as C,json
from test_engine import Engine,node,synth,lib,ROOT
DEFAULTS={'SinOscFB':[220,.3],'Blip':[220,5],'Phasor':[0,.001,-1,1,0],'Pluck':[.1,1,.02,.003,.2,.5],'MoogFF':[.1,1000,2,0],'FreqShift':[.1,100,0],'Limiter':[.1,.2,.01],**{x:[4000,1,.1,1.1,.5,.1,.1] for x in ['FBSineN','FBSineL','FBSineC']},**{x:[4000,1,.5,.5] for x in ['StandardN','StandardL']}}
def patch(name,args=None,rate=2,live=None,audio=None):
 args=DEFAULTS[name] if args is None else args;constants=[0]+list(args);refs=[(-1,i+1) for i in range(len(args))];nodes=[]
 if live is not None:nodes.append(node('Control',rate=1));refs[live]=(0,0)
 if audio is None and name in ('Pluck','MoogFF','FreqShift','Limiter') and rate==2:audio=0
 if audio is not None:nodes.append(node('DC',[refs[audio]]));refs[audio]=(len(nodes)-1,0)
 idx=len(nodes);nodes.append(node(name,refs,rate=rate))
 if rate==1:nodes.append(node('K2A',[(idx,0)]));idx+=1
 nodes.append(node('Out',[(-1,0),(idx,0)],outputs=[]));return synth(nodes,constants,[] if live is None else [args[live]],[] if live is None else [('value',0)])
def prepare(e,sr=44100):
 assert e.ok,e.error;assert e.start(sr);need=lib.fm_delay_required(e.g,sr)
 if need:
  mem=(C.c_float*(need+2))();mem[0]=123;mem[-1]=456;e.mem=mem;e.need=need
  assert lib.fm_bind_delay(e.v,e.g,C.cast(C.byref(mem,4),C.POINTER(C.c_float)),need)
 return need
class Synthesis(unittest.TestCase):
 def test_layout_and_rate_rejection(self):
  for n in DEFAULTS:
   self.assertFalse(Engine(patch(n,rate=0)).ok,n);self.assertFalse(Engine(patch(n,args=DEFAULTS[n]+[0])).ok,n)
   if n in ('Pluck','FreqShift','Limiter') or n.startswith(('FBSine','Standard')):self.assertFalse(Engine(patch(n,rate=1)).ok,n)
  for n,k in [('SinOscFB',0),('Blip',1),('MoogFF',1),('Pluck',3),('Limiter',1),('FBSineN',1),('StandardL',1)]:self.assertFalse(Engine(patch(n,audio=k)).ok,n)
  for n in ('Pluck','Limiter'):self.assertFalse(Engine(patch(n,live=2)).ok)
 def test_restart_shared_scratch_and_memory_guards(self):
  for n in DEFAULTS:
   a,b,c=[Engine(patch(n)) for _ in range(3)]
   for e in (a,b,c):prepare(e)
   lib.fm_prepare(b.g,b.v,b.b);lib.fm_prepare(b.g,b.v,b.b);c.b=b.b;first=[]
   for _ in range(40):c.render();x=a.render();self.assertEqual(x,b.render(),n);first+=x
   prepare(a);again=[]
   for _ in range(40):again+=a.render()
   self.assertEqual(first,again,n)
   for e in (a,b,c):
    if hasattr(e,'mem'):self.assertEqual((e.mem[0],e.mem[-1]),(123,456))
 def test_allocation_admission_and_limiter_duration(self):
  for n in ('Pluck','Limiter'):
   e=Engine(patch(n));lib.fm_start(e.v,e.g,44100);need=lib.fm_delay_required(e.g,44100);self.assertGreater(need,0);mem=(C.c_float*need)();self.assertFalse(lib.fm_bind_delay(e.v,e.g,mem,need-1));self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
  for sr in (8000,44100,48000,192000):
   for duration in (.00001,.001,.01,.1):
    e=Engine(patch('Limiter',args=[.1,.2,duration]));e.start(sr);self.assertEqual(lib.fm_delay_required(e.g,sr),3*math.ceil(C.c_float(duration).value*sr))
  self.assertFalse(Engine(patch('Limiter',args=[0,.2,0])).ok)
 def test_invalid_live_parameters_silence(self):
  for n,k,value in [('SinOscFB',1,9),('Blip',0,0),('Phasor',3,-2),('Pluck',3,.03),('MoogFF',1,30000),('FreqShift',1,30000),('Limiter',1,-1),('FBSineN',1,17),('StandardL',1,17)]:
   e=Engine(patch(n,live=k));prepare(e);e.render();e.set('value',value);self.assertEqual(e.render(),[0]*128,n);self.assertTrue(lib.test_fault(e.v),n)
 def test_limiter_peak_and_transparent_delayed_input(self):
  for level in (.1,.5,1):
   e=Engine(patch('Limiter',args=[2,level,.001]));prepare(e);samples=[]
   for _ in range(80):samples+=e.render()[::2]
   self.assertLessEqual(max(map(abs,samples)),level+1e-5);self.assertAlmostEqual(samples[-1],level,places=5)
  e=Engine(patch('Limiter',args=[.125,1,.001]));prepare(e);a=[]
  for _ in range(10):a+=e.render()[::2]
  self.assertEqual(a[:89],[0]*89);self.assertEqual(a[89:],[.125]*(len(a)-89))
 def test_phasor_rejects_unrepresentable_small_ranges(self):
  for span in (1e-38,1e-12,0):
   e=Engine(patch('Phasor',args=[1,0,0,span,1e6]));prepare(e);self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
 def test_fault_stops_downstream_buffer_writes(self):
  data=synth([node('Blip',[(-1,0),(-1,1)]),node('Limiter',[(0,0),(-1,2),(-1,3)]),node('Out',[(-1,0),(1,0)],outputs=[])],[0,5,.2,.001])
  e=Engine(data);prepare(e)
  for i in range(1,e.need+1):e.mem[i]=.125
  self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v));self.assertEqual(list(e.mem)[1:-1],[.125]*e.need)
 def test_frequency_shift_initial_phase(self):
  a=Engine(patch('FreqShift',args=[.1,100,0]));b=Engine(patch('FreqShift',args=[.1,100,math.pi]));prepare(a);prepare(b)
  for _ in range(100):self.assertLess(max(abs(x+y) for x,y in zip(a.render(),b.render())),2e-6)
 def test_extended_runs(self):
  results=[]
  for n in DEFAULTS:
   for sr in (32000,44100,48000,192000):
    e=Engine(patch(n));need=prepare(e,sr);peak=0
    for _ in range(1024):
     samples=e.render();self.assertTrue(all(math.isfinite(x) for x in samples));peak=max(peak,max(map(abs,samples)))
    self.assertFalse(lib.test_fault(e.v),(n,sr));results.append(dict(name=n,sample_rate=sr,frames=65536,arena_bytes=need*4,peak=peak,faults=0))
    if need:self.assertEqual((e.mem[0],e.mem[-1]),(123,456))
  (ROOT/'build/synthesis-long-results.json').write_text(json.dumps(results,indent=2)+'\n')
if __name__=='__main__':unittest.main()
