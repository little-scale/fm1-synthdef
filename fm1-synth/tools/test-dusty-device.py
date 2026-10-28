#!/usr/bin/env python3
"""Test installed Dusty in slot 5 with two voices and each master effect.

No slot writes. Low MASTER during test; restores physical MASTER and prior FX.
Leaves Dusty selected, synth knobs focused. Keep physical keys untouched.
"""
import argparse,array,hashlib,json,subprocess,time,wave
from pathlib import Path
import mido
from device import Device
from bank import ROOT
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output;out.mkdir(parents=True,exist_ok=True)
dev=Device();initial=dev.panel();before={i:dev.read(i)[0] for i in range(dev.info[1])};assert before[4]==(ROOT/'examples/dusty.fmslot').read_bytes(),'Expected Dusty in slot 5'
rows=[];capture=None

def remote(data):dev.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43]+data));time.sleep(.1)
def home():remote([1,64,0]);remote([1,0,0])
def effect(fx):remote([3,1,64+fx-dev.panel()['fx_choice']]);time.sleep(.15)
def knob(index,value):
 for attempt in range(3):
  current=dev.panel()['knobs'][index]
  if current==value:return
  assert (value-current)%2==0
  remote([3,3+index,64+(value-current)//2])
  deadline=time.monotonic()+.4
  while time.monotonic()<deadline:
   if dev.panel()['knobs'][index]==value:return
   time.sleep(.02)
 raise AssertionError('Knob command not applied after readback retries')
def notes(keys,on):
 for key in keys:dev.out.send(mido.Message('note_on' if on else 'note_off',note=key,velocity=80 if on else 0))
def check(label,voices,fx):
 s=dev.stats();inputs=dev.inputs();rows.append(dict(label=label,stats=s,inputs=inputs));assert s['slot_index']==4 and s['loaded'],s
 assert s['active_voices']==voices and not s['faults'],s
 assert s['overloads']==baseline['overloads'] and s['xruns']==baseline['xruns'],s
 assert inputs['key_note_events']==input0['key_note_events'],('Unexpected physical note',inputs)
 assert s['effect_active']==int(fx!=0),s
 print(label+' passed',flush=True)
try:
 remote([4,0,10]);home()
 if dev.panel()['fx_mode']:remote([1,1,0]);remote([1,0,0])
 remote([3,2,68-dev.stats()['slot_index']]);effect(0);home();knob(0,20);knob(1,50);baseline=dev.stats();input0=dev.inputs()
 with (out/'capture.log').open('w') as log:
  capture=subprocess.Popen(['ffmpeg','-hide_banner','-f','avfoundation','-i',':SynthDef FM-1','-ar','44100','-ac','2','-c:a','pcm_s16le','-y',str(out/'dusty-device.wav')],stdin=subprocess.PIPE,stdout=log,stderr=log,text=True)
  time.sleep(2);assert capture.poll() is None
  for fx in range(5):
   effect(fx);home();notes([60,67],True);time.sleep(.4);check(f'FX{fx} two voices',2,fx)
   knob(0,100);knob(1,100);check(f'FX{fx} live maximum knobs',2,fx)
   knob(0,0);knob(1,0);check(f'FX{fx} live minimum knobs',2,fx)
   knob(0,20);knob(1,50)
   notes([60,67],False);time.sleep(.15);check(f'FX{fx} percussion continues after key release',2,fx)
   time.sleep(4.8);check(f'FX{fx} envelope frees voices',0,fx)
  effect(1);home();notes([60,67],True);time.sleep(.15);notes([72],True);time.sleep(.15);check('third note steals oldest',2,1)
  notes([60],False);time.sleep(.15);check('stolen note-off preserves newer voice',2,1)
  notes([67,72],False);time.sleep(5.15);check('all voices retire',0,1)
  capture.communicate('q\n',timeout=10);assert capture.returncode==0;capture=None
 for i,data in before.items():assert dev.read(i)[0]==data,('Slot changed',i)
 with wave.open(str(out/'dusty-device.wav')) as w:pcm=array.array('h',w.readframes(w.getnframes()));rate=w.getframerate();assert w.getnchannels()==2
 peaks=[max(abs(x) for x in pcm[c::2])/32768 for c in (0,1)];assert min(peaks)>.0001 and max(peaks)<.99,peaks
 assert any(l!=r for l,r in zip(pcm[::2],pcm[1::2])),'Stereo spread missing'
 result=dict(checks=len(rows),voices=2,effects=4,bypass=True,stereo_peaks=peaks,overloads_at_baseline=baseline['overloads'],new_overloads=rows[-1]['stats']['overloads']-baseline['overloads'],new_xruns=rows[-1]['stats']['xruns']-baseline['xruns'],faults=0,sampled_render_max_us=max(r['stats']['render_last_us'] for r in rows),preserved_slots=len(before),patch_sha256=hashlib.sha256(before[4]).hexdigest())
 (out/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
finally:
 try:
  dev.out.send(mido.Message('control_change',control=123,value=0));home();knob(0,20);knob(1,50);effect(initial['fx_choice'])
 finally:
  try:remote([4,1,32])
  finally:dev.close()
  if capture and capture.poll() is None:
   try:capture.communicate('q\n',timeout=10)
   except subprocess.TimeoutExpired:capture.terminate()
  (out/'telemetry.json').write_text(json.dumps(rows,indent=2)+'\n')
