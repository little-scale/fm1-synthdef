#!/usr/bin/env python3
"""Verify the latching FX bank on hardware without changing saved patches.

Requires Keys in slot 1 and four populated FX presets. Keep keys untouched.
Sets MASTER low during the test, then restores physical control. Ends at synth 1,
FX1, FX mode off. Saves telemetry and a stereo USB recording.
"""
import argparse,array,hashlib,json,subprocess,time,wave
from pathlib import Path
import mido
from device import Device
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
out=a.output;out.mkdir(parents=True,exist_ok=True)
dev=Device();assert dev.info[:2]==bytes([3,12]),'Install the four-FX firmware first'
rows=[];capture=None;baseline=None;original={};source0=dev.inputs();expected_midi=source0['midi_note_events']
def remote(data,wait=.08):
 dev.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43]+data));time.sleep(wait)
def tap(bit):remote([1,1<<bit,0]);remote([1,0,0])
def home():tap(6)
def mode(value):
 if dev.panel()['fx_mode']!=value:tap(0)
def select(index):remote([3,2,64+index-dev.stats()['slot_index']],.15)
def effect(choice):remote([3,1,64+choice-dev.panel()['fx_choice']],.18)
def turn(knob,steps):
 panel=dev.panel();index=knob+(4 if panel['fx_mode'] else 0)
 expected=max(0,min(100,panel['knobs'][index]+2*steps))
 remote([3,3+knob,64+steps]);deadline=time.monotonic()+1
 while True:
  actual=dev.panel()
  if actual['knobs'][index]==expected:return
  if time.monotonic()>deadline:raise AssertionError(('Encoder command not applied',index,expected,actual))
  time.sleep(.02)
def note(key,on):
 global expected_midi
 dev.out.send(mido.Message('note_on' if on else 'note_off',note=key,velocity=75 if on else 0))
 if on:expected_midi+=1
 time.sleep(.025)
def check(label,voices=None,fx=None,fxmode=None,slot=0):
 state=dev.stats();panel=dev.panel();inputs=dev.inputs();row=dict(label=label,stats=state,panel=panel,inputs=inputs);rows.append(row)
 assert inputs['key_note_events']==source0['key_note_events'],('Unexpected physical key event',row)
 assert inputs['midi_note_events']==expected_midi,('Unexpected MIDI event',row)
 assert state['slot_index']==slot and state['loaded'] and state['faults']==0,row
 if baseline:
  assert state['overloads']==baseline['overloads'] and state['xruns']==baseline['xruns'],row
 if voices is not None:assert state['active_voices']==voices,row
 if fx is not None:assert panel['fx_choice']==fx and state['effect_active']==int(fx!=0),row
 if fxmode is not None:assert panel['fx_mode']==fxmode,row
 assert panel['switching']==0 and panel['gain']==1000,row
 expected_view=(7+panel['fx_choice'] if panel['fx_choice'] else 12) if panel['fx_mode'] else slot
 assert state['view_slot_index']==expected_view,row
 print(label+' passed',flush=True);return panel
try:
 for i in range(12):original[i]=dev.read(i)[0]
 assert original[0] and all(original[i] for i in range(8,12)),'Expected Keys and four effects'
 remote([4,0,12]);home();mode(0);select(0);effect(1);home();baseline=dev.stats()
 # Use deterministic levels; encoders are relative, .02 per detent.
 turn(0,-50);turn(0,25);turn(1,-50);turn(1,15)
 before=check('synth controls',0,1,0);assert before['knobs'][:2]==[50,30],before
 with (out/'capture.log').open('w') as log:
  capture=subprocess.Popen(['ffmpeg','-hide_banner','-f','avfoundation','-i',':SynthDef FM-1','-ar','44100','-ac','2','-c:a','pcm_s16le','-y',str(out/'switches.wav')],stdin=subprocess.PIPE,stdout=log,stderr=log,text=True)
  time.sleep(2);assert capture.poll() is None,'USB audio capture failed'
  note(60,True);note(67,True);time.sleep(.2);check('held chord',2,1,0)
  remote([1,1,0],.6);check('FX remains latched once while held',2,1,1)
  remote([1,0,0]);check('FX stays on after release',2,1,1)
  turn(0,-50);turn(0,20);turn(1,-50);turn(1,20);turn(2,-50);turn(2,35)
  on=check('effect controls independent',2,1,1);assert on['knobs'][:4]==before['knobs'][:4] and on['knobs'][4:7]==[40,40,70],on
  tap(0);off=check('FX toggle returns to synth',2,1,0);assert off['knobs']==on['knobs'],off
  turn(0,5);changed=check('synth knob leaves effect unchanged',2,1,0);assert changed['knobs'][0]==60 and changed['knobs'][4:]==on['knobs'][4:],changed
  for _ in range(2):
   for fx in [2,3,4,0,1]:
    effect(fx);check('held chord through FX'+str(fx),2,fx,0);time.sleep(.3)
  tap(0);effect(3);check('algorithm updates effect code with notes held',2,3,1)
  note(60,False);note(67,False);time.sleep(.6);check('release in FX mode',0,3,1)
  # Change synth while keeping even an unreferenced effect control.
  turn(2,-50);turn(2,35);saved=dev.panel()['knobs'][4:]
  select(3);check('PRESET selects FM while FX stays focused',0,3,1,slot=3);assert dev.panel()['knobs'][4:]==saved
  select(0);check('PRESET returns to Keys without changing FX',0,3,1);assert dev.panel()['knobs'][4:]==saved
  # A note-off sent during a requested switch must not become stuck.
  for fx in [4,0,1,2,3,1]:
   note(60,True);note(67,True);time.sleep(.12)
   dev.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43,3,1,64+fx-dev.panel()['fx_choice']]))
   note(60,False);note(67,False);time.sleep(.7);check('note-off across switch to FX'+str(fx),0,fx,1)
  tap(0);home();check('ready to play synth 1 with FX1',0,1,0)
  capture.communicate('q\n',timeout=10);assert capture.returncode==0;capture=None
 for i,data in original.items():assert dev.read(i)[0]==data,('Saved slot changed',i)
 with wave.open(str(out/'switches.wav')) as wav:
  assert wav.getnchannels()==2 and wav.getsampwidth()==2
  pcm=array.array('h',wav.readframes(wav.getnframes()));rate=wav.getframerate()
 peaks=[max(abs(x) for x in pcm[c::2])/32768 for c in (0,1)];assert min(peaks)>.0001 and max(peaks)<.99,peaks
 result=dict(checks=len(rows),preserved_slots=12,effects=4,bypass=True,fx_latching=True,independent_knobs=True,voices=2,stereo_peaks=peaks,recorded_seconds=len(pcm)/(2*rate),new_xruns=rows[-1]['stats']['xruns']-baseline['xruns'],new_overloads=rows[-1]['stats']['overloads']-baseline['overloads'],faults=rows[-1]['stats']['faults'],sampled_render_max_us=max(r['stats']['render_last_us'] for r in rows),slot_sha256={str(i):hashlib.sha256(data).hexdigest() for i,data in original.items()})
 (out/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
finally:
 try:
  dev.out.send(mido.Message('control_change',control=123,value=0));home();effect(1);mode(0);select(0);remote([4,1,32])
 finally:
  dev.close()
  if capture and capture.poll() is None:
   try:capture.communicate('q\n',timeout=10)
   except subprocess.TimeoutExpired:capture.terminate()
  (out/'telemetry.json').write_text(json.dumps(rows,indent=2)+'\n')
