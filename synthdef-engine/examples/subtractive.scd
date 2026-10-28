// Host profile 2 example. Evaluate in desktop SuperCollider to export.
(
SynthDef(\fm1_subtractive, { |freq=110, gate=1, amp=0.1, pan=0|
    var envelope = EnvGen.ar(Env.adsr(0.01, 0.08, 0.6, 0.15), gate, doneAction: 2);
    var cutoff = 300 + (EnvGen.kr(Env.perc(0.01, 0.3), gate) * 4000) + (SinOsc.kr(5) * 80);
    var signal = (Saw.ar(freq) + Pulse.ar(freq * 0.501, 0.35)) * 0.2;
    Out.ar(0, Pan2.ar(RLPF.ar(signal, cutoff, 0.25) * envelope * amp, pan));
}).writeDefFile(thisProcess.nowExecutingPath.dirname);
)
