#!/usr/bin/env python3
"""Compile Mix/Splay helpers and compare them with explicit supported graphs.

No device or user-bank changes. Needs the stock SC compiler; results go to build.
"""
import array,io,json,math,wave
from pathlib import Path
from bank import ROOT,compile_source,preview,pack
out=ROOT/'build/mix-splay';out.mkdir(parents=True,exist_ok=True)
results=[]
def pcm(binary):
 data,_=preview(binary)
 with wave.open(io.BytesIO(data)) as w:return array.array('h',w.readframes(w.getnframes()))
def compare(name,helper,explicit):
 a,info=compile_source('SynthDef(\\test,{Out.ar(0,'+helper+');})')
 b,_=compile_source('SynthDef(\\test,{Out.ar(0,'+explicit+');})')
 x,y=pcm(a),pcm(b);peak=max(abs(v-w) for v,w in zip(x,y))
 assert len(x)==len(y) and peak<=1,(name,peak)
 results.append(dict(name=name,nodes=info['nodes'],peak_pcm_difference=peak));print(name,'passed',flush=True)
for n in [1,2,3,4,5,9]:
 signals=[f'SinOsc.ar({180+i*73}) * 0.025' for i in range(n)]
 compare('Mix.ar '+str(n),'Pan2.ar(Mix.ar(['+','.join(signals)+']))','Pan2.ar('+('+'.join('('+x+')' for x in signals))+')')
compare('Mix.ar scalar','Pan2.ar(Mix.ar([0.01,0.02]))','Pan2.ar(DC.ar(0.03))')
compare('Mix.ar control','Pan2.ar(Mix.ar([SinOsc.kr(5)*0.02,SinOsc.kr(7)*0.02]))','Pan2.ar(K2A.ar((SinOsc.kr(5)*0.02)+(SinOsc.kr(7)*0.02)))')
compare('Mix.ar stereo arrays','Mix.ar([Pan2.ar(SinOsc.ar(220)*0.02,-0.4),Pan2.ar(SinOsc.ar(330)*0.02,0.6)])','Pan2.ar(SinOsc.ar(220)*0.02,-0.4)+Pan2.ar(SinOsc.ar(330)*0.02,0.6)')
for n,spread,center,comp in [(1,1,0,True),(2,1,0,True),(5,1,0,True),(5,0,0,True),(5,.6,.2,False),(5,.6,-.2,True)]:
 signals=[f'SinOsc.ar({180+i*73}) * 0.025' for i in range(n)]
 count=max(2,n);level=.7/(math.sqrt(count) if comp else 1)
 pans=[f'Pan2.ar({signals[i%n]}, {(-1+2*i/(count-1))*spread+center})' for i in range(count)]
 compare(f'Splay.ar n={n} spread={spread} center={center} levelComp={comp}',f'Splay.ar([{",".join(signals)}], {spread}, 0.7, {center}, {str(comp).lower()})','('+ '+'.join(pans)+f') * {level}')
source=(ROOT/'examples/dusty.scd').read_text();binary,info=compile_source(source)
(ROOT/'examples/dusty.scsyndef').write_bytes(binary);(ROOT/'examples/dusty.fmslot').write_bytes(pack('Dusty',source,binary))
original,_=compile_source(r"""SynthDef(\dusty, {
 arg freq=440, amp=0.5, knob1=0, knob2=0, gate=1;
 var env=EnvGen.ar(Env.perc(0.01,3),doneAction:2);
 var sig=SinOsc.ar(freq)*SinOsc.ar(freq)*Decay2.ar(Dust.ar(10!5))*env*amp;
 Out.ar(0,Splay.ar(sig));
})""")
x,y=pcm(original),pcm(binary)
factoring_difference=max(abs(a-b) for a,b in zip(x,y))
assert len(x)==len(y) and factoring_difference<=1,factoring_difference
data,meters=preview(binary);(out/'dusty.wav').write_bytes(data)
with wave.open(io.BytesIO(data)) as w:samples=array.array('h',w.readframes(w.getnframes()))
peaks=[max(abs(x) for x in samples[c::2])/32768 for c in (0,1)]
assert min(peaks)>.001 and max(peaks)<.99,peaks
assert any(a!=b for a,b in zip(samples[::2],samples[1::2])),'Splay collapsed stereo'
result=dict(helper_cases=results,dusty=dict(info=info,stereo_peaks=peaks,bytes=(ROOT/'examples/dusty.fmslot').stat().st_size),factoring_max_pcm_difference=factoring_difference,helpers_require_new_node_types=False)
(out/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result['dusty']),flush=True)
