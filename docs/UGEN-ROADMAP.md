# UGen additions: implementation analysis

Review date: 28 October 2026. **Analysis only: none of the candidates below has been added to the firmware.** The current implementation remains the 53 types listed in README and profile.json.

This assessment compares the actual loader, renderer, per-voice state, float backend and existing SuperCollider 3.14.1 reference sources. Effort labels describe relative implementation and validation work, not a measured delivery estimate. A small DSP formula does not by itself establish SuperCollider compatibility.

## Best candidates

| Candidate | Relative effort | Musical or practical benefit | Existing support we can reuse / main remaining work |
| --- | --- | --- | --- |
| `SampleRate`, `SampleDur`, `ControlRate`, `ControlDur`, `BlockSize`, `RadiansPerSample` | Very low | Portable timing and frequency expressions without hardcoded 44,100/64 | Voice sample rate and block size already exist; these are initial-rate information outputs. ControlRate must mean sample rate / **engine block 64**, not hardware callback 256. Loader needs explicit zero-input, initial-rate admission. |
| `Delay1`, `Delay2` | Low | Small phase shifts and building blocks for filters | Reuse one or two state values; no delay arena needed. Match explicit initial-value inputs, audio/control defaults and block boundaries. They do not create general feedback routing. |
| `OnePole`, `OneZero`, `LPZ1`, `HPZ1` | Low, with rate caveats | Cheap smoothing, simple low/high-pass shaping and differences | Existing filter state and arithmetic suffice. Constructor behaviour, sign-changing coefficients and coefficient-rate handling need reference tests; do not simply assume the public formula describes every upstream kernel path. |
| `Latch`, `Gate` | Low–moderate | Sample-and-hold and track-and-hold modulation | Reuse a previous trigger and held value. Match startup output, edge detection and mixed-rate inputs; the loader currently rejects inputs faster than the output except A2K, so any required exceptions must be narrow and explicit. |
| `TRand`, `TExpRand` | Low–moderate | New random values on triggers, instead of only once per note | Existing taus88 stream and Rand/ExpRand math can be reused. TRand generates an initial value at construction; test subsequent edges, held values, changing bounds and seeded draw order. TExpRand needs exponential-domain checks and audio-trigger CPU measurement. |
| `Dust`, `Dust2` | Low–moderate | Random events, clicks and irregular modulation | Reuse the uniform random stream. Test density scaling, amplitude distribution, zero/high density, deterministic seeded output and both rates. |
| `Impulse` | Moderate | Regular clocks for envelopes, triggered random changes and rhythmic patches | Reuse bounded phase machinery, but match the first-sample impulse, zero/negative frequency, phase offsets and rate combinations. Timing near boundaries needs more care than a naive phase-wrap loop. |
| `Decay`, `Decay2` | Low–moderate | Percussive trigger envelopes and excitation for resonators | Reuse exponential math and filter state; check time constants, startup, repeated triggers, very short times and zero/near-equal attack/decay cases. |

The DSP/state requirements suggest these can avoid growing `fm_state` and avoid extra delay buffers. That is an engineering expectation, not a measured target result. Adding a four-byte field to the common per-node state would cost roughly 1,920 bytes across 160 nodes × three voice instances (two synths plus the effect), even when most nodes do not use it. Prefer reusing existing fields or a deliberate state-layout design.

## High-value improvements that are not new UGen types

Expand the existing operator selector subset, particularly `.midiratio`, `.ratiomidi`, `.cpsmidi`, `.dbamp`, `.ampdb` and carefully specified rounding/wrapping operations. This would make pitch, gain and knob mappings much more idiomatic. The engine already has exp/log functions, but we still need matching SuperCollider domains, zero/negative behaviour and target accuracy tests.

General `pow` deserves separate treatment: the current device math helper rejects negative bases, while SuperCollider's operator has its own semantics. It must not be enabled merely because a function called `powf` exists. Changes also need coverage in the optimized arithmetic dispatch.

`LinExp` is another useful small mapping UGen for frequency/time knobs. A conservative initial/control-bound implementation would fit the existing approach; source-range degeneracy, destination signs, rate rules and device math accuracy must be checked.

`XFade2` and `LinXFade2` are useful follow-ons for morphing sounds. Existing pan lookup/math can help. However, adding XFade2 alone does **not** make `SelectX` work: the installed SuperCollider helper also emits currently unsupported round/truncate/fold operator selectors.

## Some helpers already work

The current Mac compiler expands `Mix`, `LinLin` and `PMOsc` into existing arithmetic and oscillator nodes. During this review, small examples compiled, imported and host-rendered successfully (7, 6 and 8 nodes respectively). These were smoke checks, not exhaustive compatibility or hardware performance certification.

A direct `LinExp.kr` example was rejected for an unsupported type; signal `.midiratio` was rejected for an unsupported unary selector; `SelectX.ar` was rejected for an unsupported binary selector. This distinction matters: language-side helpers and compiled UGen types are not interchangeable lists.

## Suggested order

1. **Compatibility basics:** the six information UGens, Delay1/Delay2 and pitch/gain conversion selectors. Small additions with little expected per-sample cost.
2. **Musically useful modulation:** Impulse, Latch/Gate, TRand/TExpRand and Dust/Dust2. Impulse is not the very easiest, but supplies the clocks that make the others useful.
3. **More shaping:** OnePole/OneZero, LPZ1/HPZ1, Decay/Decay2, LinExp, then crossfades and the missing selectors needed by SelectX.

If selecting one small first implementation batch, choose the six information UGens plus Delay1/Delay2. If selecting the most audible next feature set, choose Impulse + TRand/TExpRand + Latch.

## Larger work, not easy additions

- `PlayBuf`/`BufRd`, granular buffers and sample storage need a buffer registry, upload/storage management and a new RAM/streaming budget.
- `FFT`/PV processing needs spectral buffers, transforms and a different processing budget.
- `LocalIn`/`LocalOut` need explicit graph feedback/block-delay semantics; they are not aliases for slot 0's immutable input.
- `FreeVerb`, pitch shifting and look-ahead dynamics such as Limiter need separate memory/CPU budgeting. Familiar names do not make them cheap on this target.
- More band-limited oscillators may be straightforward to express but expensive to run. The initial triple-Saw patch already demonstrated that two-voice timing can fail despite admission.

## Acceptance required before claiming support

For each addition: validate binary layout/rates, implement startup and render behaviour, add SuperCollider 3.14.1 reference cases for edge conditions and rate combinations, exercise malformed input and numerical faults, then compare host/device math and rebuild the firmware. Test two-voice playback with slot 0 active and knob changes on the actual FM-1. Update the type list and profile only after those checks pass.

The current link has 18,072 unused pool bytes and 19,920 unused static-RAM bytes, in separate regions. Those are not interchangeable memory or a CPU guarantee. Small control-rate additions are generally a better first fit than large audio effects. No reflashing or DSP changes were performed for this analysis.

## Sources

- Local implementation: `synthdef-engine/src/fm_load.c`, `fm_render.c`, `fm_synth.h`, `fm_noise.inc`, `fm_fast.inc`; device math in `fm1-synth/firmware/device_include/math.h`.
- Pinned SuperCollider 3.14.1 `FilterUGens.cpp`, `NoiseUGens.cpp` and `LFUGens.cpp` in `synthdef-engine/reference/`, plus the installed 3.14.1 class library. These determine exact version-specific constructor/rate details.
- Official help: [SampleRate](https://doc.sccode.org/Classes/SampleRate.html), [OnePole](https://doc.sccode.org/Classes/OnePole.html), [Latch](https://doc.sccode.org/Classes/Latch.html), [TRand](https://doc.sccode.org/Classes/TRand.html), [Impulse](https://doc.sccode.org/Classes/Impulse.html). The hosted help currently labels these pages 3.14.0; implementation work should follow the pinned 3.14.1 reference.
