(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
 var def=SynthDef(name,fn);def.writeDefFile(dir);
 if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
 Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[\ar,\kr].do{|rate|
 [Fold,Wrap].do{|cls|
  [0,1,2].do{|mode|write.(("m8_"++cls.name++"_"++rate++"_"++mode).asSymbol,{|gate=0.2|
   var lo=if(mode==1,{SinOsc.perform(rate,5,0,0.1,-0.3)},{-0.3});
   var hi=if(mode==2,{gate},{0.2});
   Out.ar(0,cls.perform(rate,SinOsc.perform(rate,17,0.3),lo,hi).asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,0.4]]])};
 };
 [BrownNoise,ClipNoise].do{|cls|write.(("m8_"++cls.name++"_"++rate).asSymbol,{RandSeed.ir(1,12345);Out.ar(0,cls.perform(rate,0.1).asAudioRateInput)})};
 [Trig,Trig1,PulseDivider,PulseCount,ToggleFF,SetResetFF,Stepper].do{|cls|
  [0,1,2].do{|mode|write.(("m8_"++cls.name++"_"++rate++"_"++mode).asSymbol,{|gate=0.01|
   var trigger=if(mode==1,{DC.perform(rate,1)},{LFPulse.perform(rate,37,0,0.3,0.4)});
   var reset=if(mode==2,{LFPulse.perform(rate,11)},{0});
   var y=switch(cls,
    Trig,{Trig.perform(rate,trigger,gate)},Trig1,{Trig1.perform(rate,trigger,gate)},
    PulseDivider,{PulseDivider.perform(rate,trigger,3,1)},PulseCount,{PulseCount.perform(rate,trigger,reset)},
    ToggleFF,{ToggleFF.perform(rate,trigger)},SetResetFF,{SetResetFF.perform(rate,trigger,reset)},
    Stepper,{Stepper.perform(rate,trigger,reset,-2,3,2,1)});
   Out.ar(0,(y*0.01).asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,0.003]]])};
 };
 [XFade2,LinXFade2,Balance2].do{|cls|
  [0,1,2].do{|mode|write.(("m8_"++cls.name++"_"++rate++"_"++mode).asSymbol,{|gate= -0.5|
   var pos=if(mode==1,{SinOsc.perform(rate,7)},{gate});
   var level=if(mode==2,{gate+1},{0.5});
   Out.ar(0,cls.perform(rate,SinOsc.perform(rate,23,0.4,0.1),SinOsc.perform(rate,39,1,0.1),pos,level).asArray.collect(_.asAudioRateInput));
  },[[0.25,[\n_set,1000,\gate,0.8]]])};
 };
 [Lag2,Lag3,Slew].do{|cls|
  [0,1,2].do{|mode|write.(("m8_"++cls.name++"_"++rate++"_"++mode).asSymbol,{|gate=0.03|
   var time=if(mode==0,{0.03},{if(mode==1,{0},{gate})});
   var x=LFPulse.perform(rate,31,0,0.3,0.1,0.1);
   var y=if(cls==Slew,{Slew.perform(rate,x,time*200,4)},{cls.perform(rate,x,time)});
   Out.ar(0,y.asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,0.007]]])};
 };
 [0,1].do{|mode|write.(("m8_LinExp_"++rate++"_"++mode).asSymbol,{|gate=2|
  Out.ar(0,(LinExp.perform(rate,SinOsc.perform(rate,11,0.7),-1,1,if(mode==0,{0.01},{-0.01}),if(mode==0,{gate},{gate.neg}))*0.1).asAudioRateInput);
 },[[0.25,[\n_set,1000,\gate,3]]])};
};
[0.0,0.03,-0.13,0.37,-0.8,3.0,-3.1].do{|value,i|
 write.(("m8_MiMu_dc"++i).asSymbol,{|gate=1|Out.ar(0,MiMu.ar(DC.ar(value),gate))},[[0.25,[\n_set,1000,\gate,2]]]);
};
write.(\m8_MiMu_bypass,{|gate=0|Out.ar(0,MiMu.ar(LFPulse.ar(71,0.3,0.5,0.4,-0.2),3,gate))},[[0.25,[\n_set,1000,\gate,1]]]);
[\mod,\round,\roundUp,\trunc,\clip2,\fold2,\wrap2].do{|op|
 [\ar,\kr].do{|rate|
  [0,1].do{|mode|write.(("m8_op_"++op++"_"++rate++"_"++mode).asSymbol,{|gate=0.3|
   var y=SinOsc.perform(rate,17,0.4).perform(op,if(mode==0,{0.3},{gate}));
   Out.ar(0,(y*0.1).asAudioRateInput);
  },[[0.25,[\n_set,1000,\gate,0.2]]])};
 };
};
[\ar,\kr].do{|rate|
 write.(("m8_SelectX_"++rate).asSymbol,{
  Out.ar(0,SelectX.perform(rate,SinOsc.kr(3,0.7,1.2,1.2),[SinOsc.perform(rate,31,0,0.1),SinOsc.perform(rate,47,0.3,0.1),SinOsc.perform(rate,71,0.7,0.1)]).asAudioRateInput);
 });
};
[Stepper,PulseCount,SetResetFF].do{|cls|
 [\ar,\kr].do{|rate|write.(("m8_"++cls.name++"_"++rate++"_initialReset").asSymbol,{
  var trigger=LFPulse.perform(rate,37),y=if(cls==Stepper,{Stepper.perform(rate,trigger,1,-2,3,2,1)},{cls.perform(rate,trigger,1)});
  Out.ar(0,(y*0.01).asAudioRateInput);
 })};
};
"Utility fixtures exported".postln;0.exit;
)
