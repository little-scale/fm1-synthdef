SynthDef(\tripleSaw, {
    arg freq=440, amp=0.5, gate=1,
        knob1=0.25, knob2=0.3,
        knob3=0.25, knob4=0.5;
    var time, env, detune, vibrato;
    var pitch, cutoff, sound;

    // Knob 1 lengthens both attack and release.
    time = knob1 * knob1;
    env = EnvGen.kr(
        Env.adsr(0.005 + (time * 1.5),
            0.15, 0.7, 0.06 + (time * 3)),
        gate, doneAction: 2
    );

    detune = Lag.kr(knob2, 0.03) * 0.018;
    vibrato = SinOsc.kr(5, 0,
        Lag.kr(knob3, 0.03) * 0.006);
    pitch = freq * (1 + vibrato);
    sound = LFSaw.ar(pitch *
        [1 - detune, 1, 1 + detune]).sum / 3;

    cutoff = 180 + (env *
        (600 + (Lag.kr(knob4, 0.03) * 10000)));
    sound = RLPF.ar(sound, cutoff, 0.3);
    Out.ar(0, Pan2.ar(sound * env * amp * 0.12));
})
