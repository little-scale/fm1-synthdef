#!/usr/bin/env python3
"""Battery states in the real host UI, with an isolated bank and explicit ADC fixtures."""
import os, subprocess, tempfile
from pathlib import Path
from bank import Bank, ROOT, pack
workspace=ROOT.parent
output=ROOT/'build/battery-panel';output.mkdir(exist_ok=True)
env=dict(os.environ,PATH=str(workspace/'.venv/bin')+os.pathsep+os.environ['PATH'],PYTHON='python3',X0X_FONTSET='terminus')
subprocess.run([str(workspace/'.venv/bin/python3'),str(workspace/'fm1-base/tools/build.py'),str(ROOT/'firmware'),'--host','--no-ble'],env=env,check=True)
with tempfile.TemporaryDirectory(prefix='fm1-battery-') as tmp:
    tmp=Path(tmp);path=tmp/'bank.flash';bank=Bank(path)
    for index,name,effect in [(0,'keys',False),(8,'master-echo',True)]:
        bank.put(index,pack(name,(ROOT/f'examples/{name}.scd').read_text(),(ROOT/f'examples/{name}.scsyndef').read_bytes(),effect),0)
    for name,raw,bars,usb in [('unknown',-1,-1,0),('empty',500,0,0),('one',545,1,0),('two',575,2,0),('full',610,3,0),('usb',610,3,1)]:
        script=tmp/f'{name}.txt'
        script.write_text(f'wait 600\nexpect battery {bars}\nexpect battery_raw {raw}\nexpect battery_usb {usb}\nshot {name}.png\nturn PRESET -1\nwait 80\nexpect view 8\nexpect slot 0\nshot {name}-fx.png\nkey 7 down 90\nwait 80\nexpect held 1\nkey 7 up\nwait 80\nexpect held 0\n')
        subprocess.run([str(ROOT/'firmware/build/host/synthdef_host'),str(script),str(output)],env=dict(env,FMS_BANK=str(path),FMS_BATTERY_RAW=str(raw),FMS_BATTERY_USB=str(usb)),check=True)
print('Battery UI: 6 states, 42 assertions; synth/FX screenshots saved.')
