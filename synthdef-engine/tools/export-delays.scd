(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
    var def=SynthDef(name,fn);
    def.writeDefFile(dir);
    Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
    if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[[0.25,1],[0.4,0]]")})};
};
[DelayN,DelayL,DelayC,CombN,CombL,CombC,AllpassN,AllpassL,AllpassC].do{|cls|
    var feedback=cls.name.asString.beginsWith("Delay").not;
    [\ar,\kr].do{|rate|
        ["fixed","step","decay","negative","zero"].do{|mode|
            write.(("m4_"++cls.name++"_"++rate++"_"++mode).asSymbol,{|gate=0|
                var input=SinOsc.perform(rate,if(rate==\ar,{440},{11}))*0.1;
                var time=if(mode=="step",{0.00731+(gate*0.0054)},{if(mode=="zero",{0},{0.00731})});
                var decay=if(mode=="negative",{-0.13},{if(mode=="decay",{0.13+(gate*0.1)},{0.13})});
                var output=if(feedback,{cls.perform(rate,input,0.03,time,decay)},{cls.perform(rate,input,0.03,time)});
                Out.ar(0,Pan2.ar(if(rate==\kr,{K2A.ar(output)},{output})))
            },if(["step","decay"].includes(mode),{[[0.25,[\n_set,1000,\gate,1]],[0.4,[\n_set,1000,\gate,0]]]},{#[]}));
        };
    };
    write.(("m4_"++cls.name++"_audio").asSymbol,{
        var time=0.00731+(SinOsc.ar(3)*0.001);
        var input=SinOsc.ar(440)*0.1;
        Out.ar(0,Pan2.ar(if(feedback,{cls.ar(input,0.03,time,0.13)},{cls.ar(input,0.03,time)})))
    });
};
"Delay fixtures exported".postln;
0.exit;
)
