#!/usr/bin/env python3
"""New UGen integration through the real panel adapter and a temporary bank."""
import array, json, os, subprocess, tempfile, wave
from pathlib import Path
from bank import Bank, ROOT, compile_source, pack
workspace=ROOT.parent
env=dict(os.environ,PATH=str(workspace/'.venv/bin')+os.pathsep+os.environ['PATH'],PYTHON='python3',X0X_FONTSET='terminus')
subprocess.run([str(workspace/'.venv/bin/python3'),str(workspace/'fm1-base/tools/build.py'),str(ROOT/'firmware'),'--host','--no-ble'],env=env,check=True)
source=(ROOT/'examples/dust-chimes.scd').read_text()
binary,info=compile_source(source)
clock=r'''SynthDef(\triggerLab, {arg freq=440, amp=0.5, gate=1, knob1=0.4, knob2=0.3;
    var clock=Impulse.kr(2+knob1*20), ratio=TRand.kr(-12,12,clock).midiratio;
    var index=TExpRand.kr(0.1,2,clock), hold=Gate.kr(Latch.kr(knob2,clock),clock);
    var signal=SinOsc.ar(freq*ratio,SinOsc.ar(freq*2)*index);
    var env=EnvGen.kr(Env.asr(0.01,1,0.2),gate,doneAction:2);
    var hit=Decay.kr(clock,0.15)+Decay2.kr(clock,0.005,0.2);
    var dust=Decay.ar(Dust2.ar(8),0.015)*0.03;
    signal=LPZ1.ar(OneZero.ar(signal,-0.1));
    signal=signal+HPZ1.ar(Delay2.ar(Delay1.ar(signal)))*0.1;
    Out.ar(0,Pan2.ar((signal*hit*(0.1+hold*0.1)+dust)*env*amp*0.2));
})'''
clock_binary,clock_info=compile_source(clock)
output=ROOT/'build/extensions-panel';output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='fm1-extensions-') as tmp:
    bank=Bank(Path(tmp)/'bank.flash')
    bank.put(0,pack('Dust chimes',source,binary),0)
    bank.put(1,pack('Trigger lab',clock,clock_binary),0)
    bank.put(8,pack('Master echo',(ROOT/'examples/master-echo.scd').read_text(),(ROOT/'examples/master-echo.scsyndef').read_bytes(),True),0)
    run=subprocess.run([str(ROOT/'firmware/build/host/synthdef_host'),str(ROOT/'firmware/tests/extensions.txt'),str(output)],env=dict(env,FMS_BANK=str(bank.path)),capture_output=True,text=True)
    (output/'panel.log').write_text(run.stdout+run.stderr)
    print(run.stdout,run.stderr)
    assert run.returncode==0,'Panel assertions failed'
with wave.open(str(output/'extension-voices.wav')) as wav:
    assert wav.getsampwidth()==2 and wav.getnchannels()==2
    samples=array.array('h',wav.readframes(wav.getnframes()))
peaks=[max(abs(x) for x in samples[c::2])/32768 for c in (0,1)]
assert min(peaks)>.001 and max(peaks)<.99
result=dict(assertions=(run.stdout+run.stderr).count('  ok '),stereo_peaks=peaks,dust_nodes=info['nodes'],trigger_nodes=clock_info['nodes'],hardware_flashed=False)
(output/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
