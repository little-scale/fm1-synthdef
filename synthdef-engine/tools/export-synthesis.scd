(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,live=false|
 var def=SynthDef(name,fn),events=if(live,{[[0.25,[\n_set,1000,\gate,1]]]},{[]});def.writeDefFile(dir);
 if(live){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[[0.25,1]]")})};
 Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[\ar,\kr].do{|rate|
 [0,0.3,1,2,4].do{|fb,i|write.(("m15_SinOscFB_"++rate++"_"++i).asSymbol,{Out.ar(0,SinOscFB.perform(rate,if(rate==\ar,220,10),fb).asAudioRateInput*0.1)})};
 write.(("m15_SinOscFB_"++rate++"_live").asSymbol,{|gate=0|Out.ar(0,SinOscFB.perform(rate,if(rate==\ar,220,10)+(gate*20),gate*0.3).asAudioRateInput*0.1)},true);
 [1,5,100,0,-3].do{|harm,i|write.(("m15_Blip_"++rate++"_"++i).asSymbol,{Out.ar(0,Blip.perform(rate,if(rate==\ar,220,10),harm).asAudioRateInput*0.1)})};
 write.(("m15_Blip_"++rate++"_live").asSymbol,{|gate=0|Out.ar(0,Blip.perform(rate,if(rate==\ar,220,10)+(gate*200),5+(gate*100)).asAudioRateInput*0.1)},true);
 [0,1,2,3].do{|i|write.(("m15_Phasor_"++rate++"_"++i).asSymbol,{var trig=if(i==0,0,Impulse.perform(rate,17));Out.ar(0,Phasor.perform(rate,trig,[0.001,-0.001,0.125,2][i],-1,1,0.25).asAudioRateInput*0.1)})};
 [0,1,2].do{|i|write.(("m15_MoogFF_"++rate++"_"++i).asSymbol,{var x=SinOsc.perform(rate,if(rate==\ar,220,10));Out.ar(0,MoogFF.perform(rate,x,if(rate==\ar,[50,1000,10000][i],[10,100,250][i]),[0,2,3.8][i]).asAudioRateInput*0.1)})};
 write.(("m15_MoogFF_"++rate++"_live").asSymbol,{|gate=0|Out.ar(0,MoogFF.perform(rate,Impulse.perform(rate,17),if(rate==\ar,1000,100)+(gate*100),1+(gate*2),gate).asAudioRateInput*0.1)},true);
};
[0,1,2,3].do{|i|write.(("m15_FreqShift_"++i).asSymbol,{var f=if(i==2,{SinOsc.ar(3)*100},{[-100,0,0,100][i]}),p=if(i==3,{SinOsc.ar(2)},{0});Out.ar(0,FreqShift.ar(SinOsc.ar(440),f,p)*0.1)})};
write.(\m15_FreqShift_live,{|gate=0|Out.ar(0,FreqShift.ar(SinOsc.ar(440),100-(gate*200),gate*0.5)*0.1)},true);
[0,1,2,3].do{|i|write.(("m15_Pluck_"++i).asSymbol,{var x;RandSeed.ir(1,12345);x=WhiteNoise.ar(0.1);Out.ar(0,Pluck.ar(x,Impulse.perform(if(i==3,\ar,\kr),10),0.02,[0.001,0.003,0.005,0.01][i],[0,0.2,-0.2,1][i],[-0.5,0,0.5,0.8][i]))})};
write.(\m15_Pluck_live,{|gate=0|RandSeed.ir(1,12345);Out.ar(0,Pluck.ar(WhiteNoise.ar(0.1),Impulse.kr(10),0.02,0.003+(gate*0.002),0.2+(gate*0.3),gate*0.5))},true);
write.(\m15_Pluck_audio_coef,{RandSeed.ir(1,12345);Out.ar(0,Pluck.ar(WhiteNoise.ar(0.1),Impulse.ar(10),0.02,0.003,0.3,SinOsc.ar(3)*0.5))});
[0.00001,0.001,0.01,0.1].do{|dur,i|write.(("m15_Limiter_"++i).asSymbol,{Out.ar(0,Limiter.ar(SinOsc.ar(220)*LFPulse.ar(13,0,0.7)*2,0.2,dur))})};
write.(\m15_Limiter_live,{|gate=0|Out.ar(0,Limiter.ar(SinOsc.ar(220)*2,0.2-(gate*0.1),0.01))},true);
[FBSineN,FBSineL,FBSineC,StandardN,StandardL].do{|cls|
 [\stable,\chaotic,\live].do{|mode|write.(("m15_"++cls.name++"_"++mode).asSymbol,{|gate=0|
 var args,x,freq=if(mode==\live,{2000+(gate*4000)},{4000});
 args=if(cls.name.asString.beginsWith("FBSine"),{[freq,if(mode==\stable,0,1),if(mode==\stable,0,0.1),1.1,0.5,0.1,0.1]},{[freq,if(mode==\stable,0,1),if(mode==\stable,0,0.5),if(mode==\stable,0,0.5)]});
 x=cls.ar(*args);Out.ar(0,x*0.1);
 },mode==\live)};
};
write.(\m15_FreqShift_audio_both,{Out.ar(0,FreqShift.ar(SinOsc.ar(440),SinOsc.ar(3)*100,SinOsc.ar(2))*0.1)});
write.(\m15_Phasor_audio_rate,{Out.ar(0,Phasor.ar(Impulse.ar(17),SinOsc.ar(3)*0.01,-1,1,0)*0.1)});
write.(\m15_Phasor_live,{|gate=0|Out.ar(0,Phasor.ar(Impulse.kr(17),0.001+(gate*0.002),-1,1,0)*0.1)},true);
"Synthesis fixtures exported".postln;0.exit;
)
