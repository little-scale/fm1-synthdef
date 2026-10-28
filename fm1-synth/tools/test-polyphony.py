#!/usr/bin/env python3
"""Sustained hardware test for Keys (slot 1) and Addy (slot 2).

Plays notes, temporarily turns MASTER down, exercises all four knobs, then
returns to slot 1 and restores the physical MASTER. Does not write slots.
"""
import argparse,json,time
from pathlib import Path
import mido
from device import Device

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
dev=Device();samples=[];baseline=dev.stats()
def remote(data):
    dev.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43]+data));time.sleep(.04)
def select(slot):
    state=dev.stats();remote([3,2,64+slot-state.get('view_slot_index',state['slot_index'])]);time.sleep(.1)
def note(key,on,velocity=80):
    dev.out.send(mido.Message('note_on' if on else 'note_off',note=key,velocity=velocity if on else 0))
def check(phase,slot,voices=2):
    s=dev.stats();s['phase']=phase;samples.append(s);print(json.dumps(s),flush=True)
    assert s['slot_index']==slot and s['loaded'],'Unexpected slot change'
    assert not s['faults'] and s['overloads']==baseline['overloads'] and s['xruns']==baseline['xruns'],'Playback fault/overload/overrun'
    assert s['active_voices']==voices,'Expected simultaneous voices'
try:
    assert dev.info[2]==2,'Device does not advertise two voices'
    remote([4,0,8]);select(0)
    for i in range(24):
        a=36+(i*7)%49;b=a+7
        note(a,True,35+i*2);note(b,True,75)
        time.sleep(.08)
        remote([3,3+i%4,64+(10 if i//4%2==0 else -10)])
        check('Keys chord + live knob',0)
        note(a,False);note(b,False);time.sleep(.08)
    select(1)
    for i in range(18):
        a=48+i%24;b=a+7
        onset=time.monotonic();note(a,True,35+i*3);note(b,True,80)
        time.sleep(.12);check('Addy attack',1)
        time.sleep(max(0,onset+.57-time.monotonic()));check('Addy decay',1)
        note(a,False);note(b,False)
        time.sleep(.55 if i%3==0 else .03) # complete tails and rapid stealing
    select(0)
finally:
    dev.out.send(mido.Message('control_change',control=123,value=0))
    remote([1,64,0]);remote([1,0,0]);remote([4,1,32])
    dev.close()
    (args.output/'sustained-performance.json').write_text(json.dumps(samples,indent=2)+'\n')
print('Sustained two-voice hardware test passed')
