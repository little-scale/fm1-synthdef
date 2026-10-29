#!/usr/bin/env python3
"""Formant/Formlet memory, domains, lifetime and special-case identities."""
import math,unittest
from test_engine import Engine,node,synth,lib

def patch(kind,values,rate=2,live=None):
 nodes=[];refs=[(-1,i+1) for i in range(len(values))];constants=[0]+list(values)
 if live is not None:nodes.append(node('Control',rate=1));refs[live]=(0,0)
 if kind=='Formlet':
  nodes.append(node('Impulse',[(-1,len(constants)),(-1,0)],rate=rate));constants.append(17);refs.insert(0,(len(nodes)-1,0))
 index=len(nodes);nodes.append(node(kind,refs,rate=rate))
 if rate==1:nodes.append(node('K2A',[(index,0)]));index+=1
 nodes.append(node('Out',[(-1,0),(index,0)],outputs=[]))
 return synth(nodes,constants=constants,controls=[values[live]] if live is not None else [],names=[('value',0)] if live is not None else [])

class Formants(unittest.TestCase):
 def test_domains(self):
  for kind,values in [('Formant',[220,1100,440]),('Formlet',[1100,.01,.1])]:
   for param in range(3):
    for invalid in (-1,1e12):
     e=Engine(patch(kind,values,live=param));self.assertTrue(e.ok,e.error);e.start();e.render();self.assertFalse(lib.test_fault(e.v))
     e.set('value',invalid);self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
 def test_rate_and_input_rejection(self):
  self.assertFalse(Engine(patch('Formant',[220,1100,440],rate=1)).ok)
  for kind,values in [('Formant',[220,1100,440]),('Formlet',[1100,.01,.1])]:
   self.assertFalse(Engine(patch(kind,values,rate=0)).ok)
   for param in range(3):
    data=patch(kind,values,live=param).replace(b'\x07Control\x01',b'\x07Control\x02');self.assertFalse(Engine(data).ok)
  self.assertFalse(Engine(synth([node('Formlet',[(-1,1)]*4),node('Out',[(-1,0),(0,0)],outputs=[])])).ok)
 def test_formlet_equal_times_cancel_and_reversing_times_inverts(self):
  for rate in (1,2):
   for time in (0,.001,.1,1):
    e=Engine(patch('Formlet',[100,time,time],rate));self.assertTrue(e.ok,e.error);e.start()
    for _ in range(500):self.assertEqual(e.render(),[0]*128)
   a,b=[Engine(patch('Formlet',[100,*times],rate)) for times in [(.01,.1),(.1,.01)]]
   a.start();b.start()
   for _ in range(500):self.assertEqual(a.render(),[-x for x in b.render()])
 def test_no_arena_restart_and_voice_isolation(self):
  for kind,values in [('Formant',[220,1100,440]),('Formlet',[1100,.01,.1])]:
   data=patch(kind,values);a,b,c=[Engine(data) for _ in range(3)]
   for e in (a,b,c):self.assertTrue(e.ok,e.error);self.assertEqual(lib.fm_delay_required(e.g,44100),0);self.assertTrue(e.start())
   c.b=b.b;lib.fm_prepare(b.g,b.v,b.b);lib.fm_prepare(b.g,b.v,b.b);first=[]
   for _ in range(100):c.render();out=a.render();self.assertEqual(out,b.render());first+=out
   a.start();again=[]
   for _ in range(100):again+=a.render()
   self.assertEqual(first,again)
 def test_long_render_multiple_rates(self):
  for sr in (8000,44100,48000,192000):
   for kind,values in [('Formant',[220,1100,440]),('Formlet',[20,.01,3])]:
    e=Engine(patch(kind,values));e.start(sr);peak=0
    for _ in range(4096):
     out=e.render();self.assertTrue(all(math.isfinite(x) for x in out));peak=max(peak,max(map(abs,out)))
    self.assertGreater(peak,0);self.assertFalse(lib.test_fault(e.v))
if __name__=='__main__':unittest.main()
