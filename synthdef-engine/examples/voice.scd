// Export this on the Mac. Import the resulting file with the host inspector.
// Compatible with host profile 2; hardware upload is a later milestone.
(
SynthDef(\fm1_voice, { |freq=440, amp=0.1, gate=1, pan=0|
    var envelope = EnvGen.ar(Env.adsr(0.01, 0.08, 0.5, 0.15), gate, doneAction: 2);
    Out.ar(0, Pan2.ar(SinOsc.ar(freq) * envelope * amp, pan));
}).writeDefFile(thisProcess.nowExecutingPath.dirname);
)
