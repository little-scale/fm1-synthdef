# Oscillators, physical modelling and effects

This expansion adds **12 UGen types**, bringing the development profile to **154**. It includes SinOscFB, Blip, Phasor, Pluck, MoogFF, FreqShift, Limiter, FBSineN/L/C and StandardN/L. It does not change the published 53-type preview firmware or any saved slots. Hardware listening and realtime CPU checks remain deferred.

## Rates and bounded inputs

| Type | Rates | Supported profile |
| --- | --- | --- |
| SinOscFB | ar, kr | Initial/control frequency within ±half calculation rate; initial/control feedback ±8; feedback interpolates between blocks |
| Blip | ar, kr | Initial/control frequency strictly positive and at most half calculation rate; Nyquist/frequency at most 16,777,216; harmonic argument within ±16,777,216, truncated, clamped to at least 1 and at most Nyquist/frequency |
| Phasor | ar, kr | Trigger and increment can be audio rate for `.ar`; audio increment requires an audio trigger; bounds/reset are initial/control, ordered bounds within ±1,000,000 with range at least 0.000001, reset within ±1,000,000; increment magnitude at most 1,024 times the range |
| Pluck | ar | Audio excitation; audio/control trigger and damping; literal nonnegative maximum delay; initial/control delay between zero and maximum, decay ±1,000,000 seconds; damping coefficient ±1 |
| MoogFF | ar, kr | Audio input required for `.ar`; initial/control cutoff from zero to strictly below Nyquist, feedback gain clamped to 0–4, initial/control reset |
| FreqShift | ar | Audio input; shift and phase may be audio rate; shift within ±Nyquist, phase within ±16 radians |
| Limiter | ar | Audio input; initial/control nonnegative level up to 1,000,000; strictly positive literal lookahead duration, subject to memory budget |
| FBSineN/L/C | ar | Initial/control parameters; iteration frequency 0–4×sample rate, other parameters within ±16 |
| StandardN/L | ar | Initial/control parameters; iteration frequency 0–4×sample rate, other parameters within ±16 |

Ordinary SuperCollider defaults, `mul`/`add` where exposed by the class, and multichannel expansion apply within graph limits. Output is not universally normalized: excitation, resonance, initial conditions and mixing still determine level. Unstable/nonfinite or excessive output faults and silences the voice.

`SinOscFB` uses the sine wavetable and feedback order from the pinned source. `Blip` reuses the band-limited harmonic kernel and crossfades harmonic-count changes. `Phasor` retains the distinct control/audio trigger reset timing and uses compensated single-precision accumulation to reduce ramp drift. MoogFF retains its four filter states, reset behavior and gain interpolation, with float coefficients and processing.

FreqShift shifts each spectral component by a fixed number of hertz; it is not a transposing pitch shifter. Its two six-stage allpass paths and oscillator use existing node storage. **Initial control-rate phase is honored deterministically**: the pinned upstream constructor reads its phase-conversion member before initializing it. This port initializes conversion first. Zero-initial-phase fixtures compare against SuperCollider; an independent half-turn inversion test checks the corrected nonzero phase behavior. Later control phase changes and audio phase modulation are supported.

## Memory and latency

Only Pluck and Limiter allocate from the existing DSP arena. No new fixed node/voice storage or reserved RAM is added.

- **Pluck:** the ring length is the next power of two covering `ceil(maxdelaytime * sampleRate + 1) + 64` float samples. Cubic reads have a minimum two-sample delay. The buffer size is fixed at note start. Excitation length follows the pinned implementation, including rounding at half-sample boundaries; negative decay gives negative feedback.
- **Limiter:** three buffers of `ceil(duration * sampleRate)` float samples. The exact three-buffer storage omits upstream allocation padding without changing the algorithm. Lookahead is approximately twice the duration; the upstream constructor consumes one sample, which is included in timing checks. Live threshold changes follow the reference block algorithm and are not an instantaneous hard clip. Array expansion creates independent channel limiters, not a linked stereo detector.

The instrument permits 64 KiB per playable voice and 128 KiB shared across two voices and the selected effect. For example, Pluck's default maximum delay of 0.2 seconds at 44.1 kHz consumes 64 KiB per voice, leaving no shared space for buffered effects when both voices play. The supplied string example uses a 0.04-second maximum and **8,192 bytes per voice**. Its frequency is clamped to at least 25 Hz to stay within that delay range.

The supplied stereo frequency-shift/limiter bus uses **3,192 bytes** at 44.1 kHz. Two supplied string voices plus that effect use **19,576 / 131,072 bytes**. Memory fit does not establish realtime CPU capacity.

## Chaotic feedback

FBSine and Standard use the pinned upstream equations, constructors, interpolation and live initial-condition resets with single-precision arithmetic. FBSine's phase map wraps at 2π; Standard uses the standard-map recurrence. As with the earlier chaotic ports, rounding can change long trajectories and periodic windows. SinOscFB can also become sensitive to tiny rounding differences at high feedback.

Stable cases retain waveform checks. Selected chaotic cases use labeled mean, variance, RMS and roughness checks, backed by independent identical-state calculations against upstream C++ source. A statistical pass does not mean sample identity or perceptual equivalence for every parameter setting.

## Examples

| Patch | Knobs |
| --- | --- |
| [Feedback lead](../fm1-synth/examples/feedback-lead.scd) | 1 feedback, 2 cutoff, 3 resonance, 4 release |
| [Plucked string](../fm1-synth/examples/plucked-string.scd) | 1 decay, 2 damping, 3 cutoff, 4 attack |
| [Harmonic blip](../fm1-synth/examples/harmonic-blip.scd) | 1 harmonic count, 2 cutoff, 3 resonance, 4 release |
| [Frequency-shift bus](../fm1-synth/examples/frequency-shift-bus.scd) | FX 5 shift, 6 wet/dry, 7 stereo spread, 8 output level |

Source, compiled `.scsyndef` and `.fmslot` files are supplied. These are optional imports; none is installed automatically. The bus example belongs in an effect slot. Restart a running local editor to load the rebuilt native engine.

## Reproduction

Run the existing engine and instrument tests, plus `synthdef-engine/tests/test_synthesis.py`, both modes of `synthdef-engine/tests/test_synthesis_oracle.py`, and `fm1-synth/tools/test-synthesis.py`. `export-synthesis.scd` supplies the new reference fixtures; `export-all.py` updates their integrity manifest. The full reference suite and both modes of `fm1-synth/tools/test-device-math.py` check the three rendering backends. `generate-chaos.py` and `generate-chaos-oracle.py` reproduce the chaos port and identical-state tests from pinned sources. See [verification results](VERIFICATION.md) for measured coverage, artifact hashes and remaining hardware checks.
