SynthDef(\masterEcho, {
    arg knob5=0.25, knob6=0.5;
    var dry, echo, amount, time;
    dry = In.ar(0, 2);
    amount = Lag.kr(knob5, 0.03);
    time = Lag.kr(0.04 + (knob6 * 0.26), 0.03);
    echo = CombC.ar(
        (dry[0] + dry[1]) * 0.5,
        0.3, time, 1.2
    );
    Out.ar(0, dry + (echo * amount));
})
