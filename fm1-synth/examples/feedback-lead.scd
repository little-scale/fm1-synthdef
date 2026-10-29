// Knobs: oscillator feedback, filter cutoff, resonance, release.
SynthDef(\feedbackLead, {
    arg freq=220, amp=0.5, gate=1, knob1=0.3, knob2=0.6, knob3=0.3, knob4=0.4;
    var sig, env;
    sig = SinOscFB.ar(freq, Lag.kr(knob1 * 3.5, 0.03));
    sig = MoogFF.ar(sig, Lag.kr(100 + (knob2 * 6000), 0.03), knob3 * 3.5);
    env = EnvGen.kr(Env.asr(0.01, 1, 0.05 + knob4), gate, doneAction: 2);
    Out.ar(0, Pan2.ar(sig * env * amp * 0.2));
})
