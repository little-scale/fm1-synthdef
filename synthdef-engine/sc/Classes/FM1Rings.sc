// GPL-3.0-or-later. FM1 fixed-model wrappers, one internal Rings voice.
// Model numbers 1..6 map to upstream MiRings model 0..5; no Easter egg modes.
// Explicit positive-edge trigger; default 1 excites a new synth once.
// pit retains upstream's +12-semitone tonic: use midinote - 12 for keyboard pitch.
MiRings1 : MultiOutUGen {
    *ar { |in=0, trig=1, pit=60, struct=0.25, bright=0.5, damp=0.7, pos=0.25,
        intern_exciter=0, bypass=0, compact=0, mul=1, add=0|
        ^this.multiNew('audio', in, trig, pit, struct, bright, damp, pos,
            intern_exciter, bypass, compact).madd(mul, add);
    }
    init { |...inputs| this.inputs = inputs; ^this.initOutputs(2, rate); }
}
MiRings2 : MiRings1 {}
MiRings3 : MiRings1 {}
MiRings4 : MiRings1 {}
MiRings5 : MiRings1 {}
MiRings6 : MiRings1 {}
