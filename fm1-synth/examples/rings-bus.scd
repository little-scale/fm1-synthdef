// Persistent string/reverb effect. Knobs 5–8: pitch, structure, brightness, damping.
SynthDef(\ringsBus, {
    arg knob5=0.4, knob6=0.4, knob7=0.6, knob8=0.5;
    var dry = In.ar(0, 2);
    var wet = MiRings6.ar(Mix.ar(dry) * 0.3, 0,
        24 + (knob5 * 48), knob6, knob7, knob8, 0.4);
    Out.ar(0, dry * 0.7 + (wet * 0.3));
})
