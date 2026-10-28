SynthDef(\dusty, {
    arg freq=440, amp=0.5, knob1=0.2, knob2=0.5, gate=1;
    var sig, env;

    env = EnvGen.ar(Env.perc(0.01, 5), doneAction: 2);
    sig = SinOsc.ar(freq);
    sig = sig * sig * env * amp;

    Out.ar(0, Splay.ar(Decay2.ar(Dust.ar((knob2 * 10) ! 5), 0.01, knob1)) * sig);
})
