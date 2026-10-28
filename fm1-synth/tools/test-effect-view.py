#!/usr/bin/env python3
"""Verify that the latching FX code view leaves held notes and synth selection intact."""
import argparse,json,time
from pathlib import Path
import mido
from device import Device
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);out=p.parse_args().output
dev=Device();samples=[]
def remote(data):dev.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43]+data));time.sleep(.08)
def sample(label):
    s=dev.stats();s['phase']=label;samples.append(s);print(json.dumps(s),flush=True);return s
try:
    state=dev.stats();remote([3,2,64-state['slot_index']]);remote([4,0,8])
    remote([3,1,65-dev.panel()['fx_choice']])
    if dev.panel()['fx_mode']:remote([1,1,0]);remote([1,0,0])
    for key in (60,67):dev.out.send(mido.Message('note_on',note=key,velocity=75))
    time.sleep(.15);before=sample('two held notes')
    assert before['active_voices']==2 and before['effect_active']
    for i in range(4):
        remote([1,1,0]);remote([1,0,0]);s=sample('view effect code')
        assert s['view_slot_index']==8 and s['slot_index']==0 and s['active_voices']==2 and s['notes_started']==before['notes_started']
        remote([1,1,0]);remote([1,0,0]);s=sample('return to synth code')
        assert s['view_slot_index']==0 and s['slot_index']==0 and s['active_voices']==2 and s['notes_started']==before['notes_started']
    assert all(s['effect_active'] and not s['faults'] and s['overloads']==before['overloads'] and s['xruns']==before['xruns'] for s in samples)
finally:
    for key in (60,67):dev.out.send(mido.Message('note_off',note=key,velocity=0))
    remote([1,64,0]);remote([1,0,0]);remote([4,1,32]);dev.close()
    out.mkdir(parents=True,exist_ok=True);(out/'effect-view.json').write_text(json.dumps(samples,indent=2)+'\n')
print('Effect code browsing preserves both held notes and the selected synth')
