// Knobs: pulse shape, cutoff, noise movement, envelope release.
SynthDef(\modulationLab, { |freq=220, amp=0.5, gate=1,
    knob1=0.5, knob2=0.5, knob3=0.2, knob4=0.3|
    var shape=0.05 + (knob1 * 0.9);
    var movement=LFDNoise1.kr(0.5 + (knob3 * 12), 0.008, 1);
    var sig=VarSaw.ar(freq * movement, 0, shape);
    var cutoff=Lag3.kr(LinExp.kr(knob2, 0, 1, 100, 9000), 0.1);
    var env=EnvGen.ar(Env.asr(0.01, 1, 0.05 + (knob4 * 2)), gate, doneAction:2);
    sig=LPF.ar(sig, cutoff) * env * amp * 0.2;
    Out.ar(0, Pan2.ar(sig));
})
