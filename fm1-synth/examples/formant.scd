// Knobs: formant frequency, bandwidth, attack, release.
SynthDef(\formantVoice, { |freq=220, amp=0.5, gate=1,
    knob1=0.3, knob2=0.3, knob3=0.1, knob4=0.2|
    var center=300 + (knob1 * 4700);
    var bandwidth=freq.max(100 + (knob2 * 4000));
    var env=EnvGen.ar(Env.asr(0.005 + (knob3 * 0.5), 1,
        0.05 + (knob4 * 1.5)), gate, doneAction:2);
    var sig=Formant.ar(freq, center, bandwidth);
    Out.ar(0, Pan2.ar(sig * env * amp * 0.12));
})
