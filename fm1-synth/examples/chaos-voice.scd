// Knobs: iteration rate, Henon coefficient, filter cutoff, Cusp blend.
SynthDef(\chaosVoice, {
    arg freq=220, amp=0.5, gate=1, knob1=0.4, knob2=0.7, knob3=0.6, knob4=0.2;
    var henon, cusp, sound, env;
    henon = HenonC.ar(freq * (1 + (knob1 * 8)), 1.1 + (knob2 * 0.3), 0.3);
    cusp = CuspL.ar(freq * 3, 1, 1.9);
    sound = LPF.ar(LeakDC.ar((henon * (1 - knob4)) + (cusp * knob4)), Lag.kr(200 + (knob3 * 5000), 0.03));
    env = EnvGen.kr(Env.asr(0.01, 1, 0.4), gate, doneAction: 2);
    Out.ar(0, Pan2.ar(sound * env * amp * 0.12));
})
