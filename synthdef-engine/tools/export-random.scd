(
var dir=thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write={|name,fn,events=#[]|
    var def=SynthDef(name,fn);
    def.writeDefFile(dir);
    Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,name,1000,0,1]]] ++ events ++ [[0.51,[\g_freeAll,1]],[0.52,[0]]]).writeOSCFile(dir +/+ name ++ ".osc");
    if(events.size>0){File.use(dir +/+ name ++ ".events.json","w",{|f|f.write("[[0.25,4]]")})};
};
[Rand,ExpRand].do{|cls|
    [[0.1,0.9],[-0.9,-0.1],[0.9,0.1],[0.3,0.3]].do{|bounds,index|
        write.(("m5_"++cls.name++"_"++index).asSymbol,{
            RandSeed.ir(1,12345);
            Out.ar(0,DC.ar(Array.fill(2,{cls.new(bounds[0],bounds[1])})));
        });
    };
    write.(("m5_"++cls.name++"_control").asSymbol,{|gate=1|
        RandSeed.ir(1,12345);
        Out.ar(0,DC.ar(cls.new(0.1,gate)));
    },[[0.25,[\n_set,1000,\gate,4]]]);
};
write.(\m5_random_noise,{
    var seed=RandSeed.ir(1,12345);
    var a=Rand(-0.5,0.5),b=ExpRand(0.01,0.2);
    Out.ar(0,[WhiteNoise.ar(0.1)+a, PinkNoise.ar(b)]);
});
// Keep the user's source as the integration fixture; add a seed before its first draw.
{
    var source=File.readAllString(thisProcess.nowExecutingPath.dirname.dirname.dirname +/+ "fm1-synth/examples/addy-5.scd");
    var def=source.replace("\\addy,","\\m5_addy,").replace("var lfo_depth", "var seed = RandSeed.ir(1,12345);\n\t\tvar lfo_depth").interpret;
    def.writeDefFile(dir);
    Score([[0,[\d_recv,def.asBytes]],[0,[\s_new,def.name,1000,0,1]],[1.2,[\g_freeAll,1]],[1.21,[0]]]).writeOSCFile(dir +/+ "m5_addy.osc");
}.value;
"Random fixtures exported".postln;
0.exit;
)
