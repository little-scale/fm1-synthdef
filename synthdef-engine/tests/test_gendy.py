#!/usr/bin/env python3
"""Gendy memory admission, rates, lifetime, hostile controls and long renders."""
import ctypes as C,math,unittest
from test_engine import ROOT,Engine,node,synth,lib

def patch(kind,cp=12,rate=2,live=None):
 values=[0,0,.7,.6,37,55,.5,.5,cp,cp,1.17,.31]
 if kind==3:values=[0,0,.7,.6,37,.5,.5,cp,cp]
 elif kind==1:values=values[:10]
 nodes=[];refs=[(-1,i+1) for i in range(len(values))];constants=[0]+values
 if live is not None:nodes=[node('Control',rate=1)];refs[live]=(0,0)
 index=len(nodes);nodes.append(node('Gendy'+str(kind),refs,rate=rate))
 if rate==1:nodes.append(node('K2A',[(index,0)]));index+=1
 nodes.append(node('Out',[(-1,0),(index,0)],outputs=[]))
 return synth(nodes,constants=constants,controls=[values[live]] if live is not None else [],names=[('value',0)] if live is not None else [])

class Gendy(unittest.TestCase):
 def test_memory_bounds(self):
  for kind in (1,2,3):
   for cp in (1,12,64):
    e=Engine(patch(kind,cp));self.assertTrue(e.ok,e.error);need=(2*cp if kind==1 else 4*cp if kind==2 else 4*cp+2)
    for sr in (8000,44100,48000,192000):self.assertEqual(lib.fm_delay_required(e.g,sr),need)
    self.assertTrue(lib.fm_start(e.v,e.g,44100));self.assertFalse(lib.fm_bind_delay(e.v,e.g,None,need))
    mem=(C.c_float*(need+2))();mem[0]=123;mem[-1]=456;ptr=C.cast(C.byref(mem,4),C.c_void_p)
    self.assertFalse(lib.fm_bind_delay(e.v,e.g,ptr,need-1));self.assertTrue(lib.fm_bind_delay(e.v,e.g,ptr,need))
    for _ in range(100):e.render()
    self.assertEqual(mem[0],123);self.assertEqual(mem[-1],456);self.assertFalse(lib.test_fault(e.v))
    self.assertFalse(lib.fm_bind_delay(e.v,e.g,ptr,need));self.assertTrue(lib.fm_start(e.v,e.g,44100));self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
 def test_reject_bad_capacity_rate_and_layout(self):
  for kind in (1,2,3):
   for cp in (0,-1,1.5,65,1e12):self.assertFalse(Engine(patch(kind,cp)).ok)
   self.assertFalse(Engine(patch(kind,rate=0)).ok)
   self.assertFalse(Engine(patch(kind,live=7 if kind==3 else 8)).ok)
   good=patch(kind)
   # Every control parameter must reject an audio-rate source.
   for parameter in range(9 if kind==3 else 10 if kind==1 else 12):
    data=patch(kind,live=parameter);data=data.replace(b'\x07Control\x01',b'\x07Control\x02');self.assertFalse(Engine(data).ok)
 def test_restart_prepare_and_voice_isolation(self):
  for kind in (1,2,3):
   data=patch(kind);a,b,c=[Engine(data) for _ in range(3)]
   for e in (a,b,c):self.assertTrue(e.start())
   c.b=b.b;lib.fm_prepare(b.g,b.v,b.b);lib.fm_prepare(b.g,b.v,b.b);first=[]
   for _ in range(100):c.render();out=a.render();self.assertEqual(out,b.render());first+=out
   a.start();again=[]
   for _ in range(100):again+=a.render()
   self.assertEqual(first,again)
 def test_live_controls_and_safe_failure(self):
  for kind in (1,2,3):
   cpinput=7 if kind==3 else 8
   for param in range(9 if kind==3 else 10 if kind==1 else 12):
    if param==cpinput:continue
    e=Engine(patch(kind,live=param));self.assertTrue(e.ok,e.error);e.start();e.render();self.assertFalse(lib.test_fault(e.v))
    e.set('value',1e12);self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
   for knum in (-10,0,1,4,64,100):
    e=Engine(patch(kind,live=cpinput+1));e.start();e.set('value',knum)
    for _ in range(100):e.render()
    self.assertFalse(lib.test_fault(e.v))
 def test_long_render_finite_and_seeded(self):
  # 3 types x 2 rates x 4 sample rates x 131072 audio frames = 3,145,728.
  for kind in (1,2,3):
   for rate in (1,2):
    for sr in (8000,44100,48000,192000):
     e=Engine(patch(kind,rate=rate));self.assertTrue(e.ok,e.error);e.start(sr);peak=0
     for _ in range(2048):
      out=e.render();self.assertTrue(all(math.isfinite(x) for x in out));peak=max(peak,max(map(abs,out)))
     self.assertFalse(lib.test_fault(e.v),(kind,rate,sr));self.assertGreater(peak,0)
if __name__=='__main__':unittest.main()
