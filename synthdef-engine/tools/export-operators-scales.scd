(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
 var def=SynthDef(name,fn);def.writeDefFile(dir);
 if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
 Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[\octcps,\cpsoct,\acos,\atan,\sinh,\cosh,\tanh].do{|op|
 [\ar,\kr].do{|rate|
  [0,1,2].do{|mode|
   write.(("m12_unary_"++op++"_"++rate++"_"++mode).asSymbol,{|gate=0|
    var x=SinOsc.perform(rate,13,0.2,if(op==\acos,0.9,3.0));
    if(mode==1){x=DC.perform(rate,if(op==\acos,-1.0,-0.7))};
    if(mode==2){x=if(rate==\ar,{gate.asAudioRateInput},{gate})};
    if(op==\cpsoct){x=(x*100)+440};
    Out.ar(0,x.perform(op).asAudioRateInput * if(op==\octcps,0.0001,0.01));
   },[[0.15,[\n_set,1000,\gate,0.7]],[0.3,[\n_set,1000,\gate,-0.9]]]);
  };
 };
};
[\div,\gcd,\lcm,\atan2,\hypot,\hypotApx,\pow,\ring1,\ring2,\ring3,\ring4,\difsqr,\sumsqr,\sqrsum,\sqrdif,\absdif,\thresh,\amclip,\scaleneg,\excess,\firstArg].do{|op|
 [\ar,\kr].do{|rate|
  [0,1,2,3,4].do{|mode|
   // SC 3.14.1 idiv_ak misses a loop; firstArg_ka falls back to addition.
   // Test intended arithmetic for those two paths independently, not against broken NRT output.
   if(((op!=\div) or: {rate!=\ar} or: {mode!=3}) and: {(op!=\firstArg) or: {rate!=\ar} or: {mode!=4}}){
   write.(("m12_binary_"++op++"_"++rate++"_"++mode).asSymbol,{|gate=0|
    var a=SinOsc.perform(rate,17,0.2,0.8),b=SinOsc.perform(rate,7,0.3,0.4,0.7),y;
    if([\div,\gcd,\lcm].includes(op)){a=DC.perform(rate,-15.9);b=3.2};
    if(mode==1){b=0.37};
    if(mode==2){a=if(rate==\ar,{(gate-0.4).asAudioRateInput},{gate-0.4});b=1.3};
    if(mode==3){b=0.7+(gate*0.2)};
    if(mode==4){a=gate-0.4;b=SinOsc.perform(rate,7,0.31,0.41,0.73)};
    y=a.perform(op,b);Out.ar(0,y.asAudioRateInput * 0.001);
   },[[0.15,[\n_set,1000,\gate,0.7]],[0.3,[\n_set,1000,\gate,-0.9]]]);
   };
  };
 };
};
[Scale.major,Scale.minor,Scale.major(Tuning.just),Scale.chromatic(Tuning.werckmeister),Scale.new([0,1,4,6,9],12,Tuning.et12)].do{|scale,si|
 [\ar,\kr].do{|rate|
  write.(("m12_scale_"++si++"_"++rate).asSymbol,{|gate=0|
   var table=scale.as(LocalBuf),degree=if(rate==\ar,{SinOsc.ar(9,0,11.2,0.3)},{gate});
   var key=DegreeToKey.perform(rate,table,degree,scale.stepsPerOctave);
   Out.ar(0,key.asAudioRateInput*0.001);
  },[[0.08,[\n_set,1000,\gate,3.7]],[0.16,[\n_set,1000,\gate,-1.2]],[0.24,[\n_set,1000,\gate,13]],[0.32,[\n_set,1000,\gate,-8.1]],[0.4,[\n_set,1000,\gate,0]]]);
 };
 write.(("m12_scale_compile_"++si).asSymbol,{
  var ratio=scale.degreeToRatio(-3,2),freq=scale.degreeToFreq(10,110,-1);
  Out.ar(0,SinOsc.ar([freq,110*ratio],0,0.01));
 });
};
write.(\m12_scale_multiple,{|gate=0|
 var a=Scale.major(Tuning.just).as(LocalBuf),b=Scale.minor.as(LocalBuf);
 Out.ar(0,[DegreeToKey.kr(a,gate,12),DegreeToKey.kr(b,gate,12)].asAudioRateInput*0.001);
},[[0.2,[\n_set,1000,\gate,3]],[0.35,[\n_set,1000,\gate,-2]]]);
write.(\m12_scale_tuning_custom,{
 var tuning=Tuning.new([0,2.3,4.2,5.8,7.1,9.6,11.9],2,"custom");
 var scale=Scale.new([0,1,2,3,4,5,6],7,tuning),freq=scale.degreeToFreq(-2,440,1);
 Out.ar(0,SinOsc.ar(freq,0,0.01));
});
"Operators/Scale/Tuning fixtures exported".postln;0.exit;
)
