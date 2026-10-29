(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
 var def=SynthDef(name,fn);def.writeDefFile(dir);
 if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
 Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[
 [440,1760,880],[100,500,100],[123.45,1234.56,789.01],[0,1000,500],
 [100,0,500],[100,1000,0],[1000,300,100],[22000,19000,21000],
 [0.01,22050,100],[22050,22050,22050],[440,1760,440],[440,1760,20000]
].do{|args,i|write.(("m11_Formant_"++i).asSymbol,{Out.ar(0,Formant.ar(*args)*0.1)})};
(0..2).do{|param|
 write.(("m11_Formant_live"++param).asSymbol,{|gate=500|
  var args=[200,1600,800];args[param]=gate;Out.ar(0,Formant.ar(*args)*0.1);
 },[[0.25,[\n_set,1000,\gate,1100]]]);
};
[\ar,\kr].do{|rate|
 [0,1,2].do{|source|
  [[0,0.01,0.1],[0.1,0.01,0.1],[20,0.01,0.1],[200,0.01,0.1],[300,0,0.1],[300,0.1,0],[300,0.1,0.1],[300,0.2,0.03],[300,0,0],[300,1,3],[300,0.00001,0.00002],[if(rate==\ar,{22050},{344.53125}),0.02,0.1]].do{|params,i|
   write.(("m11_Formlet_"++rate++"_"++source++"_"++i).asSymbol,{
    var x,y;RandSeed.ir(1,12345);
    x=switch(source,0,{Impulse.perform(rate,17)*0.01},1,{SinOsc.perform(rate,37,0.3,0.01)},2,{WhiteNoise.perform(rate,0.01)});
    y=Formlet.perform(rate,x,*params);Out.ar(0,y.asAudioRateInput);
   });
  };
 };
 (1..3).do{|param|
  write.(("m11_Formlet_"++rate++"_live"++param).asSymbol,{|gate=0|
   var x=Impulse.perform(rate,31)*0.01,args=[x,200,0.01,0.1],y;
   args[param]=switch(param,1,{100+(gate*200)},2,{0.01+(gate*0.04)},3,{0.1+(gate*0.2)});
   y=Formlet.perform(rate,*args);Out.ar(0,y.asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,1]]]);
 };
};
[\ar,\kr].do{|rate|
 write.(("m11_Formlet_"++rate++"_cross").asSymbol,{|gate=0|
  var freq=if(rate==\ar,{1000+(gate*14000)},{100+(gate*200)});
  var y=Formlet.perform(rate,Impulse.perform(rate,31)*0.01,freq,0.005,0.04);
  Out.ar(0,y.asAudioRateInput);
 },[[0.1,[\n_set,1000,\gate,1]],[0.2,[\n_set,1000,\gate,0]],[0.3,[\n_set,1000,\gate,1]],[0.4,[\n_set,1000,\gate,0]]]);
};
"Formant/Formlet fixtures exported".postln;0.exit;
)
