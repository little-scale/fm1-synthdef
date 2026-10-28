SynthDef(\keys, {
    arg freq=440, amp=0.5, gate=1,
        knob1=0.5, knob2=0.3,
        knob3=0.2, knob4=0.5;
    var cutoff, env, sound;
    cutoff = 200 + (Lag.kr(knob1, 0.02) * 6000);
    env = EnvGen.ar(
        Env.adsr(0.01, 0.1, 0.7, 0.3),
        gate, doneAction: 2
    );
    sound = Saw.ar(freq);
    sound = RLPF.ar(sound, cutoff,
        0.1 + (knob2 * 0.8));
    sound = sound + (CombC.ar(sound,
        0.1, 0.07, 0.4) * knob3);
    Out.ar(0, Pan2.ar(
        sound * env * amp * 0.12,
        (knob4 * 2) - 1
    ));
})
