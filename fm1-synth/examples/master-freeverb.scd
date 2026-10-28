SynthDef(\masterFreeVerb, {
    arg knob3 = 0.3, knob4 = 0.5;
    var dry, wet, mix, room;
    dry = In.ar(0, 2);
    mix = Lag.kr(knob3, 0.03) * 0.6;
    room = 0.2 + Lag.kr(knob4, 0.03) * 0.75;
    wet = FreeVerb.ar((dry[0] + dry[1]) * 0.5, 1, room, 0.5);
    Out.ar(0, dry * (1 - mix) + wet * mix);
})
