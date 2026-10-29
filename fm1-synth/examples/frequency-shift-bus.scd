// FX knobs 5–8: shift, wet/dry, stereo spread, output level.
SynthDef(\frequencyShiftBus, {
    arg knob5=0.5, knob6=0.3, knob7=0.4, knob8=0.8;
    var dry, shifted, hz, wet, level;
    dry = In.ar(0, 2);
    hz = Lag.kr((knob5 * 400) - 200, 0.03);
    shifted = FreqShift.ar(dry, [hz, hz * (1 - (knob7 * 2))]);
    wet = Lag.kr(knob6, 0.03);
    level = Lag.kr(0.1 + (knob8 * 0.8), 0.03);
    Out.ar(0, Limiter.ar(((dry * (1 - wet)) + (shifted * wet)) * level, 0.9, 0.003));
})
