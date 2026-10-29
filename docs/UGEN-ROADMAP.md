# UGen roadmap

Updated 30 October 2026. The development engine contains **154 UGen types**. The 77 additions below are implemented; hardware installation and realtime checks are deferred at the user's request. The existing published preview remains unchanged.

## Completed expansion

| Group | Added types |
| --- | --- |
| Bounds and mappings | Fold, Wrap, LinExp |
| Noise | BrownNoise, ClipNoise, LFDNoise0, LFDNoise1, LFDNoise3, LFClipNoise, LFDClipNoise, GrayNoise, Hasher |
| Random control | CoinGate, TIRand, TWindex |
| Chaotic oscillators | Crackle, Logistic, HenonN/L/C, CuspN/L, LatoocarfianN/L/C, LorenzL, FBSineN/L/C, StandardN/L |
| Triggers and counters | Trig, Trig1, PulseDivider, PulseCount, ToggleFF, SetResetFF, Stepper, Phasor |
| Mixing and panning | XFade2, LinXFade2, Balance2 |
| Smoothing and tracking | Slew, Lag2, Lag3, Amplitude |
| Oscillators | VarSaw, SyncSaw, LFCub, LFPar, Gendy1, Gendy2, Gendy3, Formant, SinOscFB, Blip |
| Filters and EQ | RHPF, BRF, BPeakEQ, BLowShelf, BHiShelf, Formlet, MoogFF |
| Physical strings and effects | Pluck, FreqShift, Limiter |
| Distortion | MiMu |
| Rings models | MiRings1, MiRings2, MiRings3, MiRings4, MiRings5, MiRings6 |
| Scale/tuning tables | MaxLocalBufs, LocalBuf, SetBuf, DegreeToKey |

A further 28 operators now cover signed `pow`, `div`, gcd/lcm, atan2/hypot, ring arithmetic, hyperbolic functions and octave conversions. Scale/Tuning tables use graph constants without increasing the DSP arena. Modulo, round, roundUp, trunc, clip2, fold2 and wrap2 binary selectors were added too. SelectX now compiles and renders at audio/control rate; it is a source helper, not a separate runtime UGen. The [full profile](../synthdef-engine/README.md#supported-profile) states rate and domain restrictions. In particular, the new EQ coefficients are initial/control rate, oscillator frequencies are bounded, and MiMu is audio-only.

The initial 32 types reuse existing per-node state and allocate no delay buffers. Gendy1/2/3 additionally charge bounded breakpoint arrays to the existing shared arena (default 96/192/200 bytes per instance). Formant and Formlet also use existing per-node state and require no arena memory. Static RAM and the instrument pool remain unchanged. A stable difference-state form avoids cancellation in low-cutoff high-pass/notch/EQ filters. Control-frequency SinOsc increments and MiMu's float-to-PCM conversion match the reference without software double routines on the single-precision device.

[MiMu drive](../fm1-synth/examples/mimu-drive.scd) is a stereo master-effect example; [modulation lab](../fm1-synth/examples/modulation-lab.scd) exercises VarSaw, LFDNoise1, LinExp and Lag3 with live knobs. Neither example replaces a saved user slot.

The [18 stochastic/chaotic additions](STOCHASTIC.md) reuse existing state without arena allocation. Their equations and interpolation are ported from the pinned sources; chaotic trajectories intentionally use single precision. Stable waveform, identical-state transition and labeled statistical checks cover different aspects of correctness.

## MiRings feasibility result

The complete pinned upstream implementation was built and run on the Mac. All six resonator models and all six alternate-mode effect settings produced finite, nonzero audio over 131,072 frames each. This is a host smoke test, not a hardware port or performance measurement.

The host probe measured:

| Storage | Bytes |
| --- | ---: |
| Rings Part | 110,888 |
| Alternate string synth | 2,168 |
| Strummer | 908 |
| Reverb buffer | 65,536 |
| Performance/patch state and 64-frame input/silence buffers | 548 |
| Total, excluding wrapper overhead | **180,048** |

The full upstream layout remains too large, but the development engine now supplies **six independently allocated models** as MiRings1–MiRings6. Models 2/5 can use compact 960-sample floating-point string buffers (all eight strings retained), and model 6 removes unused reverb padding without changing its delay lengths. All six have playable configurations within the existing per-voice budget. Shared-effect memory and realtime CPU limits still apply. See [Rings compatibility and memory](RINGS.md) and [verification](VERIFICATION.md).

Reproduce the host probe with `synthdef-engine/tools/probe-mirings.py --upstream PATH --output RESULT.json`, using mi-UGens commit `10f6824f68ad63475f67dfe3fe5016e2d2634f84` including its eurorack sources. The probe verifies source hashes before building. See [probe evidence](../evidence/mirings-feasibility-2026-10-30.json).

The [12 synthesis/effect additions](SYNTHESIS-EXPANSION.md) are now implemented, including the remaining recommended feedback/standard-map generators. Their examples include a stereo frequency-shift/limiter effect.

## Remaining work

- Install the new image, preserve/read back all slots, and check two voices through the chosen master effect with live knobs. Emulation cannot certify realtime load.
- Signed-base `pow`, hyperbolic/trigonometric functions, ring arithmetic, integer gcd/lcm and live scale tables are now implemented and tested. Bitwise/shift selectors, random arithmetic selectors and demand-rate operators remain excluded.
- PlayBuf/BufRd and granular processing require sample storage, a buffer registry and memory/streaming budgets.
- FFT/PV processing requires spectral buffers and transforms.
- LocalIn/LocalOut require explicit graph feedback semantics.

The earlier finding still applies: two Addy voices through FreeVerb exceeded the device budget. More supported types do not imply every graph is affordable. See [verification](VERIFICATION.md) for measured results and the distinction between emulation, builds and physical hardware checks.
