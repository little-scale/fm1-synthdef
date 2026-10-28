#!/usr/bin/env python3
"""Exercise the actual panel adapter against an isolated bank; never edit the user's bank."""
from pathlib import Path
import os
import subprocess
import tempfile
from bank import Bank,ROOT,pack
workspace=ROOT.parent
env=dict(os.environ,PATH=str(workspace/'.venv/bin')+os.pathsep+os.environ['PATH'],PYTHON='python3',X0X_FONTSET='terminus')
subprocess.run([str(workspace/'.venv/bin/python3'),str(workspace/'fm1-base/tools/build.py'),str(ROOT/'firmware'),'--host','--no-ble'],env=env,check=True)
with tempfile.TemporaryDirectory(prefix='fm1-panel-') as tmp:
    path=Path(tmp)/'bank.flash';bank=Bank(path)
    bank.put(0,pack('Keys + comb',(ROOT/'examples/keys.scd').read_text(),(ROOT/'examples/keys.scsyndef').read_bytes()),0)
    bank.put(8,pack('Master echo',(ROOT/'examples/master-echo.scd').read_text(),(ROOT/'examples/master-echo.scsyndef').read_bytes(),True),0)
    output=ROOT/'build/panel';output.mkdir(exist_ok=True)
    subprocess.run([str(ROOT/'firmware/build/host/synthdef_host'),str(ROOT/'firmware/tests/panel.txt'),str(output)],env=dict(env,FMS_BANK=str(path)),check=True)
