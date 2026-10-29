#!/usr/bin/env python3
"""Compile synthesis/effects examples and emulate two voices through the frequency-shift/limiter bus; no device/bank writes."""
import sys,json,ctypes as C,math
from pathlib import Path
from bank import compile_source,pack,validate,LIB,ROOT
examples={}
for name,effect in [('frequency-shift-bus',True),('feedback-lead',False),('plucked-string',False),('harmonic-blip',False)]:
 source=(ROOT/'examples'/f'{name}.scd').read_text();binary,info=compile_source(source,effect);payload=pack(info['name'],source,binary,effect)
 (ROOT/'examples'/f'{name}.scsyndef').write_bytes(binary);(ROOT/'examples'/f'{name}.fmslot').write_bytes(payload)
 examples[name]=(binary,info)
results=[]
for name,setting in [(name,setting) for name in ['feedback-lead','plucked-string','harmonic-blip'] for setting in [0,.25,.5,.75,1]]:
 graph,info=validate(examples[name][0]);effect_graph,fxinfo=validate(examples['frequency-shift-bus'][0],True)
 total=2*info['delay_bytes']+fxinfo['delay_bytes'];assert total<=131072
 player=C.create_string_buffer(LIB.fms_performance_size());fxvoice=C.create_string_buffer(LIB.fms_voice_size())
 LIB.fms_init(player,graph);assert LIB.fms_attach_effect(player,effect_graph,fxvoice);assert LIB.fms_reset_effect(player)
 for k in range(8):LIB.fms_knob(player,k,setting)
 LIB.fms_note_on(player,1,60,70);LIB.fms_note_on(player,2,67,90);peaks=[0,0]
 for block in range(4800):
  if block==200:
   for k in range(8):LIB.fms_knob(player,k,1-setting)
  if block==400:LIB.fms_note_on(player,3,72,80)
  if block==500:LIB.fms_note_off(player,1);LIB.fms_note_off(player,2);LIB.fms_note_off(player,3)
  buf=(C.c_float*128)();LIB.fms_render(player,buf);assert all(math.isfinite(x) for x in buf)
  for c in range(2):peaks[c]=max(peaks[c],max(abs(buf[i]) for i in range(c,128,2)))
 print(name,setting,peaks,LIB.fms_faults(player),flush=True)
 assert not LIB.fms_faults(player);assert min(peaks)>1e-4 and max(peaks)<.99
 assert max(abs(x) for x in buf)<1e-7
 results.append(dict(patch=name,voice_bytes=info['delay_bytes'],fx_bytes=fxinfo['delay_bytes'],shared_bytes=total,initial_knobs=setting,live_knobs=1-setting,frames=4800*64,peaks=peaks,faults=0,release_silent=True))
(ROOT/'build/synthesis-emulation.json').write_text(json.dumps(results,indent=2)+'\n');print(results)
