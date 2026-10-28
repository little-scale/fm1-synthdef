(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
    var def=SynthDef(name,fn);
    def.writeDefFile(dir);
    if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[" ++ events.collect{|e|"["++e[0]++","++e[1][3]++"]"}.join(",") ++ "]")})};
    Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[4.01,[\g_freeAll,1]],[4.02,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
};
[FreeVerb,FreeVerb2,MiVerb].do{|cls|
    var verb={|left,right,mix=0.5,room=0.7,damp=0.5|
        if(cls==FreeVerb,{FreeVerb.ar(left,mix,room,damp)},
            {if(cls==FreeVerb2,{FreeVerb2.ar(left,right,mix,room,damp)},
                {MiVerb.ar([left,right],room,mix,damp)})});
    };
    [0,0.5,1].do{|mix,index|
        write.(("m7_"++cls.name++"_impulse"++index).asSymbol,{
            Out.ar(0,verb.(Impulse.ar(0,0,0.5),DC.ar(0),mix));
        });
    };
    [[0,0],[0,1],[1,0],[1,1],[-1,-1],[2,2]].do{|params,index|
        write.(("m7_"++cls.name++"_parameters"++index).asSymbol,{
            Out.ar(0,verb.(Impulse.ar(7,0,0.1),Impulse.ar(11,0.3,0.05),0.7,params[0],params[1]));
        });
    };
    write.(("m7_"++cls.name++"_noise").asSymbol,{
        var env=EnvGen.kr(Env.linen(0.001,0.2,0.05));
        RandSeed.ir(1,12345);
        Out.ar(0,verb.(WhiteNoise.ar(0.1)*env,SinOsc.ar(317,0,0.07)*env));
    });
    write.(("m7_"++cls.name++"_step").asSymbol,{|gate=0.2|
        Out.ar(0,verb.(SinOsc.ar(113,0,0.07),SinOsc.ar(317,0,0.03),gate,gate,gate));
    },[[0.5,[\n_set,1000,\gate,0.9]],[2,[\n_set,1000,\gate,0]]]);
};
write.(\m7_MiVerb_mono,{
    Out.ar(0,MiVerb.ar(Impulse.ar(2,0,0.5),0.7,0.8));
});
write.(\m7_MiVerb_freeze,{|gate=0|
    Out.ar(0,MiVerb.ar([Impulse.ar(3,0,0.2),DC.ar(0)],0.8,1,0.4,0.1,gate));
},[[0.5,[\n_set,1000,\gate,1]],[2,[\n_set,1000,\gate,0]]]);
write.(\m7_MiVerb_diffusion,{|gate=0|
    Out.ar(0,MiVerb.ar(Impulse.ar(2,0,0.3),0.8,0.8,0.5,gate,0,gate));
},[[0.5,[\n_set,1000,\gate,0.9]],[2,[\n_set,1000,\gate,2]]]);
write.(\m7_MiVerb_saturation,{
    Out.ar(0,MiVerb.ar([SinOsc.ar(113,0,4),SinOsc.ar(317,0,3)],1.2,0.8,0.1,0.1,0,0.8));
});
"Reverb fixtures exported".postln;
0.exit;
)
