#!/usr/bin/env python3
"""Compile every stock Scale/Tuning preset; check live lookup against its pitch data."""
from pathlib import Path
import os,sys,subprocess,tempfile,json,math,ctypes
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from sc_language import language_command
from test_engine import Engine,lib
script=r'''(
var dir=__DIRECTORY__,rows=List.new;
[Scale,Tuning].do{|klass|
 klass.names.do{|name|
  var object=klass.at(name),id=(klass.name++"_"++name).asSymbol;
  var values=object.semitones,octave=object.stepsPerOctave;
  SynthDef(id,{|degree=0|Out.ar(0,K2A.ar(DegreeToKey.kr(object.as(LocalBuf),degree,octave)))}).writeDefFile(dir);
  rows.add("{\"name\":"++id.asString.asCompileString++",\"kind\":"++klass.name.asString.asCompileString++",\"octave\":"++octave.asCompileString++",\"values\":["++values.collect(_.asCompileString).join(",")++"]}");
 };
};
File.use(dir+/+"catalog.json","w",{|f|f.write("["++rows.join(",")++"]")});
"Scale/Tuning catalog exported".postln;0.exit;
)'''
checks=0;results=[]
with tempfile.TemporaryDirectory(prefix='fm1-scale-catalog-') as tmp:
 path=Path(tmp);source=path/'catalog.scd';source.write_text(script.replace('__DIRECTORY__',json.dumps(tmp)))
 r=subprocess.run(language_command(os.environ.get('SCLANG','/Applications/SuperCollider.app/Contents/MacOS/sclang'),source,tmp),capture_output=True,text=True,timeout=45)
 assert r.returncode==0 and 'ERROR:' not in r.stdout+r.stderr and 'catalog exported' in r.stdout,r.stdout+r.stderr
 for row in json.loads((path/'catalog.json').read_text()):
  e=Engine((path/(row['name']+'.scsyndef')).read_bytes());assert e.ok,(row['name'],e.error);assert e.start()
  values=[ctypes.c_float(v).value for v in row['values']];size=len(values);octave=int(row['octave']);degrees=[i+.25 for i in range(-2*size,2*size+1)]+[-2*size,-size,0,size,2*size]
  peak=0
  for degree in degrees:
   assert e.set('degree',degree);e.render();out=e.render();index=math.floor(degree)
   expected=ctypes.c_float(values[index%size]+octave*(index//size)).value
   error=abs(out[-2]-expected);peak=max(peak,error);assert error<=2e-5,(row['name'],degree,out[-2],expected)
   assert not lib.test_fault(e.v);checks+=1
  results.append(dict(name=row['name'],kind=row['kind'],entries=size,checks=len(degrees),peak_error=peak))
record=dict(presets=len(results),scales=sum(r['kind']=='Scale' for r in results),tunings=sum(r['kind']=='Tuning' for r in results),lookup_checks=checks,results=results)
(ROOT/'build/scale-catalog-results.json').write_text(json.dumps(record,indent=2)+'\n');print({k:v for k,v in record.items() if k!='results'})
