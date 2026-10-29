#!/usr/bin/env python3
"""Rings graph validation, bounded arenas, triggers, bypass, isolation and domains."""
import ctypes as C,math,unittest
from test_engine import Engine,node,synth,lib
SIZES=[1396,67568,13536,228,67568,56836]
def patch(model,compact=0,rate=2,outputs=2,source=0,parameter_rate=1,compact_control=False):
 values=[1,48,.4,.6,.5,.4,0,0]
 nodes=[node('Control',rate=parameter_rate,outputs=[parameter_rate]*8)]
 refs=[(-1,0)]+[(0,i) for i in range(8)]+[(-1,1)]
 if source==1:refs[0]=(0,0)
 if source==2:nodes.append(node('DC',[(-1,2)]));refs[0]=(1,0)
 if compact_control:refs[9]=(0,0)
 index=len(nodes);nodes.append(node('MiRings'+str(model),refs,rate=rate,outputs=[rate]*outputs));nodes.append(node('Out',[(-1,0)]+[(index,i) for i in range(outputs)],outputs=[]))
 return synth(nodes,constants=[0,compact,.1],controls=values,names=[('p'+str(i+1),i) for i in range(8)])
class Rings(unittest.TestCase):
 def test_layout_and_literal_budget(self):
  for model in range(1,7):
   for opts in [{'rate':0},{'rate':1},{'outputs':1},{'outputs':3},{'source':1},{'parameter_rate':2},{'compact_control':True},{'compact':-1},{'compact':.5},{'compact':2}]:
    e=Engine(patch(model,**opts));self.assertFalse(e.ok,(model,opts,e.error))
 def test_exact_arenas_and_bounds(self):
  for model in range(1,7):
   for compact in (0,1):
    e=Engine(patch(model,compact));self.assertTrue(e.ok,e.error);need=(32752 if compact and model in (2,5) else SIZES[model-1])//4
    self.assertEqual(lib.fm_delay_required(e.g,44100),need);self.assertTrue(lib.fm_start(e.v,e.g,44100))
    memory=(C.c_float*(need+2))();memory[0]=123;memory[-1]=456;pointer=C.cast(C.byref(memory,4),C.c_void_p)
    self.assertFalse(lib.fm_bind_delay(e.v,e.g,pointer,need-1));self.assertFalse(lib.fm_bind_delay(e.v,e.g,None,need));self.assertTrue(lib.fm_bind_delay(e.v,e.g,pointer,need))
    for _ in range(500):e.render()
    self.assertEqual((memory[0],memory[-1]),(123,456));self.assertFalse(lib.test_fault(e.v))
 def test_prepare_restart_shared_scratch_and_seed_isolation(self):
  for model in range(1,7):
   a,b,c=[Engine(patch(model,1)) for _ in range(3)]
   for e in (a,b,c):self.assertTrue(e.start())
   lib.fm_prepare(b.g,b.v,b.b);lib.fm_prepare(b.g,b.v,b.b);c.b=b.b;first=[]
   for _ in range(128):c.render();out=a.render();first+=out;self.assertEqual(out,b.render())
   self.assertGreater(max(map(abs,first)),1e-4);a.start();again=[]
   for _ in range(128):again+=a.render()
   self.assertEqual(first,again)
 def test_trigger_bypass_and_sample_rate_guard(self):
  for model in range(1,7):
   e=Engine(patch(model,1));e.start();e.set('p1',0)
   for _ in range(3):self.assertEqual(e.render(),[0]*128)
   e.set('p1',1);out=[]
   for _ in range(20):out+=e.render()
   self.assertGreater(max(map(abs,out)),1e-5)
   e=Engine(patch(model,1,source=2));e.start();e.set('p8',1);self.assertTrue(all(abs(x-.1)<1e-7 for x in e.render()))
   e.start(8000);self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
 def test_live_extremes_and_numerical_containment(self):
  for model in range(1,7):
   for sr in (32000,44100,48000,192000):
    e=Engine(patch(model,1));e.start(sr)
    for value in (0,1,-1e6,1e6,.5):
     for param in range(2,7):e.set('p'+str(param),value if param!=2 else value*114)
     e.set('p1',0);e.render();e.set('p1',1)
     for _ in range(64):self.assertTrue(all(math.isfinite(x) for x in e.render()))
     self.assertFalse(lib.test_fault(e.v),(model,sr,value))
if __name__=='__main__':unittest.main()
