#!/usr/bin/env python3
"""Render a supported SynthDef to a stereo PCM WAV, without opening audio hardware."""
import argparse
import array
import math
from pathlib import Path
import subprocess
import tempfile
import wave
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('synthdef',type=Path)
p.add_argument('output',type=Path)
p.add_argument('--seconds',type=float,default=1)
p.add_argument('--release',type=float,default=0.25,help='Set gate=0 at this time; block-quantized')
a=p.parse_args()
if not math.isfinite(a.seconds) or not 0<a.seconds<=60 or not math.isfinite(a.release) or a.release<0:
    p.error('Choose 0 < seconds <= 60 and release >= 0')
with tempfile.TemporaryDirectory(prefix='fm-synth-') as tmp:
    raw=Path(tmp)/'audio.f32'
    subprocess.run([str(ROOT/'build/fm-render'),str(a.synthdef),str(raw),str(math.ceil(a.seconds*44100/64)),str(int(a.release*44100)//64),'0'],check=True)
    floats=array.array('f');floats.frombytes(raw.read_bytes())
    pcm=array.array('h',(round(max(-1,min(1,v))*32767) for v in floats[:int(a.seconds*44100)*2]))
    import sys
    if sys.byteorder!='little':pcm.byteswap()
    with wave.open(str(a.output),'wb') as w:
        w.setparams((2,2,44100,0,'NONE','not compressed'));w.writeframes(pcm.tobytes())
print(a.output)
