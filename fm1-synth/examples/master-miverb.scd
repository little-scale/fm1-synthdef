SynthDef(\masterMiVerb, {
    arg knob3 = 0.3, knob4 = 0.5, freeze = 0;
    var dry, mix, time;
    dry = In.ar(0, 2);
    mix = Lag.kr(knob3, 0.03) * 0.6;
    time = 0.45 + Lag.kr(knob4, 0.03) * 0.5;
    Out.ar(0, MiVerb.ar(dry, time, mix, 0.5, 0.05, freeze, 0.625));
})
