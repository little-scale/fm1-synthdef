SynthDef(\masterMiVerb, {
    arg knob5 = 0.3, knob6 = 0.5, freeze = 0;
    var dry, mix, time;
    dry = In.ar(0, 2);
    mix = Lag.kr(knob5, 0.03) * 0.6;
    time = 0.45 + Lag.kr(knob6, 0.03) * 0.5;
    Out.ar(0, MiVerb.ar(dry, time, mix, 0.5, 0.05, freeze, 0.625));
})
