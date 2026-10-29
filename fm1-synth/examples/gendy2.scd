// Knobs: amplitude movement, duration movement, active breakpoints, cutoff.
SynthDef(\gendy2, { |freq=220, amp=0.5, gate=1,
    knob1=0.3, knob2=0.3, knob3=0.4, knob4=0.5|
    var movement=0.02 + (knob1 * 0.4);
    var variation=0.02 + (knob2 * 0.4);
    var points=(2 + (knob3 * 10)).round;
    var sig=Gendy2.ar(1, 1, 0.7, 0.6, freq, freq * 1.1, movement, variation, 12, points);
    var env=EnvGen.ar(Env.asr(0.02, 1, 0.3), gate, doneAction:2);
    // Stochastic breakpoints can overshoot: soft clipping keeps headroom.
    sig=LPF.ar(LeakDC.ar(sig.softclip), 200 + (knob4 * 7000));
    Out.ar(0, Pan2.ar(sig * env * amp * 0.15));
})
