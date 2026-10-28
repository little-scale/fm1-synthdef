#!/usr/bin/env python3
"""Exercise latching FX focus, independent controls and all effect switches."""
from pathlib import Path
import os,subprocess,tempfile,json
from bank import ROOT,Bank,pack,unpack
workspace=ROOT.parent
env=dict(os.environ,PATH=str(workspace/'.venv/bin')+os.pathsep+os.environ['PATH'],PYTHON='python3',X0X_FONTSET='terminus')
subprocess.run([str(workspace/'.venv/bin/python3'),str(workspace/'fm1-base/tools/build.py'),str(ROOT/'firmware'),'--host','--no-ble'],env=env,check=True)
lines=['wait 600','expect slot 0','expect fx_mode 0','expect fx_choice 1','expect effect 1','turn K1 10','wait 40','expect knob1 70','key 7 down 90','key 11 down 80','wait 100','expect held 2','wav switches.wav','press FX','wait 500','expect fx_mode 1','expect view 8','expect held 2','release FX','wait 100','expect fx_mode 1','turn K1 10','turn K3 10','wait 80','expect knob5 45','expect knob7 70','expect knob1 70','shot fx-controls.png','tap FX','wait 80','expect fx_mode 0','expect view 0','expect knob1 70','expect knob5 45','expect knob7 70','shot synth-controls.png']
for choice in [2,3,4,0,1]:
 lines+=['turn ALGO 1','wait 180',f'expect fx_choice {choice}',f'expect effect {int(choice!=0)}','expect held 2','expect slot 0','expect fx_mode 0','expect faults 0']
lines+=['tap FX','wait 80','expect fx_mode 1','turn K3 10','wait 80','expect knob7 70','turn PRESET 1','wait 100','expect slot 1','expect view 8','expect fx_mode 1','expect fx_choice 1','expect knob7 70','expect effect 1','key 7 up','key 11 up','wait 60','tap FX','wait 80','expect fx_mode 0','expect view 1','turn ALGO -1','wait 100','expect fx_choice 0','expect effect 0','turn PRESET -2','wait 80','expect slot 7','expect loaded 0','turn ALGO 1','wait 100','expect fx_choice 1','expect effect 1','tap FX','wait 80','expect view 8','turn SELECT 1','wait 80','expect scroll 1','expect faults 0','wavstop']
output=ROOT/'build/fx-banks-panel';output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='fm1-fx-banks-') as temp:
 temp=Path(temp);bank=Bank(temp/'bank.flash')
 for i,name in [(0,'keys-master'),(1,'two-op-fm'),(8,'master-echo'),(9,'master-freeverb'),(10,'master-freeverb2'),(11,'master-miverb')]:
  payload=(ROOT/'examples'/(name+'.fmslot')).read_bytes()
  if i==8:
   title,source,binary=unpack(payload);payload=pack(title,source+'\n'+('// Scrolling fixture\n'*18),binary,True)
  bank.put(i,payload,bank.get(i)[1])
 script=temp/'test.txt';script.write_text('\n'.join(lines)+'\n')
 run=subprocess.run([str(ROOT/'firmware/build/host/synthdef_host'),str(script),str(output)],env=dict(env,FMS_BANK=str(bank.path)),capture_output=True,text=True)
 (output/'run.log').write_text(run.stdout+run.stderr);print(run.stdout,run.stderr);run.check_returncode()
 result=dict(assertions=sum(x.startswith('expect ') for x in lines),effects=4,bypass=True,fx_latching=True,independent_knobs=True)
 (output/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(result)
