#!/usr/bin/env python3
"""Dusty: two voices, automatic retirement, stealing, and all master effects."""
import array,json,os,subprocess,tempfile,wave
from pathlib import Path
from bank import Bank,ROOT
workspace=ROOT.parent
env=dict(os.environ,PATH=str(workspace/'.venv/bin')+os.pathsep+os.environ['PATH'],PYTHON='python3',X0X_FONTSET='terminus')
subprocess.run([str(workspace/'.venv/bin/python3'),str(workspace/'fm1-base/tools/build.py'),str(ROOT/'firmware'),'--host','--no-ble'],env=env,check=True)
out=ROOT/'build/dusty-panel';out.mkdir(exist_ok=True)
lines=['wait 600','turn PRESET 4','wait 120','expect slot 4','expect loaded 1','turn ALGO -1','wait 150','wav dusty-effects.wav']
for fx in range(5):
 if fx:lines+=['turn ALGO 1','wait 150']
 lines += [f'expect fx_choice {fx}',f'expect effect {int(fx!=0)}','key 7 down 64','key 11 down 64','wait 500','expect active 2','expect faults 0','key 7 up','key 11 up','wait 150','expect active 2','wait 2700','expect active 0','expect faults 0']
lines+=['key 7 down 64','key 11 down 64','wait 150','key 15 down 64','wait 150','expect active 2','key 7 up','wait 150','expect active 2','key 11 up','key 15 up','wait 3100','expect active 0','expect faults 0','shot dusty.png','wavstop','expect peak_db_min -40']
with tempfile.TemporaryDirectory(prefix='fm1-dusty-') as temp:
 bank=Bank(Path(temp)/'bank.flash')
 for i,name in [(4,'dusty'),(8,'master-echo'),(9,'master-freeverb'),(10,'master-freeverb2'),(11,'master-miverb')]:bank.put(i,(ROOT/'examples'/(name+'.fmslot')).read_bytes(),bank.get(i)[1])
 script=Path(temp)/'panel.txt';script.write_text('\n'.join(lines)+'\n')
 run=subprocess.run([str(ROOT/'firmware/build/host/synthdef_host'),str(script),str(out)],env=dict(env,FMS_BANK=str(bank.path)),capture_output=True,text=True)
 (out/'run.log').write_text(run.stdout+run.stderr);print(run.stdout,run.stderr);run.check_returncode()
with wave.open(str(out/'dusty-effects.wav')) as w:
 pcm=array.array('h',w.readframes(w.getnframes()));assert w.getnchannels()==2
peaks=[max(abs(v) for v in pcm[c::2])/32768 for c in (0,1)];assert min(peaks)>.001
result=dict(assertions=sum(x.startswith('expect ') for x in lines),voices=2,effects=4,bypass=True,stereo_peaks=peaks)
(out/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(result)
