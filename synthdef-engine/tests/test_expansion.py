#!/usr/bin/env python3
"""Admission, boundary, lifetime and fault-policy tests for the UGen expansion."""
import math,unittest
from test_engine import ROOT,Engine,node,synth,lib

def patch(name,params=(),rate=2,source_rate=2,control=False,source_value=1.0):
 constants=[0,source_value,*params];nodes=[node('DC',[(-1,1)],rate=source_rate)]
 if control:nodes.append(node('Control',rate=1));refs=[(0,0),(1,0)]+[(-1,i+2) for i in range(1,len(params))]
 else:refs=[(0,0)]+[(-1,i+2) for i in range(len(params))]
 nodes.append(node(name,refs,rate=rate));index=len(nodes)-1
 if rate==1:nodes.append(node('K2A',[(index,0)]));index+=1
 nodes.append(node('Out',[(-1,0),(index,0)],outputs=[]))
 return synth(nodes,constants=constants,controls=[params[0]] if control else [],names=[('value',0)] if control else [])

class Expansion(unittest.TestCase):
 def test_no_new_delay_allocations_and_fixture_lifetimes(self):
  for fixture in sorted((ROOT/'fixtures').glob('m[89]_*.scsyndef')):
   e=Engine(fixture.read_bytes());self.assertTrue(e.ok,(fixture.name,e.error));self.assertEqual(lib.fm_delay_required(e.g,44100),0)
   self.assertTrue(e.start());
   for block in range(8):self.assertTrue(all(math.isfinite(x) for x in e.render()))
   self.assertFalse(lib.test_fault(e.v),fixture.name)
 def test_invalid_domains_mute_only_voice(self):
  for name,values in [('LinExp',[1,1,.1,1]),('LinExp',[0,1,0,1]),('LinExp',[0,1,-1,1]),('Fold',[1,-1]),('Wrap',[1,-1]),('Trig',[-1]),('Trig1',[1e12]),('Slew',[-1,1]),('Lag2',[-1]),('Lag3',[-1]),('BPeakEQ',[-10,.5,0]),('BLowShelf',[1000,0,0]),('BHiShelf',[1000,1,200]),('Stepper',[0,3,-3,1,0]),('PulseDivider',[1e12,0])]:
   e=Engine(patch(name,values));self.assertTrue(e.ok,e.error);self.assertTrue(e.start());out=e.render();self.assertTrue(lib.test_fault(e.v),name);self.assertEqual(out,[0]*128,name)
 def test_live_invalid_parameter_faults(self):
  for name,values,bad in [('Lag2',[.01],-1),('Slew',[1,1],-1),('PulseDivider',[2,0],1e12),('BPeakEQ',[1000,.5,0],30000)]:
   e=Engine(patch(name,values,control=True));self.assertTrue(e.ok,e.error);self.assertTrue(e.start());e.render();self.assertFalse(lib.test_fault(e.v),name);e.set('value',bad);self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v),name)
 def test_declared_rate_limits(self):
  e=Engine(patch('MiMu',[1,0],rate=1,source_rate=1));self.assertFalse(e.ok)
  for name in ['BPeakEQ','BLowShelf','BHiShelf']:
   self.assertFalse(Engine(patch(name,[100,.5,0],rate=1,source_rate=1)).ok)
  e=Engine(patch('Amplitude',[.01,.03],rate=1));self.assertTrue(e.ok,e.error);self.assertTrue(e.start());self.assertTrue(all(math.isfinite(x) for x in e.render()));self.assertFalse(lib.test_fault(e.v))
 def test_equal_bounds_and_zero_quantization(self):
  for name in ['Fold','Wrap']:
   e=Engine(patch(name,[.25,.25]));self.assertTrue(e.ok,e.error);e.start();self.assertEqual(e.render()[::2],[.25]*64)
  for op in [19,20,21]:
   e=Engine(synth([node('DC',[(-1,1)]),node('BinaryOpUGen',[(0,0),(-1,0)],special=op),node('Out',[(-1,0),(1,0)],outputs=[])]));self.assertTrue(e.ok,e.error);e.start();self.assertEqual(e.render()[::2],[1]*64)
 def test_oscillator_domain_guards(self):
  for name,values in [('VarSaw',[0,.5]),('SyncSaw',[31]),('LFCub',[0]),('LFPar',[0]),('LFDNoise0',[]),('LFDNoise1',[]),('LFDNoise3',[])]:
   data=patch(name,values,source_value=-1.0)
   e=Engine(data);self.assertTrue(e.ok,e.error);e.start();e.render();self.assertTrue(lib.test_fault(e.v),name)
if __name__=='__main__':unittest.main()
