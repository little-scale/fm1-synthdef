(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
 var def=SynthDef(name,fn);def.writeDefFile(dir);
 if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
 Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[\ar,\kr].do{|rate|
 [GrayNoise,LFClipNoise,LFDClipNoise,Crackle].do{|cls|
  [0,1,2].do{|i|write.(("m14_"++cls.name++"_"++rate++"_"++i).asSymbol,{
   var x;RandSeed.ir(1,12345);
   x=if(cls==GrayNoise,{cls.perform(rate)},{cls.perform(rate,if(cls==Crackle,{[1.1,1.5,1.95][i]},{[0,17,1000][i]}))});
   Out.ar(0,x.asAudioRateInput*0.1);
  })};
 };
 [0,1,2].do{|i|
  write.(("m14_Hasher_"++rate++"_"++i).asSymbol,{var x=switch(i,0,{DC.perform(rate,0.125)},1,{Impulse.perform(rate,17)},2,{LFPulse.perform(rate,17,0,0.5)});Out.ar(0,Hasher.perform(rate,x).asAudioRateInput*0.1)});
  [CoinGate,TIRand,TWindex].do{|cls|write.(("m14_"++cls.name++"_"++rate++"_"++i).asSymbol,{
   var trig,x;RandSeed.ir(1,12345);trig=Impulse.perform(rate,17);
   x=switch(cls,CoinGate,{CoinGate.perform(rate,[0,0.4,1][i],trig)},TIRand,{TIRand.perform(rate,[-5,0,3][i],[5,0,9][i],trig)},TWindex,{TWindex.perform(rate,trig,[[0,0,1],[0.2,0.5,0.3],[1,2,4]][i],if(i==2,1,0))});
   // Keep the trigger alive: upstream TIRand constructor otherwise aliases it with a negative output.
   Out.ar(0,if(cls==TIRand,{[x.asAudioRateInput*0.05,trig.asAudioRateInput*0.001]},{x.asAudioRateInput*0.05}));
  })};
 };
 write.(("m14_Logistic_"++rate++"_stable").asSymbol,{Out.ar(0,Logistic.perform(rate,2,123,0.5).asAudioRateInput*0.1)});
 write.(("m14_Logistic_"++rate++"_chaotic").asSymbol,{Out.ar(0,Logistic.perform(rate,3.9,if(rate==\ar,4000,344),0.5).asAudioRateInput*0.1)});
};
[HenonN,HenonL,HenonC,CuspN,CuspL,LatoocarfianN,LatoocarfianL,LatoocarfianC,LorenzL].do{|cls|
 [\stable,\chaotic,\live].do{|mode|
  write.(("m14_"++cls.name++"_"++mode).asSymbol,{|gate=0|
   var args,x;
   args=case
   {cls.name.asString.beginsWith("Henon")} {if(mode==\stable,{[2000,0.1,0.1,0,0]},{[4000,1.4,0.3,0,0]})}
   {cls.name.asString.beginsWith("Cusp")} {if(mode==\stable,{[2000,0.5,0,0]},{[4000,1,1.9,0]})}
   {cls==LorenzL} {if(mode==\stable,{[2000,10,28,2.667,0,0.1,0,0]},{[4000,10,28,2.667,0.05,0.1,0,0]})}
   {true} {if(mode==\stable,{[2000,0,0,0.5,0.5,0.5,0.5]},{[4000,1,3,0.5,0.5,0.5,0.5]})};
   if(mode==\live){args[0]=2000+(gate*4000)};
   x=cls.ar(*args);Out.ar(0,x*0.1);
  },if(mode==\live,{[[0.25,[\n_set,1000,\gate,1]]]},#[]));
 };
};
write.(\m14_LFDClipNoise_audio,{RandSeed.ir(1,12345);Out.ar(0,LFDClipNoise.ar(SinOsc.ar(3,0,500,501))*0.1)});
"Stochastic fixtures exported".postln;0.exit;
)
