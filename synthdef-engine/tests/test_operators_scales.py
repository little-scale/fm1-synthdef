#!/usr/bin/env python3
"""Mathematical identities, hostile domains, immutable tables and voice isolation."""
import ctypes,math,unittest
from test_engine import Engine,node,synth,lib

def arithmetic(op,a,b=None,rate=2):
 constants=[0,a]+([] if b is None else [b]);nodes=[node('DC',[(-1,1)],rate=rate)]
 nodes.append(node('UnaryOpUGen' if b is None else 'BinaryOpUGen',[(0,0)]+([] if b is None else [(-1,2)]),rate=rate,special=op))
 index=1
 if rate==1:nodes.append(node('K2A',[(1,0)]));index=2
 nodes.append(node('Out',[(-1,0),(index,0)],outputs=[]))
 return synth(nodes,constants)

def table_patch(values=(1,3,5,6,8,10,12),octave=12,rate=1,edit=None):
 size=len(values);constants=[0,1,size,octave]+list(values)
 nodes=[node('Control',rate=1),node('MaxLocalBufs',[(-1,1)],rate=0),node('LocalBuf',[(-1,1),(-1,2),(1,0)],rate=0),node('SetBuf',[(2,0),(-1,0),(-1,2)]+[(-1,4+i) for i in range(size)],rate=0),node('DegreeToKey',[(2,0),(0,0),(-1,3)],rate=rate)]
 if edit:edit(nodes,constants)
 index=len(nodes)-1
 if rate==1:nodes.append(node('K2A',[(index,0)]));index+=1
 nodes.append(node('Out',[(-1,0),(index,0)],outputs=[]))
 return synth(nodes,constants,controls=[0],names=[('degree',0)])

class OperatorsScales(unittest.TestCase):
 def value(self,data):
  e=Engine(data);self.assertTrue(e.ok,e.error);self.assertTrue(e.start());out=e.render();self.assertFalse(lib.test_fault(e.v));self.assertEqual(out[1::2],[0]*64);return out[::2]
 def test_unary_math_domains_and_rates(self):
  funcs={23:lambda x:440*2**(x-4.75),24:lambda x:math.log2(x/440)+4.75,32:math.acos,33:math.atan,34:math.sinh,35:math.cosh,36:math.tanh}
  for op,fn in funcs.items():
   for a in ([0.01,110,440,12000] if op==24 else [-1,-0.25,0,0.25,1]):
    for rate in (1,2):
     with self.subTest(op=op,a=a,rate=rate):self.assertAlmostEqual(self.value(arithmetic(op,a,rate=rate))[-1],fn(a),delta=max(2e-6,abs(fn(a))*2e-6))
 def test_binary_signed_math_and_rates(self):
  funcs={3:lambda a,b:math.floor(a/b),22:math.atan2,23:math.hypot,24:lambda a,b:abs(a)+abs(b)-(math.sqrt(2)-1)*min(abs(a),abs(b)),25:lambda a,b:math.copysign(abs(a)**b,a),30:lambda a,b:a*b+a,31:lambda a,b:a*b+a+b,32:lambda a,b:a*a*b,33:lambda a,b:a*a*b-a*b*b,34:lambda a,b:a*a-b*b,35:lambda a,b:a*a+b*b,36:lambda a,b:(a+b)**2,37:lambda a,b:(a-b)**2,38:lambda a,b:abs(a-b),39:lambda a,b:0 if a<b else a,40:lambda a,b:0 if b<0 else a*b,41:lambda a,b:a if a>=0 else a*b,43:lambda a,b:a-min(b,max(-b,a)),46:lambda a,b:a}
  for op,fn in funcs.items():
   for a,b in [(-1.5,0.5),(0.5,0.25),(-0.25,-2),(0.125,-0.5)]:
    for rate in (1,2):
     with self.subTest(op=op,a=a,b=b,rate=rate):self.assertAlmostEqual(self.value(arithmetic(op,a,b,rate))[-1],fn(a,b),delta=2e-6)
 def test_integer_truncation_signs_and_limits(self):
  for a,b in [(15.9,6.5),(-15.9,6.5),(15.9,-6.5),(-15.9,-6.5),(0,-6.5),(-6.5,0),(0,0),(16777216,8)]:
   ia,ib=int(a),int(b);g=math.gcd(ia,ib)
   if not ia:g=ib
   elif not ib:g=ia
   elif ia<0 and ib<0:g=-g
   l=0 if not g else ia*ib//g
   for op,expected in [(17,l),(18,g)]:self.assertEqual(self.value(arithmetic(op,a,b))[-1],expected)
  for a,b in [(14930352,9227465),(9227465,14930352),(-14930352,-9227465)]:self.assertEqual(self.value(arithmetic(18,a,b))[-1],-1 if a<0 and b<0 else 1)
  for op in (17,18):
   e=Engine(arithmetic(op,16777218,3));self.assertTrue(e.start());self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
 def test_mixed_rate_upstream_edge_cases_against_math(self):
  # SC 3.14.1 idiv_ak writes only one sample for a held divisor.
  nodes=[node('Control',rate=1),node('DC',[(-1,1)]),node('BinaryOpUGen',[(1,0),(0,0)],special=3),node('Out',[(-1,0),(2,0)],outputs=[])]
  e=Engine(synth(nodes,[0,-15.9],controls=[3.2],names=[('value',0)]));self.assertTrue(e.ok,e.error);self.assertTrue(e.start())
  self.assertEqual(e.render()[::2],[-5]*64);e.set('value',6.4);e.render();self.assertEqual(e.render()[::2],[-3]*64)
  # SC 3.14.1 firstArg_ka dispatches to addition. Keep firstArg's intended identity.
  nodes=[node('Control',rate=1),node('DC',[(-1,1)]),node('BinaryOpUGen',[(0,0),(1,0)],special=46),node('Out',[(-1,0),(2,0)],outputs=[])]
  e=Engine(synth(nodes,[0,10],controls=[-.4],names=[('value',0)]));self.assertTrue(e.ok,e.error);self.assertTrue(e.start());self.assertAlmostEqual(e.render()[0],-.4,places=6)
  e.set('value',.3);out=e.render()[::2]
  for j,value in enumerate(out):self.assertAlmostEqual(value,-.4+(.7*j/64),delta=2e-7)
  self.assertFalse(lib.test_fault(e.v))
 def test_invalid_math_faults_are_contained(self):
  for data in [arithmetic(32,1.1),arithmetic(24,-1),arithmetic(3,1,0),arithmetic(25,0,-1)]:
   e=Engine(data);self.assertTrue(e.ok,e.error);self.assertTrue(e.start());self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
 def test_table_negative_octave_boundaries_and_fractional_degrees(self):
  values=[1,3,5,6,8,10,12]
  for rate in (1,2):
   e=Engine(table_patch(values,rate=rate));self.assertTrue(e.ok,e.error);self.assertTrue(e.start());self.assertEqual(lib.fm_delay_required(e.g,44100),0)
   for degree in [-16777216,-100,-21,-14,-7,-6.999,-1,-0.1,0,0.9,6.999,7,14,16777216]:
    self.assertTrue(e.set('degree',degree));e.render();out=e.render();index=math.floor(degree);expected=ctypes.c_float(values[index%len(values)]+12*(index//len(values))).value;self.assertEqual(out[-2],expected);self.assertFalse(lib.test_fault(e.v))
 def test_table_sizes_multiple_voices_and_no_memory(self):
  for size in (1,2,7,19,31,128):
   data=table_patch([i*0.25 for i in range(size)],octave=19.9);a=Engine(data);b=Engine(data)
   self.assertTrue(a.ok,a.error);self.assertTrue(b.ok,b.error);self.assertTrue(a.start());self.assertTrue(b.start())
   for d in range(-size*2,size*2+1):
    a.set('degree',d);b.set('degree',d+1);a.render();b.render();av=a.render()[-2];bv=b.render()[-2];self.assertEqual(av,(d%size)*.25+19*(d//size));self.assertEqual(bv,((d+1)%size)*.25+19*((d+1)//size))
 def test_table_out_of_domain_degrees_fault(self):
  for degree in (16777218,-16777218,1e20):
   e=Engine(table_patch());self.assertTrue(e.start());e.set('degree',degree);self.assertEqual(e.render(),[0]*128);self.assertTrue(lib.test_fault(e.v))
 def test_reject_mutable_partial_dynamic_and_invalid_tables(self):
  edits=[lambda n,c:n.pop(3),lambda n,c:n.__setitem__(4,node('UnaryOpUGen',[(2,0)],rate=0,special=0)),lambda n,c:n.__setitem__(1,node('MaxLocalBufs',[(-1,0)],rate=0)),lambda n,c:c.__setitem__(2,129),lambda n,c:n.__setitem__(2,node('LocalBuf',[(-1,2),(-1,2),(1,0)],rate=0)),lambda n,c:n.__setitem__(3,node('SetBuf',[(2,0),(-1,1),(-1,2)]+[(-1,4+i) for i in range(7)],rate=0)),lambda n,c:n.__setitem__(3,node('SetBuf',[(2,0),(-1,0),(-1,2)]+[(0,0)]*7,rate=0)),lambda n,c:n.__setitem__(4,node('DegreeToKey',[(-1,0),(0,0),(-1,3)],rate=1)),lambda n,c:n.__setitem__(4,node('DegreeToKey',[(2,0),(0,0),(0,0)],rate=1)),lambda n,c:n.insert(4,node('SetBuf',[(2,0),(-1,0),(-1,2)]+[(-1,4+i) for i in range(7)],rate=0))]
  for edit in edits:self.assertFalse(Engine(table_patch(edit=edit)).ok)
  self.assertFalse(Engine(table_patch([])).ok);self.assertFalse(Engine(table_patch(list(range(129)))).ok)
 def test_all_new_fixtures_render(self):
  from pathlib import Path
  for p in sorted((Path(__file__).resolve().parents[1]/'fixtures').glob('m12_*.scsyndef')):
   e=Engine(p.read_bytes());self.assertTrue(e.ok,(p.name,e.error));self.assertTrue(e.start())
   for _ in range(5):self.assertTrue(all(math.isfinite(x) for x in e.render()))
   self.assertFalse(lib.test_fault(e.v),p.name)

if __name__=='__main__':unittest.main()
