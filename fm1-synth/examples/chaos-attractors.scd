// Knobs: Lorenz time step, iteration rate, Latoocarfian blend, filter cutoff.
SynthDef(\chaosAttractors, {
    arg freq=220, amp=0.5, gate=1, knob1=0.5, knob2=0.3, knob3=0.5, knob4=0.7;
    var lorenz, lato, sound, env;
    lorenz = LorenzL.ar(freq * (1 + (knob2 * 12)), 10, 28, 2.667, 0.01 + (knob1 * 0.04));
    lato = LatoocarfianC.ar(freq * 4, 1, 3, 0.5, 0.5);
    sound = LPF.ar(LeakDC.ar((lorenz * (1 - knob3)) + (lato * knob3)), Lag.kr(200 + (knob4 * 5000), 0.03));
    env = EnvGen.kr(Env.asr(0.03, 1, 0.6), gate, doneAction: 2);
    Out.ar(0, Pan2.ar(sound * env * amp * 0.12));
})
