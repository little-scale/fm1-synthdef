SynthDef(\twoOpFM, {
    arg freq=440, amp=0.5, gate=1,
        knob1=0.2, knob2=0.35;
    var ratio, index, env, modFreq, mod, sound;

    // Knob 1: modulator/carrier ratio, 0.5 to 8.
    ratio = Lag.kr(0.5 + (knob1 * 7.5), 0.03);
    // Knob 2: FM index, from pure sine to bright FM.
    index = Lag.kr(knob2 * knob2 * 5, 0.03);
    env = EnvGen.kr(
        Env.adsr(0.01, 0.3, 0.6, 0.45),
        gate, doneAction: 2
    );

    modFreq = freq * ratio;
    mod = SinOsc.ar(modFreq, 0, modFreq * index * env);
    sound = SinOsc.ar(freq + mod);
    Out.ar(0, Pan2.ar(sound * env * amp * 0.15));
})
