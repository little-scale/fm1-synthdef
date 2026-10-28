# UGen additions: implementation status

Updated 28 October 2026. The development source now implements **77 UGen types**, up from 53 in the published **v0.1.0-preview** firmware. The additions below passed emulation and firmware build checks; physical FM-1 playback and timing remain to be checked. Release assets have not changed.

## Implemented from the initial analysis

| Addition | Rates | Main compatibility work |
| --- | --- | --- |
| `SampleRate`, `SampleDur`, `ControlRate`, `ControlDur`, `BlockSize`, `RadiansPerSample` | Initial | Zero-input admission; constructor caching; control rate uses engine block 64 |
| `Delay1`, `Delay2` | Audio/control | Explicit initial values, different rate defaults, block boundaries and restart |
| `OnePole`, `OneZero`, `LPZ1`, `HPZ1` | Audio/control | Constructor state, signed coefficients and control interpolation |
| `Latch`, `Gate` | Audio/control | Startup, held triggers, edge capture versus tracking, mixed rates |
| `TRand`, `TExpRand` | Audio/control | Seeded draw order, changing bounds, exponential domains, pinned upstream audio-trigger behavior |
| `Dust`, `Dust2` | Audio/control | Seeded streams, density scaling, zero/negative/high density, live density changes |
| `Impulse` | Audio/control | Startup, zero/negative frequency, phase offsets, block changes and audio frequency modulation |
| `Decay`, `Decay2` | Audio/control | Accurate long tails, changing times, repeated triggers, zero/equal/near-equal/very short times |
| `FreeVerb`, `FreeVerb2`, `MiVerb` | Audio | Exact-size caller-owned state, constructor behavior, stereo routing, freeze, parameter clamps, pinned upstream oracles |
| `.cpsmidi`, `.midiratio`, `.ratiomidi`, `.dbamp`, `.ampdb` | Initial/control/audio operators | Domain checks and host/device-math comparison |

The implementation reuses existing per-node state. Static RAM remains **78,384 bytes** and pool use **325,992 of 344,064 bytes**. The development application is **226,972 bytes**, 19,068 bytes larger than the preview. Those separate memory regions do not establish a CPU budget.

See the [engine profile](../synthdef-engine/README.md#supported-profile) for rate/domain restrictions. In particular: Impulse phase is initial-rate only; filter coefficients, Dust density and decay times are initial/control rate; audio triggered-random outputs require an audio trigger. TExpRand audio behavior follows a version-specific SuperCollider 3.14.1 quirk documented there.

Reverb memory is charged to the existing arena: FreeVerb 50,492 bytes, FreeVerb2 102,088 bytes, MiVerb 65,536 bytes per instance. Their implementation required dedicated memory admission and reference tests; physical CPU verification remains pending.

The [verification record](VERIFICATION.md) covers reference comparisons, API checks, sanitizer mutations, two-voice instrument emulation and firmware compilation. The [Dust chimes example](../fm1-synth/examples/dust-chimes.scd) demonstrates the requested random-trigger/decay combination.

## Next moderate candidates

- **LinExp:** useful exponential knob mappings. A bounded initial/control-bound implementation can reuse existing math, but needs source-range degeneracy, destination-sign and target-accuracy coverage.
- **XFade2 / LinXFade2:** morphing and wet/dry control. Existing pan math helps; constructor behavior, rates and level interpolation still need reference tests.
- **Rounding, wrapping and folding operators:** useful independently and needed by the installed `SelectX` helper. Adding crossfade UGens alone does not make SelectX work.
- **General pow:** needs separate signed-base semantics. The current device helper rejects negative bases; its existence does not establish SuperCollider operator compatibility.

These candidates are **not implemented**. The present batch covers both the easy additions and the moderate trigger/decay group; it does not claim all prospective UGens are cheap or complete.

## Helpers already usable

The Mac compiler expands `Mix`, `LinLin` and `PMOsc` into existing arithmetic and oscillator nodes. Simple examples passed compilation/import/rendering during the initial review. `DynKlang` and `DynKlank` similarly expand into supported nodes within graph limits. These helpers are not additional runtime UGen types, and smoke checks do not certify arbitrary arguments or CPU load.

## Larger work

- `PlayBuf`/`BufRd`, granular buffers and sample storage need a buffer registry, upload/storage management and a RAM/streaming budget.
- `FFT`/PV processing needs spectral buffers and transforms.
- `LocalIn`/`LocalOut` need explicit graph feedback/block-delay semantics; they are not aliases for slot 0's immutable input.
- Pitch shifting and look-ahead dynamics need separate memory/CPU budgeting.
- More band-limited oscillators may be expensive. The original three-Saw patch demonstrated that admission alone cannot guarantee two-voice timing.

## Validation policy and sources

An implementation must validate graph layout/rates, match constructor/render behavior against pinned SuperCollider 3.14.1, cover malformed inputs and numerical faults, compare host/device math and rebuild firmware. Emulation support and physical-device acceptance are recorded separately. Hardware acceptance additionally needs two voices, slot 0 and live controls on the FM-1.

Primary references are the pinned sources and hash manifest in `synthdef-engine/reference/`, especially `FilterUGens.cpp`, `NoiseUGens.cpp`, `TriggerUGens.cpp` and `LFUGens.cpp`, plus the installed 3.14.1 class library. Official help provides usage context: [Dust](https://doc.sccode.org/Classes/Dust.html), [Decay](https://doc.sccode.org/Classes/Decay.html), [Decay2](https://doc.sccode.org/Classes/Decay2.html), [Impulse](https://doc.sccode.org/Classes/Impulse.html). Exact version-specific behavior follows the pinned implementation.
