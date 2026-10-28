#!/usr/bin/env python3
"""Hardware acceptance: requires the test patches already in slots 1 and 2."""
import argparse,json,subprocess,time
from pathlib import Path
import mido
from device import Device
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'logs/synthdef-polyphony-2026-10-28')
OUT=parser.parse_args().output;OUT.mkdir(parents=True,exist_ok=True)
dev=Device();samples=[]
def remote(data):dev.out.send(mido.Message('sysex',data=[0x7d,0x4c,0x43]+data));time.sleep(.08)
def turn(which,value):remote([3,which,64+value])
def note(key,on):dev.out.send(mido.Message('note_on' if on else 'note_off',note=key,velocity=80 if on else 0))
def sample(label):
 s=dev.stats();s['phase']=label;samples.append(s);print(json.dumps(s),flush=True)
rec=None
try:
 initial=dev.stats();turn(2,-initial.get('view_slot_index',initial['slot_index']));remote([4,0,8]) # MASTER 1024 / 4096, temporary
 with (OUT/'capture.log').open('w') as log:
  rec=subprocess.Popen(['ffmpeg','-hide_banner','-nostdin','-f','avfoundation','-i',':SynthDef FM-1','-t','16','-ar','44100','-ac','2','-c:a','pcm_s16le','-y',str(OUT/'hardware-audio.wav')],stdout=log,stderr=subprocess.STDOUT)
  time.sleep(2);sample('keys idle');note(60,True)
  for i in range(4):time.sleep(.25);sample('keys one voice')
  note(67,True)
  for i in range(4):time.sleep(.25);sample('keys two voices')
  note(72,True);time.sleep(.2);sample('keys third note steals oldest');note(60,False);time.sleep(.1);sample('keys stolen note-off')
  turn(3,20);time.sleep(.3);sample('keys live knob');turn(3,-20)
  note(60,False);note(67,False);note(72,False);time.sleep(.7);sample('keys released')
  turn(2,1);sample('addy idle')
  for key in (60,64,67):
   note(key,True);note(key+7,True);time.sleep(.15);sample('addy two voices');time.sleep(.3);sample('addy decay');note(key,False);note(key+7,False);time.sleep(.7)
  turn(2,-1);sample('returned to keys')
  rec.wait(timeout=25)
  if rec.returncode:raise RuntimeError('Audio capture failed; inspect capture.log')
finally:
 for key in (60,64,67,71,72,74):note(key,False)
 remote([1,64,0]);remote([1,0,0]);remote([4,1,32]) # HOME and physical MASTER restored
 dev.close()
 if rec and rec.poll() is None:rec.terminate()
 (OUT/'performance.json').write_text(json.dumps(samples,indent=2)+'\n')
assert samples and all(s['loaded'] and not s['faults'] and s['overloads']==samples[0]['overloads'] for s in samples),'Device fault or overload during test'
assert samples[-1]['xruns']==samples[0]['xruns'],'Audio overrun during playback'
assert all(s['active_voices']==2 for s in samples if s['phase'] in ('keys two voices','keys third note steals oldest','keys stolen note-off','addy two voices')),'Polyphony was not sustained'
