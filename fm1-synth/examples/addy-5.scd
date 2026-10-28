SynthDef.new(\addy,
	{
		arg freq = 220, amp = 0.5, pan = 0, out = 0, gate = 1, attack = 0.01, dur = 1, output = 0, low_harmonic = 0, high_harmonic = 5, timer = 1, randamp = 10000;
		var sound, env, max_harmonics = 5, amp_array = [max_harmonics], unmute_array = [max_harmonics];
		var lfo_depth = Rand(0.0, 1);

		var lfo = SinOsc.ar(Rand(0.1, 1), 90.degrad, lfo_depth, 1.0 - lfo_depth);

		amp_array = { ExpRand(1, randamp) } ! max_harmonics;
		amp_array = amp_array.normalizeSum;

		env = Env.perc(attack, dur, 1, -4);

		Array.fill(max_harmonics, {
			arg index;
			var signal, envelope;

			envelope = EnvGen.ar(env, gate, 1, 0, timer * dur * (1 / (index + 1)) * Rand(0.9, 1.1)) ; // leaving doneAction as 0...

			signal = SinOsc.ar(freq * (index + 1) * Rand(0.993, 1.007), 0, amp_array[index] * (1 / (index + 1)));

			signal = signal * (index < high_harmonic);
			signal = signal * (index >= low_harmonic);

			signal = signal * envelope * amp * lfo;

			signal = Pan2.ar(signal, Rand(-1.0, 1.0));

			Out.ar(output, signal)
		});

		Line.ar(0, 1, (attack * high_harmonic + dur) * timer, 0, 0, 2);
	}
)
