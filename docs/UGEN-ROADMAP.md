# UGen roadmap

Updated 30 October 2026. The development engine contains **109 UGen types**. The 32 additions below are implemented; hardware installation and realtime checks are deferred at the user's request. The existing published preview remains unchanged.

## Completed expansion

| Group | Added types |
| --- | --- |
| Bounds and mappings | Fold, Wrap, LinExp |
| Noise | BrownNoise, ClipNoise, LFDNoise0, LFDNoise1, LFDNoise3 |
| Triggers and counters | Trig, Trig1, PulseDivider, PulseCount, ToggleFF, SetResetFF, Stepper |
| Mixing and panning | XFade2, LinXFade2, Balance2 |
| Smoothing and tracking | Slew, Lag2, Lag3, Amplitude |
| Oscillators | VarSaw, SyncSaw, LFCub, LFPar |
| Filters and EQ | RHPF, BRF, BPeakEQ, BLowShelf, BHiShelf |
| Distortion | MiMu |

Modulo, round, roundUp, trunc, clip2, fold2 and wrap2 binary selectors were added too. SelectX now compiles and renders at audio/control rate; it is a source helper, not a separate runtime UGen. The [full profile](../synthdef-engine/README.md#supported-profile) states rate and domain restrictions. In particular, the new EQ coefficients are initial/control rate, oscillator frequencies are bounded, and MiMu is audio-only.

New types reuse existing per-node state and allocate no delay buffers. Static RAM and the instrument pool remain unchanged. A stable difference-state form avoids cancellation in low-cutoff high-pass/notch/EQ filters. Control-frequency SinOsc increments and MiMu's float-to-PCM conversion match the reference without software double routines on the single-precision device.

[MiMu drive](../fm1-synth/examples/mimu-drive.scd) is a stereo master-effect example; [modulation lab](../fm1-synth/examples/modulation-lab.scd) exercises VarSaw, LFDNoise1, LinExp and Lag3 with live knobs. Neither example replaces a saved user slot.

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

Target pointer/alignment differences do not remove the large gap to the current **131,072-byte shared DSP arena**, or the **65,536-byte per-synth-voice admission limit**. The current pool has approximately 18 KiB free. Full MiRings therefore remains unsupported in this firmware; it is rejected at import rather than approximated under the same name.

Possible next approaches are a deliberately named modal-only engine, splitting model-specific storage, or a dedicated lower-polyphony firmware/memory layout. These require design, audio-reference and realtime work. Existing Ringz/Klank can make useful resonant patches but are not claimed to sound identical to Rings.

Reproduce the host probe with `synthdef-engine/tools/probe-mirings.py --upstream PATH --output RESULT.json`, using mi-UGens commit `10f6824f68ad63475f67dfe3fe5016e2d2634f84` including its eurorack sources. The probe verifies source hashes before building. See [probe evidence](../evidence/mirings-feasibility-2026-10-30.json).

## Remaining work

- Install the new image, preserve/read back all slots, and check two voices through the chosen master effect with live knobs. Emulation cannot certify realtime load.
- General signed-base `pow` remains unsupported and needs its own reference/domain work.
- PlayBuf/BufRd and granular processing require sample storage, a buffer registry and memory/streaming budgets.
- FFT/PV processing requires spectral buffers and transforms.
- LocalIn/LocalOut require explicit graph feedback semantics.

The earlier finding still applies: two Addy voices through FreeVerb exceeded the device budget. More supported types do not imply every graph is affordable. See [verification](VERIFICATION.md) for measured results and the distinction between emulation, builds and physical hardware checks.
