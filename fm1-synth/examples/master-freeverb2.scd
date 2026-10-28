SynthDef(\masterFreeVerb2, {
    arg knob3 = 0.3, knob4 = 0.5;
    var dry, mix, room;
    dry = In.ar(0, 2);
    mix = Lag.kr(knob3, 0.03) * 0.6;
    room = 0.2 + Lag.kr(knob4, 0.03) * 0.75;
    Out.ar(0, FreeVerb2.ar(dry[0], dry[1], mix, room, 0.5));
})
