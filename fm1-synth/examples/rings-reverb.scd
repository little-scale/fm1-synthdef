// MiRings6: one Rings voice per keyboard note. Knobs: structure, brightness, damping, position.
// Original model delay lengths and precision.
SynthDef(\rings6, {
    arg midinote=60, amp=0.5, gate=1, knob1=0.4, knob2=0.6, knob3=0.5, knob4=0.4;
    var sound, env;
    sound = MiRings6.ar(0, 1, midinote - 12,
        knob1, knob2, knob3, knob4);
    env = EnvGen.kr(Env.asr(0.005, 1, 0.4), gate, doneAction: 2);
    Out.ar(0, sound * env * amp * 0.3);
})
