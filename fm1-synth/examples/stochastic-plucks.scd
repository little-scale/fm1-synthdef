// Knobs: event rate, probability, decay, random pitch range.
SynthDef(\stochasticPlucks, {
    arg freq=220, amp=0.5, gate=1, knob1=0.3, knob2=0.7, knob3=0.3, knob4=0.5;
    var trig, degree, env, pluck, sound;
    trig = CoinGate.kr(knob2, Impulse.kr(2 + (knob1 * 24)));
    degree = TIRand.kr(0, 4 + (knob4 * 20), trig);
    pluck = Decay2.kr(trig, 0.005, 0.05 + (knob3 * 0.5));
    sound = SinOsc.ar(freq * degree.midiratio) * pluck;
    env = EnvGen.kr(Env.asr(0.01, 1, 0.3), gate, doneAction: 2);
    Out.ar(0, Pan2.ar(sound * env * amp * 0.2));
})
