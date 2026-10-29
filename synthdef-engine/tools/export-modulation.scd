(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
 var def=SynthDef(name,fn);def.writeDefFile(dir);
 if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
 Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[\ar,\kr].do{|rate|
 [VarSaw,SyncSaw,LFCub,LFPar].do{|cls|
  [0,1,2,3].do{|mode|write.(("m9_"++cls.name++"_"++rate++"_"++mode).asSymbol,{|gate=31|
   var freq=if(mode==1,{SinOsc.perform(rate,7,0,4,31)},{if(mode==2,{gate},{if(mode==3,{0},{31})})});
   var y=switch(cls,VarSaw,{VarSaw.perform(rate,freq,0.37,0.23)},SyncSaw,{SyncSaw.perform(rate,freq,if(mode==1,{SinOsc.perform(rate,11,0,7,91)},{91}))},LFCub,{LFCub.perform(rate,freq,0.2)},LFPar,{LFPar.perform(rate,freq,0.2)});
   Out.ar(0,(y*0.1).asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,77]]])};
 };
 [LFDNoise0,LFDNoise1,LFDNoise3].do{|cls|
  [0,1,2,3,4].do{|mode|write.(("m9_"++cls.name++"_"++rate++"_"++mode).asSymbol,{|gate=37|
   var freq=if(mode==1,{SinOsc.perform(rate,13,0,25,37)},{if(mode==2,{gate},{if(mode==3,{0},{if(mode==4,{if(rate==\ar,{90000},{1000})},{37})})})});
   RandSeed.ir(1,12345);Out.ar(0,cls.perform(rate,freq,0.1).asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,101]]])};
 };
 [RHPF,BRF].do{|cls|
  [0,1,2].do{|mode|write.(("m9_"++cls.name++"_"++rate++"_"++mode).asSymbol,{|gate=93|
   var x=SinOsc.perform(rate,53,0.7,0.1),freq=if(mode==2,{gate},{93}),rq=if(mode==1,{0.1},{0.7});
   Out.ar(0,cls.perform(rate,x,freq,rq).asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,151]]])};
 };
 [\ar,rate].as(Set).do{|inRate|
  [0,1,2].do{|mode|write.(("m9_Amplitude_"++rate++"_"++inRate++"_"++mode).asSymbol,{|gate=0.01|
   var time=if(mode==1,{0},{gate});
   Out.ar(0,Amplitude.perform(rate,SinOsc.perform(inRate,71,0.4,0.2)*LFPulse.perform(inRate,11),time,if(mode==1,{0},{0.03})).asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,0.03]]])};
 };
};
[BPeakEQ,BLowShelf,BHiShelf].do{|cls|
 [-12,0,12].do{|db,index|
  [0,1,2].do{|mode|write.(("m9_"++cls.name++"_"++index++"_"++mode).asSymbol,{|gate=900|
   var freq=if(mode==1,{gate},{900}),gain=if(mode==2,{gate*0.01},{db});
   Out.ar(0,cls.ar(SinOsc.ar(717,0.7,0.1),freq,0.7,gain));
  },[[0.25,[\n_set,1000,\gate,2100]]])};
 };
};
"Modulation fixtures exported".postln;0.exit;
)
