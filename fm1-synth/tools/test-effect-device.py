#!/usr/bin/env python3
"""Record dry/wet note releases and a preset switch through FX1.

Requires Keys in slot 1 and masterEcho in FX1. Temporarily changes effect knob 5
and MASTER. The effect package should be reloaded afterward to restore defaults.
"""
import argparse,array,json,math,subprocess,time,wave
from pathlib import Path
import mido
from device import Device
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
out=parser.parse_args().output;out.mkdir(parents=True,exist_ok=True)
dev=Device();baseline=dev.stats();events={};capture=None
def remote(data):dev.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43]+data));time.sleep(.08)
def home():remote([1,64,0]);remote([1,0,0])
def note(on):dev.out.send(mido.Message('note_on' if on else 'note_off',note=69,velocity=90 if on else 0))
try:
    remote([3,2,64-baseline['slot_index']]);remote([4,0,8]);remote([3,1,65-dev.panel()['fx_choice']]);home()
    if not dev.panel()['fx_mode']:remote([1,1,0]);remote([1,0,0])
    remote([3,3,14])
    with (out/'effect-capture.log').open('w') as log:
        capture=subprocess.Popen(['ffmpeg','-hide_banner','-nostdin','-f','avfoundation','-i',':SynthDef FM-1','-t','12','-ar','44100','-ac','2','-c:a','pcm_s16le','-y',str(out/'effect-tail.wav')],stdout=log,stderr=subprocess.STDOUT)
        t0=time.monotonic();time.sleep(2)
        events['dry_on']=time.monotonic()-t0;note(True);time.sleep(.15)
        events['dry_off']=time.monotonic()-t0;note(False);time.sleep(1.4)
        events['dry_status']=dev.stats()
        home();remote([3,3,114]);time.sleep(.3)
        events['wet_on']=time.monotonic()-t0;note(True);time.sleep(.15)
        events['wet_off']=time.monotonic()-t0;note(False);time.sleep(.45)
        remote([3,2,65]);events['preset_switch']=time.monotonic()-t0
        time.sleep(.65);events['wet_status']=dev.stats()
        remote([3,2,63]);home();capture.wait(timeout=20)
        assert capture.returncode==0,'USB recording failed'
finally:
    note(False);home()
    if dev.panel()['fx_mode']:remote([1,1,0]);remote([1,0,0])
    remote([4,1,32]);dev.close()
    if capture and capture.poll() is None:capture.terminate()
    (out/'effect-events.json').write_text(json.dumps(events,indent=2)+'\n')
with wave.open(str(out/'effect-tail.wav')) as wav:
    rate=wav.getframerate();samples=array.array('h',wav.readframes(wav.getnframes()))
first=next(i//2 for i,x in enumerate(samples) if abs(x)>2)
offset=first/rate-events['dry_on']
def rms(event):
    start=int((events[event]+offset+.55)*rate)*2;end=int((events[event]+offset+.9)*rate)*2
    segment=samples[start:end];assert segment
    return math.sqrt(sum(x*x for x in segment)/len(segment))/32768
result=dict(dry_tail_rms=rms('dry_off'),wet_tail_rms=rms('wet_off'),alignment_seconds=offset)
assert result['wet_tail_rms']>max(.00001,result['dry_tail_rms']*10),'No measurable permanent echo tail'
for key in ('dry_status','wet_status'):
    s=events[key]
    assert s['active_voices']==0 and not s['faults'] and s['overloads']==baseline['overloads'] and s['xruns']==baseline['xruns'],'Device playback fault'
(out/'effect-tail-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
