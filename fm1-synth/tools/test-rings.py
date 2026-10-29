#!/usr/bin/env python3
"""Offline Rings examples, two-voice allocation/stealing/release and shared FX arena."""
import ctypes as C,json,math
from bank import compile_source,pack,validate,LIB,ROOT
examples={}
for name,effect in [('rings-'+n,False) for n in ['modal','sympathetic','string','fm','chords','reverb']]+[('rings-bus',True),('master-echo',True)]:
 source=(ROOT/'examples'/f'{name}.scd').read_text();binary,info=compile_source(source,effect)
 if name.startswith('rings-'):
  (ROOT/'examples'/f'{name}.scsyndef').write_bytes(binary);(ROOT/'examples'/f'{name}.fmslot').write_bytes(pack(info['name'],source,binary,effect))
 examples[name]=(binary,info)
bypass,_=compile_source('SynthDef(\\bypass, { Out.ar(0, In.ar(0, 2)) })',True)
carrier,_=compile_source('SynthDef(\\exciter, { arg freq=220,amp=0.3,gate=1; Out.ar(0,SinOsc.ar(freq)*amp*EnvGen.kr(Env.asr(0.005,1,0.1),gate,doneAction:2)) })')
for model in (2,5):
 binary=(ROOT.parent/'synthdef-engine/fixtures'/f'm13_MiRings{model}_internal.scsyndef').read_bytes();validate(binary,True)
 try:validate(binary)
 except ValueError:pass
 else:raise AssertionError('Full eight-string model admitted to a 64 KiB synth voice')
results=[]
for name in ['rings-modal','rings-sympathetic','rings-string','rings-fm','rings-chords','rings-reverb','rings-bus']:
 binary=carrier if name=='rings-bus' else examples[name][0]
 effect=examples[name][0] if name=='rings-bus' else bypass if name=='rings-reverb' else examples['master-echo'][0]
 graph,info=validate(binary);fxgraph,fxinfo=validate(effect,True)
 total=2*info['delay_bytes']+fxinfo['delay_bytes'];assert total<=131072
 if name in ('rings-sympathetic','rings-chords'):assert total==131040
 player=C.create_string_buffer(LIB.fms_performance_size());fxvoice=C.create_string_buffer(LIB.fms_voice_size());assert LIB.fms_init(player,graph);assert LIB.fms_attach_effect(player,fxgraph,fxvoice);assert LIB.fms_reset_effect(player)
 for i,v in enumerate([.4,.6,.5,.4,.25,.5,.6,.5]):LIB.fms_knob(player,i,v)
 LIB.fms_note_on(player,1,60,70);LIB.fms_note_on(player,2,67,90);peaks=[0,0]
 for block in range(4096):
  if block in (200,400,600,800):
   for k in range(4):LIB.fms_knob(player,k,[0,.25,.75,1][(block//200)-1])
  if block==500:LIB.fms_note_on(player,3,72,80)
  if block==1000:
   for key in (1,2,3):LIB.fms_note_off(player,key)
  buf=(C.c_float*128)();LIB.fms_render(player,buf);assert all(math.isfinite(x) for x in buf)
  for c in range(2):peaks[c]=max(peaks[c],max(abs(buf[i]) for i in range(c,128,2)))
 assert not LIB.fms_faults(player),(name,'fault');assert min(peaks)>1e-5 and max(peaks)<.99,(name,peaks)
 tail=max(abs(x) for x in buf)
 if name!='rings-bus':assert tail<1e-6,(name,tail)
 results.append(dict(patch=name,voice_bytes=info['delay_bytes'],fx_bytes=fxinfo['delay_bytes'],shared_bytes=total,frames=4096*64,peaks=peaks,tail_peak=tail,faults=0))
(ROOT/'build/rings-emulation.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(results,indent=2))
