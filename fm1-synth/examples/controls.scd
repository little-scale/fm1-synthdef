// Contract test: direct control outputs make live knob changes measurable.
SynthDef(\control_contract, {
    arg freq=440, midinote=69, note=9, pitch=69,
        amp=0.5, velocity=0.5, gate=1,
        knob1=0.25, knob2=0.5, knob3=0.75, knob4=1;
    Out.ar(0, [K2A.ar(knob1), K2A.ar(knob2)]);
})
