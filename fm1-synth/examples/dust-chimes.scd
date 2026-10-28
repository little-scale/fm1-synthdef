SynthDef(\dustChimes, {
    arg freq=440, amp=0.5, gate=1,
        knob1=0.4, knob2=0.35;
    var density, decay, hit, pitch, sound, env;
    density = 2 + (Lag.kr(knob1, 0.03) * 38);
    decay = 0.03 + (Lag.kr(knob2, 0.03) * 0.7);
    hit = Dust.ar(density);
    pitch = TRand.ar(freq * 0.5, freq * 3, hit);
    sound = SinOsc.ar(pitch) * Decay2.ar(hit, 0.002, decay);
    sound = OnePole.ar(sound, 0.2);
    env = EnvGen.kr(Env.asr(0.01, 1, 0.3), gate, doneAction: 2);
    Out.ar(0, Pan2.ar(sound * env * amp * 0.2));
})
