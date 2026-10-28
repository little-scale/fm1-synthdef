#!/usr/bin/env python3
"""Exercise all three slot-0 reverbs with two voices in isolated simulator banks."""
import array,json,math,os,subprocess,tempfile,wave
from pathlib import Path
from bank import Bank,ROOT,compile_source,pack
workspace=ROOT.parent
env=dict(os.environ,PATH=str(workspace/'.venv/bin')+os.pathsep+os.environ['PATH'],PYTHON='python3',X0X_FONTSET='terminus')
subprocess.run([str(workspace/'.venv/bin/python3'),str(workspace/'fm1-base/tools/build.py'),str(ROOT/'firmware'),'--host','--no-ble'],env=env,check=True)
results=[]
for verb in ['freeverb','freeverb2','miverb']:
    path=ROOT/'examples'/('master-'+verb+'.scd');source=path.read_text();binary,info=compile_source(source,True)
    path.with_suffix('.scsyndef').write_bytes(binary);path.with_suffix('.fmslot').write_bytes(pack('Master '+verb,source,binary,True))
    for synth in ['keys-master','dust-chimes']:
        output=ROOT/'build/reverb-panel'/verb/synth;output.mkdir(parents=True,exist_ok=True)
        voice_source=(ROOT/'examples'/(synth+'.scd')).read_text();voice_binary,voice_info=compile_source(voice_source)
        with tempfile.TemporaryDirectory(prefix='fm1-reverbs-') as tmp:
            bank=Bank(Path(tmp)/'bank.flash');bank.put(0,pack(synth,voice_source,voice_binary),0);bank.put(8,pack('Master '+verb,source,binary,True),0)
            run=subprocess.run([str(ROOT/'firmware/build/host/synthdef_host'),str(ROOT/'firmware/tests/reverbs.txt'),str(output)],env=dict(env,FMS_BANK=str(bank.path)),capture_output=True,text=True)
            log=run.stdout+run.stderr;(output/'panel.log').write_text(log);assert run.returncode==0,log
        with wave.open(str(output/'reverb-voices.wav')) as wav:
            assert wav.getsampwidth()==2 and wav.getnchannels()==2;rate=wav.getframerate();samples=array.array('h',wav.readframes(wav.getnframes()))
        peaks=[max(abs(x) for x in samples[c::2])/32768 for c in (0,1)];assert min(peaks)>.001 and max(peaks)<.99
        # Last second is after the gated synth release, while the effect runs.
        tail=samples[-rate*2:];rms=math.sqrt(sum((x/32768)**2 for x in tail)/len(tail));assert rms>1e-6,(verb,synth,'No tail',rms)
        result=dict(effect=verb,synth=synth,assertions=log.count('  ok '),stereo_peaks=peaks,tail_rms=rms,effect_memory_bytes=info['delay_bytes'])
        results.append(result);print(json.dumps(result),flush=True)
(ROOT/'build/reverb-panel/verification.json').write_text(json.dumps(results,indent=2)+'\n')
