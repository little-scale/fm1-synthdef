// Deterministic stage-2 fixtures. This script does not boot an audio server.
(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var changes=[[0.25,[\n_set,1000,\gate,1]],[0.4,[\n_set,1000,\gate,0]]];
var write={ |name,fn,events=#[]|
    var def=SynthDef(name,fn);
    def.writeDefFile(dir);
    Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
    if(events.size>0) {File.use(dir +/+ name ++ ".events.json", "w", { |f| f.write("[[0.25,1],[0.4,0]]") })};
};
[\ar,\kr].do { |rate|
    var audio={ |x| if(rate==\kr,{K2A.ar(x)},{x}) };
    [LFSaw,LFTri,LFPulse].do { |cls|
        write.(("m2_"++cls.name++"_"++rate).asSymbol,{ Out.ar(0,Pan2.ar(audio.(cls.perform(rate,7,0.2)))) });
        write.(("m2_"++cls.name++"_"++rate++"_step").asSymbol,{ |gate=0| Out.ar(0,Pan2.ar(audio.(cls.perform(rate,7+gate*5,0.2)))) },changes);
    };
    [Lag,LagUD].do { |cls|
        write.(("m2_"++cls.name++"_"++rate).asSymbol,{ |gate=0| Out.ar(0,Pan2.ar(audio.(cls.perform(rate,DC.perform(rate,0.1)+gate,0.05,0.1)))) },changes);
        write.(("m2_"++cls.name++"_"++rate++"_time").asSymbol,{ |gate=0| Out.ar(0,Pan2.ar(audio.(cls.perform(rate,SinOsc.perform(rate,12),0.01+gate*0.05,0.15)))) },changes);
    };
    [LPF,HPF,RLPF,BPF,Resonz,Ringz].do { |cls|
        var frequency=if(rate==\ar,{1000},{80});
        write.(("m2_"++cls.name++"_"++rate).asSymbol,{ Out.ar(0,Pan2.ar(audio.(cls.perform(rate,SinOsc.perform(rate,frequency*0.4)*0.01,frequency,0.2)))) });
        write.(("m2_"++cls.name++"_"++rate++"_dc").asSymbol,{ Out.ar(0,Pan2.ar(audio.(cls.perform(rate,DC.perform(rate,0.01),frequency,0.2)))) });
        write.(("m2_"++cls.name++"_"++rate++"_step").asSymbol,{ |gate=0| Out.ar(0,Pan2.ar(audio.(cls.perform(rate,SinOsc.perform(rate,frequency*0.4)*0.01,frequency*(1+gate),0.2+gate*0.1)))) },changes);
    };
    write.(("m2_LeakDC_"++rate).asSymbol,{ |gate=0| Out.ar(0,Pan2.ar(audio.(LeakDC.perform(rate,DC.perform(rate,0.2)+gate,0.995)))) },changes);
    write.(("m2_LeakDC_"++rate++"_coef").asSymbol,{ |gate=0| Out.ar(0,Pan2.ar(audio.(LeakDC.perform(rate,SinOsc.perform(rate,7),0.99-gate*0.1)))) },changes);
    write.(("m2_Clip_"++rate).asSymbol,{ |gate=0| Out.ar(0,Pan2.ar(audio.(Clip.perform(rate,SinOsc.perform(rate,7),-0.2,0.2+gate*0.3)))) },changes);
    write.(("m2_Select_"++rate).asSymbol,{ |gate=0| Out.ar(0,Pan2.ar(audio.(Select.perform(rate,gate,[SinOsc.perform(rate,7),SinOsc.perform(rate,17)])))) },changes);
};
write.("m2_LFSaw_negative",{Out.ar(0,Pan2.ar(LFSaw.ar(-7,0.2)))});
write.("m2_LFSaw_audio",{Out.ar(0,Pan2.ar(LFSaw.ar(SinOsc.ar(2)*5+10,0.2)))});
write.("m2_LFTri_audio",{Out.ar(0,Pan2.ar(LFTri.ar(SinOsc.ar(2)*5+10,0.2)))});
write.("m2_LFPulse_width",{|gate=0|Out.ar(0,Pan2.ar(LFPulse.ar(7,0.2,0.2+gate*0.5)))},changes);
write.("m2_Select_audio",{Out.ar(0,Pan2.ar(Select.ar(SinOsc.ar(5)+1,[SinOsc.ar(220),SinOsc.ar(330),SinOsc.ar(440)])))});
write.("m2_Clip_audio",{Out.ar(0,Pan2.ar(Clip.ar(SinOsc.ar(220),SinOsc.ar(3)*0.1-0.3,SinOsc.ar(4)*0.1+0.3)))});
write.("m2_LagControl",{Out.ar(0,Pan2.ar(K2A.ar(NamedControl.kr(\gate,0,0.05,true))))},changes);
write.("m2_LagControl_array",{var x=NamedControl.kr(\gate,[0,0.25],[0.05,0.1],true);Out.ar(0,K2A.ar(x))},changes);
write.("m2_TrigControl",{Out.ar(0,Pan2.ar(K2A.ar(NamedControl.tr(\gate,0))))},changes);
"Stage 2 fixtures exported".postln;
0.exit;
)
