// Seeded noise and band-limited oscillator fixtures.
(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var changes=[[0.25,[\n_set,1000,\gate,1]],[0.4,[\n_set,1000,\gate,0]]];
var write={ |name,fn,events=#[]|
    var def=SynthDef(name,fn);
    def.writeDefFile(dir);
    Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
    if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[[0.25,1],[0.4,0]]")})};
};
[\ar,\kr].do { |rate|
    var audio={|x|if(rate==\kr,{K2A.ar(x)},{x})};
    [WhiteNoise,PinkNoise].do { |cls|
        write.(("m3_"++cls.name++"_"++rate).asSymbol,{RandSeed.ir(1,12345);Out.ar(0,Pan2.ar(audio.(cls.perform(rate))))});
    };
    [LFNoise0,LFNoise1,LFNoise2].do { |cls|
        write.(("m3_"++cls.name++"_"++rate).asSymbol,{RandSeed.ir(1,12345);Out.ar(0,Pan2.ar(audio.(cls.perform(rate,73))))});
        write.(("m3_"++cls.name++"_"++rate++"_step").asSymbol,{|gate=0|RandSeed.ir(1,12345);Out.ar(0,Pan2.ar(audio.(cls.perform(rate,73+gate*81))))},changes);
        write.(("m3_"++cls.name++"_"++rate++"_fast").asSymbol,{RandSeed.ir(1,-654);Out.ar(0,Pan2.ar(audio.(cls.perform(rate,48000))))});
        write.(("m3_"++cls.name++"_"++rate++"_zero").asSymbol,{RandSeed.ir(1,12345);Out.ar(0,Pan2.ar(audio.(cls.perform(rate,0))))});
    };
    [Saw,Pulse].do { |cls|
        var f=if(rate==\ar,{440},{40});
        write.(("m3_"++cls.name++"_"++rate).asSymbol,{Out.ar(0,Pan2.ar(audio.(cls.perform(rate,f))))});
        write.(("m3_"++cls.name++"_"++rate++"_step").asSymbol,{|gate=0|Out.ar(0,Pan2.ar(audio.(cls.perform(rate,f*(1+gate)))))},changes);
        write.(("m3_"++cls.name++"_"++rate++"_glide").asSymbol,{Out.ar(0,Pan2.ar(audio.(cls.perform(rate,Line.kr(f,f*3,0.5))))) });
        write.(("m3_"++cls.name++"_"++rate++"_high").asSymbol,{Out.ar(0,Pan2.ar(audio.(cls.perform(rate,if(rate==\ar,{12000},{200}))))) });
    };
    write.(("m3_Pulse_"++rate++"_width").asSymbol,{|gate=0|Out.ar(0,Pan2.ar(audio.(Pulse.perform(rate,if(rate==\ar,{440},{40}),0.2+gate*0.6))))},changes);
};
write.("m3_noise_mix",{RandSeed.ir(1,12345);Out.ar(0,Pan2.ar((WhiteNoise.ar+PinkNoise.ar+LFNoise1.ar(73))*0.1))});
write.("m3_noise_reseed",{|gate=0|RandSeed.ir(1,12345);RandSeed.kr(gate,555);Out.ar(0,Pan2.ar(WhiteNoise.ar))},changes);
write.("m3_Pulse_zero",{Out.ar(0,Pan2.ar(Pulse.ar(440,0)))});
write.("m3_Pulse_one",{Out.ar(0,Pan2.ar(Pulse.ar(440,1)))});
write.("m3_noise_audio_reseed",{RandSeed.ir(1,12345);RandSeed.ar(SinOsc.ar(10),555);Out.ar(0,Pan2.ar(WhiteNoise.ar))});
write.("m3_subtractive_voice",{|freq=110,gate=1,amp=0.1,pan=0|
    var envelope=EnvGen.ar(Env.adsr(0.01,0.08,0.6,0.15),gate,doneAction:2);
    var cutoff=300+(EnvGen.kr(Env.perc(0.01,0.3),gate)*4000)+(SinOsc.kr(5)*80);
    var signal=(Saw.ar(freq)+Pulse.ar(freq*0.501,0.35))*0.2;
    Out.ar(0,Pan2.ar(RLPF.ar(signal,cutoff,0.25)*envelope*amp,pan))
},changes);
"Stage 3 fixtures exported".postln;
0.exit;
)
