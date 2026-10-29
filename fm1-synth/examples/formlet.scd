// Knobs: resonant harmonic, resonance attack, resonance decay, note release.
SynthDef(\formletVoice, { |freq=220, amp=0.5, gate=1,
    knob1=0.2, knob2=0.1, knob3=0.4, knob4=0.2|
    var center=(freq * (1 + (knob1 * 7))).min(12000);
    var attack=0.001 + (knob2 * 0.025);
    var decay=0.04 + (knob3 * 0.4);
    var sig=Formlet.ar(Impulse.ar(freq), center, attack, decay).softclip;
    var env=EnvGen.ar(Env.asr(0.01, 1, 0.05 + (knob4 * 1.5)), gate, doneAction:2);
    Out.ar(0, Pan2.ar(sig * env * amp * 0.15));
})
