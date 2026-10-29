// Each keyboard semitone advances one degree of a just-intonation major scale.
// Knob 1: scale-degree transpose; 2: formant; 3: attack; 4: release.
SynthDef(\justFormant, { |midinote=60, amp=0.5, gate=1,
    knob1=0, knob2=0.3, knob3=0.1, knob4=0.2|
    var scale=Scale.major(Tuning.just);
    var table=scale.as(LocalBuf);
    var degree=(midinote-60) + (knob1*14).round(1);
    var freq=(DegreeToKey.kr(table, degree, 12)+48).midicps;
    var env=EnvGen.ar(Env.asr(0.005+(knob3*0.5), 1,
        0.05+(knob4*1.5)), gate, doneAction:2);
    var sig=Formant.ar(freq, 300+(knob2*2700), freq*2);
    Out.ar(0, Pan2.ar(sig*env*amp*0.12));
})
