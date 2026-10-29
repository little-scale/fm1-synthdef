// Master effect: FX knob 5 = drive, FX knob 6 = wet/dry.
SynthDef(\muDrive, { |knob5=0.2, knob6=0.5|
    var dry=In.ar(0, 2);
    var wet=MiMu.ar(dry, 1 + (knob5 * 15));
    Out.ar(0, dry + ((wet - dry) * knob6));
})
