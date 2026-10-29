#!/usr/bin/env python3
"""Domain, lifecycle, trigger and long-run checks for new stochastic generators."""
import math,unittest,json,struct
from test_engine import Engine,node,synth,lib,ROOT
DEFAULTS={'Crackle':[1.95],'LFClipNoise':[100],'LFDClipNoise':[100],'GrayNoise':[],'Hasher':[.25],'CoinGate':[.5,0],'TIRand':[-5,5,0],'TWindex':[0,1,1,2,3],'Logistic':[3.9,4000,.5],**{n:[4000,1.4,.3,0,0] for n in ['HenonN','HenonL','HenonC']},**{n:[4000,1,1.9,0] for n in ['CuspN','CuspL']},**{n:[4000,1,3,.5,.5,.5,.5] for n in ['LatoocarfianN','LatoocarfianL','LatoocarfianC']},'LorenzL':[4000,10,28,2.667,.05,.1,0,0]}
def patch(name,rate=2,values=None,live=None,audio=None):
 values=DEFAULTS[name] if values is None else values;constants=[0]+list(values);refs=[(-1,i+1) for i in range(len(values))];nodes=[]
 if live is not None:nodes.append(node('Control',rate=1));refs[live]=(0,0)
 if audio is None and rate==2:audio={'Hasher':0,'CoinGate':1,'TIRand':2}.get(name)
 if audio is not None:nodes.append(node('DC',[(-1,audio+1)]));refs[audio]=(len(nodes)-1,0)
 index=len(nodes);nodes.append(node(name,refs,rate=rate))
 if rate==1:nodes.append(node('K2A',[(index,0)]));index+=1
 nodes.append(node('Out',[(-1,0),(index,0)],outputs=[]))
 return synth(nodes,constants,[] if live is None else [values[live]],[] if live is None else [('value',0)])
class Stochastic(unittest.TestCase):
 def test_no_arena_restart_preparation_and_voice_isolation(self):
  for name in DEFAULTS:
   a,b,c=[Engine(patch(name)) for _ in range(3)]
   for e in (a,b,c):self.assertTrue(e.ok,(name,e.error));self.assertEqual(lib.fm_delay_required(e.g,44100),0);self.assertTrue(e.start())
   lib.fm_prepare(b.g,b.v,b.b);lib.fm_prepare(b.g,b.v,b.b);c.b=b.b;first=[]
   for _ in range(24):c.render();x=a.render();self.assertEqual(x,b.render(),name);first+=x
   a.start();again=[]
   for _ in range(24):again+=a.render()
   self.assertEqual(first,again,name)
 def test_layout_rates_and_audio_parameter_rejection(self):
  for name,args in DEFAULTS.items():
   self.assertFalse(Engine(patch(name,rate=0)).ok,name)
   if name.startswith(('Henon','Cusp','Latoocarfian','Lorenz')):self.assertFalse(Engine(patch(name,rate=1)).ok)
   if name!='TWindex':self.assertFalse(Engine(patch(name,values=args+[0])).ok)
   if name not in ('GrayNoise','Hasher','LFDClipNoise','TWindex'):
    self.assertFalse(Engine(patch(name,audio=0)).ok,name)
  self.assertFalse(Engine(patch('Logistic',live=2)).ok)
  self.assertFalse(Engine(patch('TWindex',values=[0,1]+[1]*65)).ok)
 def test_live_invalid_domains_fault_to_silence(self):
  cases=[('Crackle',0,2.1),('LFClipNoise',0,-1),('LFDClipNoise',0,-1),('CoinGate',0,1.1),('TIRand',0,10),('TWindex',2,-1),('Logistic',0,4.1),('HenonN',1,3),('CuspL',2,3),('LatoocarfianC',1,4),('LorenzL',4,.1)]
  for name,param,value in cases:
   e=Engine(patch(name,rate=1 if name in ('TIRand','CoinGate') else 2,live=param));self.assertTrue(e.ok,e.error);e.start();e.render();e.set('value',value)
   if name=='TIRand':
    # Bounds are consumed on a draw, including construction.
    e=Engine(patch(name,rate=1,values=[10,5,1]));e.start()
   self.assertEqual(e.render(),[0]*128,name);self.assertTrue(lib.test_fault(e.v),name)
 def test_weighted_choices_and_true_trigger_edges(self):
  for weights,index in [([0,1,0],1),([0,0,7],2),([9,0,0],0)]:
   e=Engine(patch('TWindex',values=[0,1]+weights,live=0));e.start()
   for value in [0,1,1,0,1]:e.set('value',value);self.assertEqual(e.render()[::2],[index]*64)
  # A positive pulse held across an entire audio block makes exactly one draw.
  constants=[0,-5,5,1];data=synth([node('DC',[(-1,3)]),node('TIRand',[(-1,1),(-1,2),(0,0)]),node('Out',[(-1,0),(1,0)],outputs=[])],constants)
  e=Engine(data);e.start();first=e.render()[::2];self.assertEqual(len(set(first)),1)
  for _ in range(10):self.assertEqual(first,e.render()[::2])
 def test_statistical_checks_reject_bad_audio(self):
  from stochastic_metrics import comparison
  reference=[-.1,0,.1,0]*1024
  self.assertTrue(comparison('m14_HenonN_chaotic',reference,reference)['stochastic_passed'])
  for bad in ([0.0]*len(reference),[x*10 for x in reference],[x+1 for x in reference]):self.assertFalse(comparison('m14_HenonN_chaotic',bad,reference)['stochastic_passed'])
 def test_long_render_activity_and_short_cycles(self):
  records=[]
  for name in ['Crackle','Logistic','HenonN','HenonL','HenonC','CuspN','CuspL','LatoocarfianN','LatoocarfianL','LatoocarfianC','LorenzL']:
   for sr in (32000,44100,48000,192000):
    args=list(DEFAULTS[name]);
    if name not in ('Crackle','Logistic'):args[0]=sr
    elif name=='Logistic':args[1]=sr
    e=Engine(patch(name,values=args));e.start(sr);audio=[]
    for _ in range(2048):audio+=e.render()[::2]
    self.assertFalse(lib.test_fault(e.v),(name,sr));self.assertTrue(all(math.isfinite(x) for x in audio))
    tail=audio[-4096:];mean=sum(tail)/len(tail);variance=sum((x-mean)**2 for x in tail)/len(tail)
    self.assertGreater(variance,1e-7,(name,sr));period=next((p for p in range(1,129) if tail[p:]==tail[:-p]),None);self.assertIsNone(period,(name,sr,period))
    records.append(dict(name=name,sample_rate=sr,frames=len(audio),peak=max(map(abs,audio)),tail_variance=variance,short_period=period))
  (ROOT/'build/stochastic-long-results.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':unittest.main()
