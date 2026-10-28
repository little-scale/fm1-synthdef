SynthDef(\keys, {
    arg freq=440, amp=0.5, gate=1,
        knob1=0.5, knob2=0.3;
    var cutoff, env, sound;
    cutoff = 200 + (Lag.kr(knob1, 0.02) * 6000);
    env = EnvGen.ar(
        Env.adsr(0.01, 0.1, 0.7, 0.3),
        gate, doneAction: 2
    );
    sound = Saw.ar(freq);
    sound = RLPF.ar(sound, cutoff,
        0.1 + (knob2 * 0.8));
    Out.ar(0, Pan2.ar(sound * env * amp * 0.12, 0));
})
