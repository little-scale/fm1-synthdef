#!/usr/bin/env python3
"""Hardware reverb matrix. Temporarily replaces slot 0 and slot 8; restores both.

Plays two-note chords at low MASTER, records USB audio, tests live controls,
stealing and release. Requires the four supplied synths in slots 1-4.
"""
import argparse,array,hashlib,json,math,subprocess,time,wave
from pathlib import Path
import mido
from bank import ROOT,compile_source,pack,unpack,validate
from device import Device
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--skip-addy',action='store_true',help='Test lighter patches after a separately recorded Addy overload')
args=parser.parse_args();out=args.output;out.mkdir(parents=True,exist_ok=True)
dev=Device();input_baseline=dev.inputs();expected_midi=[input_baseline["midi_note_events"]];original_stats=dev.stats();original={};samples=[];results=[];capture=None;restore=[]
def remote(data):
 dev.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43]+data));time.sleep(.04)
def home():remote([1,64,0]);remote([1,0,0])
def select(index):
 state=dev.stats();remote([3,2,64+index-state['view_slot_index']]);time.sleep(.08)
def note(key,on):
 dev.out.send(mido.Message('note_on' if on else 'note_off',note=key,velocity=80 if on else 0))
 if on:expected_midi[0]+=1
def controls(values):
 for k,value in enumerate(values):
  remote([3,3+k,14]);steps=round(50*value)
  if steps:remote([3,3+k,64+steps])
def check(label,slot,voices,baseline):
 state=dev.stats();sources=dev.inputs();state.update(phase=label,elapsed=time.monotonic()-t0,input_sources=sources);samples.append(state)
 assert sources['key_note_events']==input_baseline['key_note_events'],('Unexpected keyboard note',state)
 assert sources['midi_note_events']==expected_midi[0],('Unexpected MIDI note count',expected_midi[0],state)
 assert state['slot_index']==slot and state['loaded'] and state['effect_active'],state
 assert state['active_voices']==voices and not state['faults'],state
 assert state['overloads']==baseline['overloads'] and state['xruns']==baseline['xruns'],state
 return state
try:
 for index in range(9):
  data,revision=dev.read(index);original[index]=data
  destination=out/f'pretest-slot-{index}.fmslot'
  if destination.exists():assert destination.read_bytes()==data,'Preserve previous snapshot; choose another output directory'
  else:destination.write_bytes(data)
  if index<4:assert data,'Expected supplied synth slots 1-4'
 source=(ROOT/'examples/dust-chimes.scd').read_text();binary,info=compile_source(source)
 dust=pack('Dust chimes test',source,binary)
 remote([4,0,16]);home();dev.write(7,dust)
 for verb in ['freeverb','freeverb2','miverb']:
  home();select(0);dev.write(8,(ROOT/'examples'/('master-'+verb+'.fmslot')).read_bytes());time.sleep(.15);home()
  baseline=dev.stats();start=len(samples);t0=time.monotonic()
  with (out/(verb+'-capture.log')).open('w') as log:
   capture=subprocess.Popen(['ffmpeg','-hide_banner','-f','avfoundation','-i',':SynthDef FM-1','-ar','44100','-ac','2','-c:a','pcm_s16le','-y',str(out/(verb+'-device.wav'))],stdin=subprocess.PIPE,stdout=log,stderr=log,text=True)
   time.sleep(2)
   assert capture.poll() is None,'USB capture failed'
   for slot in ([0,2,3,7] if args.skip_addy else [0,1,2,3,7]):
    data=dust if slot==7 else original[slot];name,_,binary=unpack(data);_,info=validate(binary)
    for setting in ['default','minimum','maximum']:
     select(slot);home()
     defaults=[info['controls'].get('knob'+str(i+1),[.5,.5,.3,.5][i]) for i in range(4)]
     controls(defaults if setting=='default' else [0 if setting=='minimum' else 1]*4)
     note(60,True);note(67,True);time.sleep(.13)
     label=verb+' / '+name+' / '+setting
     check(label+' chord',slot,2,baseline)
     remote([3,5,69]);remote([3,6,59]);check(label+' live controls',slot,2,baseline)
     note(72,True);time.sleep(.05);check(label+' third-note steal',slot,2,baseline)
     note(60,False);time.sleep(.03);check(label+' stolen note-off',slot,2,baseline)
     note(67,False);note(72,False)
     time.sleep(3.25 if slot==2 and setting=='maximum' else 1.4)
     check(label+' released',slot,0,baseline)
     print(label+' passed',flush=True)
   # Finish the matrix recording before any further slot change. USB capture
   # may omit frames during patch loads, so its wall-clock alignment is not
   # valid for a late tail measurement. Use a separate, uninterrupted take.
   capture.communicate('q\n',timeout=10);assert capture.returncode==0;capture=None
   select(0);home();controls([.5,.3,1,1])
   with (out/(verb+'-tail-capture.log')).open('w') as tail_log:
    capture=subprocess.Popen(['ffmpeg','-hide_banner','-f','avfoundation','-i',':SynthDef FM-1','-ar','44100','-ac','2','-c:a','pcm_s16le','-y',str(out/(verb+'-tail.wav'))],stdin=subprocess.PIPE,stdout=tail_log,stderr=tail_log,text=True)
    time.sleep(2);assert capture.poll() is None
    tail_on=time.monotonic();note(60,True);note(67,True);time.sleep(.15)
    note(60,False);note(67,False);tail_hold=time.monotonic()-tail_on;time.sleep(.65)
    check(verb+' tail only',0,0,baseline);time.sleep(.6)
    capture.communicate('q\n',timeout=10);assert capture.returncode==0;capture=None
   # Viewing the effect must keep the selected playable synth and held voices.
   note(60,True);note(67,True);time.sleep(.1);select(8);check(verb+' FX code view',0,2,baseline)
   home();check(verb+' panic',0,0,baseline);select(0)
  with wave.open(str(out/(verb+'-device.wav'))) as wav:
   rate=wav.getframerate();assert wav.getnchannels()==2 and wav.getsampwidth()==2
   pcm=array.array('h',wav.readframes(wav.getnframes()))
  peaks=[max(abs(x) for x in pcm[c::2])/32768 for c in (0,1)];assert min(peaks)>.0001 and max(peaks)<.99,peaks
  with wave.open(str(out/(verb+'-tail.wav'))) as wav:
   rate=wav.getframerate();assert wav.getnchannels()==2 and wav.getsampwidth()==2
   tail_pcm=array.array('h',wav.readframes(wav.getnframes()))
  first=next(i//2 for i,x in enumerate(tail_pcm) if abs(x)>2)
  tail=tail_pcm[int((first/rate+tail_hold+.7)*rate)*2:int((first/rate+tail_hold+1.1)*rate)*2]
  assert tail,'Missing tail recording'
  rms=math.sqrt(sum((x/32768)**2 for x in tail)/len(tail));assert rms>1e-6,(verb,'No measurable tail',rms)
  rows=samples[start:];result=dict(effect=verb,synth_slots=([1,3,4,8] if args.skip_addy else [1,2,3,4,8]),voices=2,checks=len(rows),stereo_peaks=peaks,tail_rms=rms,new_overloads=rows[-1]['overloads']-baseline['overloads'],new_xruns=rows[-1]['xruns']-baseline['xruns'],sampled_render_max_us=max(r['render_last_us'] for r in rows),tail_hold_seconds=tail_hold,tail_onset_sample=first)
  results.append(result);(out/'verification.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(result),flush=True)
finally:
 try:
  dev.out.send(mido.Message('control_change',control=123,value=0));home()
  # Reset only the two temporary test slots, verify every slot's content.
  for index in [7,8]:
   if index in original:
    current,_=dev.read(index)
    if current!=original[index]:dev.write(index,original[index])
  for index,data in original.items():
   actual,_=dev.read(index);restore.append(dict(index=index,restored=actual==data,sha256=hashlib.sha256(actual).hexdigest()))
  select(original_stats['slot_index']);home()
  if original_stats['view_slot_index']==8:select(8)
  remote([4,1,32])
 finally:
  dev.close()
  if capture and capture.poll() is None:
   try:capture.communicate('q\n',timeout=10)
   except subprocess.TimeoutExpired:capture.terminate()
  (out/'performance.json').write_text(json.dumps(samples,indent=2)+'\n')
  (out/'restoration.json').write_text(json.dumps(restore,indent=2)+'\n')
assert len(results)==3 and len(restore)==9 and all(r['restored'] for r in restore)
print('All three reverb matrices passed; original slots restored.')
