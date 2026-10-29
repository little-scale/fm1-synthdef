// Knobs: decay, damping, filter cutoff, attack. Delay allocation supports >= 25 Hz.
SynthDef(\pluckedString, {
    arg freq=220, amp=0.5, gate=1, knob1=0.4, knob2=0.5, knob3=0.7, knob4=0.1;
    var sig, env;
    sig = Pluck.ar(WhiteNoise.ar(0.4), gate, 0.04, freq.max(25).reciprocal,
        0.1 + (knob1 * 4), knob2 * 0.9);
    sig = LPF.ar(LeakDC.ar(sig), Lag.kr(200 + (knob3 * 6000), 0.03));
    env = EnvGen.kr(Env.asr(0.001 + (knob4 * 0.05), 1, 0.3), gate, doneAction: 2);
    Out.ar(0, Pan2.ar(sig * env * amp));
})
