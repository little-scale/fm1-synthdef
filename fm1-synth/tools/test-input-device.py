#!/usr/bin/env python3
"""Alternate battery sampling off/on during two-voice playback. Leave keys untouched.

No slot contents are changed. Sampling, selected/viewed slots and physical
MASTER control are restored in finally, including when a check fails.
"""
import argparse,json,time
from pathlib import Path
import mido
from device import Device
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--seconds',type=float,default=30)
args=parser.parse_args()
if args.seconds<=0:parser.error('--seconds must be positive')
args.output.mkdir(parents=True,exist_ok=True)
d=Device();records=[];base=d.inputs();expected=base['midi_note_events'];initial=d.stats()
def remote(p):d.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43]+p));time.sleep(.06)
def note(key,on):
 global expected
 d.out.send(mido.Message('note_on' if on else 'note_off',note=key,velocity=70 if on else 0))
 if on:expected+=1
try:
 remote([4,0,8]);remote([1,64,0]);remote([1,0,0]);s=d.stats();remote([3,2,64-s['view_slot_index']])
 for enabled in [0,1,0,1]:
  start=d.inputs(bool(enabled));until=time.monotonic()+args.seconds;chords=0
  while time.monotonic()<until:
   note(60,True);note(67,True);time.sleep(.16);note(60,False);note(67,False);time.sleep(.45);chords+=1
   state=d.inputs();row=dict(enabled=enabled,chord=chords,inputs=state,battery=d.battery(),stats=d.stats());records.append(row)
   assert state['key_note_events']==base['key_note_events'],('keyboard',row)
   assert state['midi_note_events']==expected,('midi',expected,row)
   assert row['stats']['overloads']==initial['overloads'] and not row['stats']['faults'],row
  print(json.dumps(dict(battery_sampling=enabled,chords=chords,samples=d.inputs()['battery_samples']-start['battery_samples'],unexpected_notes=0)),flush=True)
finally:
 d.inputs(bool(base['battery_sampling']));d.out.send(mido.Message('control_change',control=123,value=0));remote([1,64,0]);remote([1,0,0]);s=d.stats();remote([3,2,64+initial['slot_index']-s['view_slot_index']])
 if initial['view_slot_index']==8:remote([3,2,64+8-initial['slot_index']])
 remote([4,1,32]);d.close()
 (args.output/'input-ab.json').write_text(json.dumps(records,indent=2)+'\n')
