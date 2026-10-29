(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
 var def=SynthDef(name,fn);def.writeDefFile(dir);
 if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
 Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[Gendy1,Gendy2,Gendy3].do{|cls|
 [\ar,\kr].do{|rate|
  (0..6).do{|dist|
   write.(("m10_"++cls.name++"_"++rate++"_dist"++dist).asSymbol,{
    var f=if(rate==\ar,{37},{1}),y;
    RandSeed.ir(1,12345);
    y=if(cls==Gendy3,{cls.perform(rate,dist,dist,0.7,0.6,f,0.5,0.5,12,12)},{cls.perform(rate,dist,dist,0.7,0.6,f,f*1.5,0.5,0.5,12,12)});
    Out.ar(0,(y*0.1).asAudioRateInput);
   });
  };
  [1,2,64].do{|cp|
   write.(("m10_"++cls.name++"_"++rate++"_cp"++cp).asSymbol,{
    var f=if(rate==\ar,{37},{1}),y;RandSeed.ir(1,12345);
    y=if(cls==Gendy3,{cls.perform(rate,0,0,0.7,0.6,f,0.5,0.5,cp,cp)},{cls.perform(rate,0,0,0.7,0.6,f,f*1.5,0.5,0.5,cp,cp)});
    Out.ar(0,(y*0.1).asAudioRateInput);
   });
  };
  [0,1,2].do{|mode|
   write.(("m10_"++cls.name++"_"++rate++"_live"++mode).asSymbol,{|gate=3|
    var f=if(mode==0,{gate},{if(mode==1,{0},{3})}),cp=if(mode==2,{gate},{12}),y;
    RandSeed.ir(1,12345);
    y=if(cls==Gendy3,{cls.perform(rate,6,6,0.5,0.5,f,0.5,0.5,12,cp)},{cls.perform(rate,6,6,0.5,0.5,f,f*1.5,0.5,0.5,12,cp)});
    Out.ar(0,(y*0.1).asAudioRateInput);
   },[[0.25,[\n_set,1000,\gate,7]]]);
  };
 };
 write.(("m10_"++cls.name++"_defaults").asSymbol,{RandSeed.ir(1,12345);Out.ar(0,cls.ar*0.1)});
};
"Gendy fixtures exported".postln;0.exit;
)
