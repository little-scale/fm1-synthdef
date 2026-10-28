// Run from the workspace: sclang -D synthdef-engine/tools/export-fixtures.scd
(
var dir = thisProcess.nowExecutingPath.dirname.dirname +/+ "fixtures";
var write = { |name, fn, events = #[]|
    var def = SynthDef(name, fn);
    var score = Score([[0, [\d_recv, def.asBytes]], [0, [\s_new, name, 1000, 0, 1]]] ++ events ++ [[0.51, [\g_freeAll, 1]], [0.52, [0]]]);
    def.writeDefFile(dir);
    score.writeOSCFile(dir +/+ name ++ ".osc");
};
write.("tone", { |freq=440, amp=0.1, pan=0| Out.ar(0, Pan2.ar(SinOsc.ar(freq) * amp, pan)) });
write.("perc", { Out.ar(0, Pan2.ar(SinOsc.ar(440) * EnvGen.ar(Env.perc(0.01, 0.15), doneAction:2) * 0.1)) });
write.("adsr", { |gate=1| Out.ar(0, Pan2.ar(EnvGen.ar(Env.adsr(0.01, 0.05, 0.4, 0.08), gate))) }, [[0.25, [\n_set, 1000, \gate, 0]]]);
write.("forced", { |gate=1| Out.ar(0, Pan2.ar(EnvGen.ar(Env.asr(0.01, 0.6, 0.2), gate))) }, [[0.25, [\n_set, 1000, \gate, -1.05]]]);
write.("retrigger", { |gate=1| Out.ar(0, Pan2.ar(EnvGen.ar(Env.asr(0.01, 0.6, 0.08), gate))) }, [[0.1,[\n_set,1000,\gate,0]], [0.125,[\n_set,1000,\gate,1]], [0.3,[\n_set,1000,\gate,0]]]);
write.("loop", { |gate=1| Out.ar(0, Pan2.ar(EnvGen.ar(Env([0,1,0.3,0], [0.02,0.03,0.04], \lin, 2, 0),gate))) }, [[0.25,[\n_set,1000,\gate,0]]]);
[\step, \lin, \exp, \sin, \welch, -4, \squared, \cubed, \hold].do { |curve, i|
    write.(("curve" ++ i).asSymbol, { Out.ar(0, Pan2.ar(EnvGen.ar(Env([0.1, 0.8, 0.2], [0.05,0.1], curve)))) });
};
[\perc, \adsr, \dadsr, \asr, \cutoff, \linen, \triangle, \sine].do { |constructor|
    write.(("env_" ++ constructor).asSymbol, { |gate=1| Out.ar(0, Pan2.ar(EnvGen.ar(Env.perform(constructor), gate))) }, [[0.25,[\n_set,1000,\gate,0]]]);
};
write.("control_env", { Out.ar(0, Pan2.ar(K2A.ar(EnvGen.kr(Env.perc(0.01,0.1))))) });
write.("line", { Out.ar(0, Pan2.ar(Line.ar(0.1,0.9,0.1))) });
write.("xline", { Out.ar(0, Pan2.ar(XLine.ar(0.1,0.9,0.1))) });
write.("resonz", { Out.ar(0, Pan2.ar(Resonz.ar(SinOsc.ar(440),440,0.1))) });
write.("ringz", { Out.ar(0, Pan2.ar(Ringz.ar(SinOsc.ar(440)*0.001,440,0.05))) });
write.("klang", { Out.ar(0, Pan2.ar(Klang.ar(`[[220,440,880],[0.1,0.05,0.025],[0,0.3,0.7]]))) });
write.("klank", { Out.ar(0, Pan2.ar(Klank.ar(`[[220,440,880],[0.1,0.05,0.025],[0.05,0.1,0.2]],SinOsc.ar(440)*0.001))) });
write.("dynklang", { Out.ar(0, Pan2.ar(DynKlang.ar(`[[220,440,880],[0.1,0.05,0.025],[0,0.3,0.7]]))) });
write.("dynklank", { Out.ar(0, Pan2.ar(DynKlank.ar(`[[220,440,880],[0.1,0.05,0.025],[0.05,0.1,0.2]],SinOsc.ar(440)*0.001))) });
write.("arithmetic", { |a=0.2, b=0.3| Out.ar(0, Pan2.ar((SinOsc.ar(110).abs + SinOsc.ar(220).neg + SinOsc.ar(330) + SinOsc.ar(440)) * (a / b))) });
// Additional rate, modulation and boundary fixtures.
write.("phase_control", { |gate=0| Out.ar(0, Pan2.ar(SinOsc.ar(440,gate))) }, [[0.25,[\n_set,1000,\gate,1]]]);
write.("pan_control", { |gate=0| Out.ar(0, Pan2.ar(SinOsc.ar(440),gate)) }, [[0.25,[\n_set,1000,\gate,0.6]]]);
write.("pan_audio", { Out.ar(0, Pan2.ar(SinOsc.ar(440),SinOsc.ar(4))) });
write.("phase_audio", { Out.ar(0, Pan2.ar(SinOsc.ar(440,SinOsc.ar(4)))) });
write.("sinosc_control", { Out.ar(0, Pan2.ar(K2A.ar(SinOsc.kr(4)))) });
write.("a2k", { Out.ar(0, Pan2.ar(K2A.ar(A2K.kr(SinOsc.ar(440))))) });
write.("dc", { Out.ar(0, Pan2.ar(DC.ar(0.25))) });
write.("line_control", { Out.ar(0, Pan2.ar(K2A.ar(Line.kr(0.1,0.9,0.1)))) });
write.("xline_control", { Out.ar(0, Pan2.ar(K2A.ar(XLine.kr(0.1,0.9,0.1)))) });
write.("resonz_control", { Out.ar(0, Pan2.ar(K2A.ar(Resonz.kr(SinOsc.kr(20),20,0.2)))) });
write.("ringz_control", { Out.ar(0, Pan2.ar(K2A.ar(Ringz.kr(SinOsc.kr(20)*0.01,20,0.1)))) });
write.("zero_segments", { Out.ar(0, Pan2.ar(EnvGen.ar(Env([0,1,0.5,0],[0,0,0])))) });
write.("audio_gate", { Out.ar(0, Pan2.ar(EnvGen.ar(Env.asr(0.01,0.6,0.02),SinOsc.ar(10)))) });
[\step, \lin, \exp, \sin, \welch, -4, \squared, \cubed, \hold].do { |curve, i|
    write.(("curve_kr" ++ i).asSymbol, { Out.ar(0, Pan2.ar(K2A.ar(EnvGen.kr(Env([0.1,0.8,0.2],[0.05,0.1],curve))))) });
};
[\neg, \abs, \ceil, \floor, \frac, \sign, \squared, \cubed, \sqrt, \exp, \reciprocal, \midicps, \log, \log2, \log10, \sin, \cos, \tan, \asin, \distort, \softclip].do { |op|
    write.(("unary_" ++ op.specialIndex).asSymbol, { Out.ar(0, Pan2.ar(DC.ar(0.75).perform(op))) });
};
['+', '-', '*', '/', '==', '!=', '<', '>', '<=', '>=', \min, \max].do { |op|
    write.(("binary_" ++ op.specialIndex).asSymbol, { Out.ar(0, Pan2.ar(BinaryOpUGen.new(op,DC.ar(0.25),DC.ar(0.75)))) });
};
write.("fm1_voice", { |freq=440, amp=0.1, gate=1, pan=0|
    Out.ar(0, Pan2.ar(SinOsc.ar(freq)*EnvGen.ar(Env.adsr(0.01,0.08,0.5,0.15),gate,doneAction:2)*amp,pan))
}, [[0.25,[\n_set,1000,\gate,0]]]);
write.("sum4", { Out.ar(0, Pan2.ar(Sum4.new(SinOsc.ar(110),SinOsc.ar(220),SinOsc.ar(330),SinOsc.ar(440)))) });
"FM fixtures exported".postln;
0.exit;
)
