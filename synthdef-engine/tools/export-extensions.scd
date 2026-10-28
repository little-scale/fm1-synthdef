(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
    var def=SynthDef(name,fn);
    def.writeDefFile(dir);
    if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
    Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[SampleRate,SampleDur,ControlRate,ControlDur,BlockSize,RadiansPerSample].do{|cls|
    write.(("m6_"++cls.name).asSymbol,{Out.ar(0,DC.ar(cls.ir)*0.0001)});
};
[Delay1,Delay2].do{|cls|
    [\ar,\kr].do{|rate|
        write.(("m6_"++cls.name++"_"++rate).asSymbol,{
            var x=SinOsc.perform(rate,17,0.7,0.1)+0.2;
            Out.ar(0,cls.perform(rate,x).asAudioRateInput);
        });
        write.(("m6_"++cls.name++"_"++rate++"_initial").asSymbol,{
            var x=SinOsc.perform(rate,17,0.7,0.1)+0.2;
            var y=if(cls==Delay1,{cls.perform(rate,x,x1:0.3)},{cls.perform(rate,x,x1:0.3,x2:0.4)});
            Out.ar(0,y.asAudioRateInput);
        });
    };
};
[\cpsmidi,\midiratio,\ratiomidi,\dbamp,\ampdb].do{|op|
    [\ar,\kr].do{|rate|
        write.(("m6_op_"++op++"_"++rate).asSymbol,{
            var x=SinOsc.perform(rate,11,0,0.2,0.4);
            Out.ar(0,(x.perform(op)*0.01).asAudioRateInput);
        });
    };
};

[OnePole,OneZero,LPZ1,HPZ1].do{|cls|
    [\ar,\kr].do{|rate|
        [0.7,-0.7,0].do{|coef,index|
            write.(("m6_"++cls.name++"_"++rate++"_"++index).asSymbol,{
                var x=SinOsc.perform(rate,53,0.7,0.1)+0.05;
                Out.ar(0,if([OnePole,OneZero].includes(cls),{cls.perform(rate,x,coef)},{cls.perform(rate,x)}).asAudioRateInput);
            });
        };
        if([OnePole,OneZero].includes(cls)){
            write.(("m6_"++cls.name++"_"++rate++"_step").asSymbol,{|gate=0.7|
                Out.ar(0,cls.perform(rate,SinOsc.perform(rate,53,0.7,0.1),gate).asAudioRateInput);
            },[[0.25,[\n_set,1000,\gate,-0.7]]]);
        };
    };
};
[Latch,Gate].do{|cls|
    [\ar,\kr].do{|rate|
        [\kr,rate].as(Set).do{|trigRate|
            write.(("m6_"++cls.name++"_"++rate++"_"++trigRate).asSymbol,{
                Out.ar(0,cls.perform(rate,SinOsc.perform(rate,53,0.7,0.1),LFPulse.perform(trigRate,19)).asAudioRateInput);
            });
        };
        [0,1,-1].do{|trig|
            write.(("m6_"++cls.name++"_"++rate++"_held"++trig).asSymbol,{
                Out.ar(0,cls.perform(rate,SinOsc.perform(rate,53,0.7,0.1),trig).asAudioRateInput);
            });
        };
    };
};
[TRand,TExpRand].do{|cls|
    [\ar,\kr].do{|rate|
        [\kr,rate].as(Set).do{|trigRate|
            write.(("m6_"++cls.name++"_"++rate++"_"++trigRate).asSymbol,{
                RandSeed.ir(1,12345);
                Out.ar(0,cls.perform(rate,0.05,0.4,if(rate==\ar,{LFPulse.perform(trigRate,29).asAudioRateInput},{LFPulse.perform(trigRate,29)})).asAudioRateInput);
            });
        };
        [0,1,-1].do{|trig|
            write.(("m6_"++cls.name++"_"++rate++"_held"++trig).asSymbol,{
                RandSeed.ir(1,12345);Out.ar(0,cls.perform(rate,0.05,0.4,if(rate==\ar,{DC.ar(trig)},{trig})).asAudioRateInput);
            });
        };
        write.(("m6_"++cls.name++"_"++rate++"_bounds").asSymbol,{|gate=0.4|
            RandSeed.ir(1,12345);Out.ar(0,cls.perform(rate,0.05,gate,LFPulse.perform(rate,29)).asAudioRateInput);
        },[[0.25,[\n_set,1000,\gate,0.1]]]);
    };
};
[Dust,Dust2].do{|cls|
    [\ar,\kr].do{|rate|
        [-10,0,37,1000,90000].do{|density,index|
            write.(("m6_"++cls.name++"_"++rate++"_"++index).asSymbol,{
                RandSeed.ir(1,12345);Out.ar(0,cls.perform(rate,density,0.1).asAudioRateInput);
            });
        };
        write.(("m6_"++cls.name++"_"++rate++"_step").asSymbol,{|gate=37|
            RandSeed.ir(1,12345);Out.ar(0,cls.perform(rate,gate,0.1).asAudioRateInput);
        },[[0.25,[\n_set,1000,\gate,1000]]]);
    };
};
[\ar,\kr].do{|rate|
    [0,17,-17,440,-440,44100,90000].do{|freq,index|
        [0,0.3,-0.2].do{|phase,pindex|
            write.(("m6_Impulse_"++rate++"_"++index++"_"++pindex).asSymbol,{
                Out.ar(0,Impulse.perform(rate,freq,phase,0.1).asAudioRateInput);
            });
        };
    };
    write.(("m6_Impulse_"++rate++"_step").asSymbol,{|gate=17|
        Out.ar(0,Impulse.perform(rate,gate,0,0.1).asAudioRateInput);
    },[[0.25,[\n_set,1000,\gate,-53]]]);
};
write.(\m6_Impulse_ar_exact_fm,{
    Out.ar(0,Impulse.ar(LFPulse.ar(71,0,0.3,4000,-2000),0.2,0.1));
});
write.(\m6_Impulse_ar_fm,{
    Out.ar(0,Impulse.ar(SinOsc.ar(71,0,2000,30),0.2,0.1));
});
[Decay,Decay2].do{|cls|
    [\ar,\kr].do{|rate|
        [0,0.01,0.1,1].do{|dur,index|
            write.(("m6_"++cls.name++"_"++rate++"_"++index).asSymbol,{
                var trig=LFPulse.perform(rate,17,0,0.005,0.1);
                Out.ar(0,if(cls==Decay,{cls.perform(rate,trig,dur)},{cls.perform(rate,trig,dur*0.1,dur)}).asAudioRateInput);
            });
        };
        write.(("m6_"++cls.name++"_"++rate++"_step").asSymbol,{|gate=0.01|
            var trig=LFPulse.perform(rate,17,0,0.005,0.1);
            Out.ar(0,if(cls==Decay,{cls.perform(rate,trig,gate)},{cls.perform(rate,trig,gate*0.1,gate)}).asAudioRateInput);
        },[[0.25,[\n_set,1000,\gate,0.1]]]);
    };
};
[\ar,\kr].do{|rate|
    [[0,0.1],[0.1,0.1],[0.100001,0.1],[0.000001,0.000002]].do{|times,index|
        write.(("m6_Decay2_"++rate++"_edge"++index).asSymbol,{
            Out.ar(0,Decay2.perform(rate,LFPulse.perform(rate,17,0,0.005,0.1),times[0],times[1]).asAudioRateInput);
        });
    };
};
"Extension fixtures exported".postln;
0.exit;
)
