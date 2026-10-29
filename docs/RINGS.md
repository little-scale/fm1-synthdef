# Fixed-model Rings UGens

The development engine provides `MiRings1.ar` through `MiRings6.ar`. These are FM1 wrappers around separately allocated C adaptations of the six pinned Mutable Instruments Rings models. They are not aliases for the full upstream `MiRings` plugin: each instance has one internal Rings voice, an explicit trigger and two audio outputs. Keyboard polyphony comes from the instrument's two SynthDef voices.

## Memory and model selection

Memory below is the complete per-instance DSP-arena allocation, including model state. Normal graph/node/scratch storage is already reserved by the engine.

| UGen | Model (upstream model index) | Full bytes | With `compact: 1` | Playable synth |
| --- | --- | ---: | ---: | --- |
| `MiRings1` | Modal resonator (0) | 1,396 | 1,396 | Yes |
| `MiRings2` | Sympathetic strings (1) | 67,568 | **32,752** | Compact mode |
| `MiRings3` | Modulated/inharmonic string (2) | 13,536 | 13,536 | Yes |
| `MiRings4` | FM voice (3) | 228 | 228 | Yes |
| `MiRings5` | Quantized sympathetic chords (4) | 67,568 | **32,752** | Compact mode |
| `MiRings6` | String plus internal reverb (5) | **56,836** | 56,836 | Yes; allow room for the bus effect |

A synth voice has a 65,536-byte limit. The two synth voices and persistent effect share 131,072 bytes. Full models 2/5 fit as bus effects; they are rejected as playable synths. Their compact versions retain **all eight strings** and use **960-sample floating-point buffers** instead of 2,048-sample buffers. No sample-precision reduction is applied. The existing low-frequency resampling path handles shorter delay capacity, but low notes and extreme settings can sound different; compact mode is not claimed to be sample-identical to the full model.

Two compact model-2 or model-5 voices plus the supplied 65,536-byte `master-echo` consume **131,040 bytes**. That leaves only 32 bytes of arena headroom; adding further delay-based UGens to that combination will be rejected. Reducing memory does not guarantee CPU headroom: the compact models still process eight strings per voice.

Model 6 uses the original delay lengths and 16-bit reverb storage. The reverb ring was reduced from 32,768 to 21,632 sample slots by removing unused padding, preserving its 32-sample LFO update phase. This saves **22,272 bytes** without shortening any reverb delays or reducing precision. Two model-6 voices consume 113,672 bytes and leave 17,400 bytes for bus effects; the existing master echo will not fit alongside them. Use a bypass/lightweight bus, or put model 6 on the bus instead.

## Interface

```supercollider
MiRings1.ar(in: 0, trig: 1, pit: 60,
    struct: 0.25, bright: 0.5, damp: 0.7, pos: 0.25,
    intern_exciter: 0, bypass: 0, compact: 0, mul: 1, add: 0)
```

All six classes have this signature.

- `in`: mono audio excitation, or an initial scalar placeholder. Scalar input is treated as silence and enables the internal exciter. Sum stereo input explicitly when using this as an effect.
- `trig`: initial/control-rate positive-edge trigger. Default `1` excites a new instance once. Further excitation requires a transition from zero/negative to positive. There is no automatic note-change/onset strummer, debounce interval or audio-rate trigger input.
- `pit`: initial/control-rate pitch, clamped to 0–114. It retains upstream's **+12-semitone tonic offset**: pass `midinote - 12` for keyboard pitch. This makes the effective pitch range MIDI 12–126.
- `struct`, `bright`, `damp`, `pos`: initial/control-rate parameters, clamped to 0–1. Model 5 selects the pinned upstream Bryan chord table through `struct`.
- `intern_exciter`: positive enables the internal exciter even with audio input. Scalar input enables it regardless of this parameter.
- `bypass`: positive copies the mono audio input to both outputs and suspends DSP state evolution. Trigger edge tracking continues while bypassed.
- `compact`: **literal** `0` or `1`, fixed at graph compilation. Default `0` preserves full string buffers; `1` selects smaller buffers for models 2/5. It does not alter the other models. A live knob/control cannot resize an arena.
- `mul`, `add`: ordinary language-side output scaling, not extra runtime inputs.

Only `.ar` is supported; all parameters other than excitation must be scalar/control rate. Supported rendering rates are 32–192 kHz; the FM-1 uses 44.1 kHz. Lower rates fault to silence. Processing uses two 32-sample kernel calls per 64-sample engine block. The instance has its own seeded random generator, so activity in another voice cannot alter its random sequence.

These UGens do not free a note by themselves. Use an envelope with `doneAction: 2` for playable patches. Alternate StringSynth/Easter-egg modes, upstream internal polyphony and the complete `MiRings` API are not implemented.

## Examples and checks

Six playable examples are supplied: [modal](../fm1-synth/examples/rings-modal.scd), [sympathetic](../fm1-synth/examples/rings-sympathetic.scd), [string](../fm1-synth/examples/rings-string.scd), [FM](../fm1-synth/examples/rings-fm.scd), [chords](../fm1-synth/examples/rings-chords.scd), and [string/reverb](../fm1-synth/examples/rings-reverb.scd). They use knobs 1–4 for structure, brightness, damping and position. Models 2/5 explicitly request compact mode. [Rings bus](../fm1-synth/examples/rings-bus.scd) uses audio excitation and knobs 5–8. Each has a compiled graph and importable `.fmslot` alongside it; no saved slots are replaced automatically.

Pinned original sources and their MIT notices are in `synthdef-engine/reference/mi-rings/`. The direct oracle compares full models against unmodified upstream `rings::Part`; separate test-only SC wrappers exercise the explicit-trigger API in complete SynthDefs. Compact comparisons report differences and require exact-buffer safety and finite output; only the declared ordinary-pitch cases use waveform-match limits. They do not claim full-model equivalence for changed low/extreme behaviour.

Reproduce with:

```sh
make -C synthdef-engine test
make -C synthdef-engine reference-test
make -C synthdef-engine sanitize
python3 fm1-synth/tools/test-device-math.py --host
python3 fm1-synth/tools/test-device-math.py
python3 fm1-synth/tools/test-rings.py
bash fm1-synth/tools/build-device.sh
```

Restart an already-running local editor to load the new library and bundled SC classes. Firmware installation, physical listening tests and realtime CPU checks remain deferred until the instrument is available. See [verification](VERIFICATION.md) and [machine-readable evidence](../evidence/rings-split-2026-10-30.json).
