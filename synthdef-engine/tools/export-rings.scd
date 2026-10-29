(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
 var def=SynthDef(name,fn);def.writeDefFile(dir);
 if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
 Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[MiRings1,MiRings2,MiRings3,MiRings4,MiRings5,MiRings6].do{|cls|
 [\internal,\impulse,\noise,\forced,\bypass,\live,\retrigger].do{|mode|
  write.(("m13_"++cls.name++"_"++mode).asSymbol,{|gate=0|
   var source=0,trig=1,params=[0.4,0.6,0.5,0.4],pitch=48,internal=0,bypass=0;
   RandSeed.ir(1,12345);
   if(mode==\impulse){source=Impulse.ar(17)*0.1};
   if(mode==\noise){source=WhiteNoise.ar(0.05)};
   if(mode==\forced){source=DC.ar(0);internal=1};
   if(mode==\bypass){source=SinOsc.ar(200,0,0.05);bypass=1};
   if(mode==\live){pitch=48+(gate*7);params=[0.4+(gate*0.4),0.6-(gate*0.3),0.5+(gate*0.3),0.4+(gate*0.3)]};
   if(mode==\retrigger){trig=gate};
   Out.ar(0,cls.ar(source,trig,pitch,params[0],params[1],params[2],params[3],internal,bypass));
  },if(mode==\live,{[[0.25,[\n_set,1000,\gate,1]]]},if(mode==\retrigger,{[[0.05,[\n_set,1000,\gate,1]],[0.1,[\n_set,1000,\gate,0]],[0.2,[\n_set,1000,\gate,1]]]},#[])));
 };
};
"Rings fixtures exported".postln;0.exit;
)
