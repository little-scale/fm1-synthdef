# FM1 SynthDef

**SuperCollider patches on the M-VAVE FM-1.**

FM1 SynthDef is an experimental embedded SynthDef runtime and programmable instrument. Write a patch in the local browser editor, compile it with SuperCollider on your Mac, send it over USB, and play it on the FM-1.

The Mac compiles SuperCollider source into a UGen graph. The FM-1 loads and executes that graph using a bounded set of 154 supported UGen types in the development source. This is not the full SuperCollider language, scsynth server or plugin ecosystem.

## What it does

- Two simultaneous voices with velocity, pitch, gate and live controls.
- Eight persistent synth slots and **four selectable master-effect presets**, with one effect running at a time.
- Four physical knobs with a latching **FX** toggle: `knob1`–`knob4` for synths, `knob5`–`knob8` for effects.
- Source code on the device screen, syntax colours, stereo meters and scrolling.
- A local browser editor with paste, compile, preview, import/export and verified USB transfers.
- Oscillators, filters, noise, envelopes, resonators, delay/comb/allpass filters and initial random values.
- USB audio and MIDI. Bluetooth is disabled.

## Supported UGens

The latest expansion adds **SinOscFB, Blip, Phasor, Pluck, MoogFF, FreqShift, Limiter, FBSineN/L/C and StandardN/L**. See the [synthesis/effects guide and four new patches](docs/SYNTHESIS-EXPANSION.md).

The development source adds **18 random/chaotic generators** with no additional arena allocation. See the [generator guide and playable examples](docs/STOCHASTIC.md).

The development source includes **Formant, Formlet, Gendy1/2/3**, **MiMu.ar**, live Scale/Tuning pitch tables and 28 additional maths operators. [MiMu drive](fm1-synth/examples/mimu-drive.scd) and [modulation lab](fm1-synth/examples/modulation-lab.scd) are ready to compile and preview. **MiRings1–MiRings6** now provide separate Rings models, with optional compact buffers for models 2/5 and a smaller, unchanged-delay reverb allocation for model 6. See the [Rings model/memory guide](docs/RINGS.md). The complete upstream MiRings API remains unsupported.

The development source supports these **154 compiled UGen types**, verified in emulation. The published **v0.1.0-preview firmware contains 53 types**; the additions below require a development build; the previous 77-type build has passed selected hardware tests. The latest 77 additions are awaiting hardware checks. Support is bounded by the calculation rates, input rules and operator selectors in the [engine compatibility profile](synthdef-engine/README.md#supported-profile); listing a type does not imply every SuperCollider usage is supported.

| Category | UGen types |
| --- | --- |
| Oscillators | `SinOsc`, `Saw`, `Pulse`, `LFSaw`, `LFTri`, `LFPulse`, `Impulse`, `VarSaw`, `SyncSaw`, `LFCub`, `LFPar`, `Formant`, `SinOscFB`, `Blip` |
| Chaotic oscillators | `Crackle`, `Logistic`, `HenonN`, `HenonL`, `HenonC`, `CuspN`, `CuspL`, `LatoocarfianN`, `LatoocarfianL`, `LatoocarfianC`, `LorenzL`, `FBSineN`, `FBSineL`, `FBSineC`, `StandardN`, `StandardL` |
| Stochastic oscillators | `Gendy1`, `Gendy2`, `Gendy3` (audio/control; 1–64 literal initial breakpoints) |
| Noise | `WhiteNoise`, `PinkNoise`, `LFNoise0`, `LFNoise1`, `LFNoise2`, `Dust`, `Dust2`, `BrownNoise`, `ClipNoise`, `LFDNoise0`, `LFDNoise1`, `LFDNoise3`, `LFClipNoise`, `LFDClipNoise`, `GrayNoise`, `Hasher` |
| Random values and seed | `Rand`, `ExpRand`, `RandSeed`, `TRand`, `TExpRand`, `TIRand`, `TWindex`, `CoinGate` |
| Envelopes and ramps | `EnvGen`, `Line`, `XLine`, `Decay`, `Decay2` |
| Filters and DC removal | `LPF`, `HPF`, `RLPF`, `BPF`, `LeakDC`, `OnePole`, `OneZero`, `LPZ1`, `HPZ1`, `RHPF`, `BRF`, `BPeakEQ`, `BLowShelf`, `BHiShelf`, `MoogFF` |
| Dynamics | `Limiter` |
| Frequency shifting | `FreqShift` |
| Plucked strings | `Pluck` |
| Distortion | `MiMu` |
| Rings models | `MiRings1`, `MiRings2`, `MiRings3`, `MiRings4`, `MiRings5`, `MiRings6` |
| Reverbs | `FreeVerb`, `FreeVerb2`, `MiVerb` |
| Resonators and oscillator banks | `Resonz`, `Ringz`, `Formlet`, `Klang`, `Klank` |
| Delay and feedback filters | `Delay1`, `Delay2`, `DelayN`, `DelayL`, `DelayC`, `CombN`, `CombL`, `CombC`, `AllpassN`, `AllpassL`, `AllpassC` |
| Controls and smoothing | `Control`, `LagControl`, `TrigControl`, `Lag`, `LagUD`, `Latch`, `Gate`, `Slew`, `Lag2`, `Lag3`, `Amplitude` |
| Routing, selection and panning | `In`, `Out`, `Select`, `Pan2`, `XFade2`, `LinXFade2`, `Balance2` |
| Constants, rate conversion and bounds | `DC`, `K2A`, `A2K`, `Clip`, `Fold`, `Wrap`, `LinExp` |
| Timing information (initial rate) | `SampleRate`, `SampleDur`, `ControlRate`, `ControlDur`, `BlockSize`, `RadiansPerSample` |
| Triggers and counters | `Trig`, `Trig1`, `PulseDivider`, `PulseCount`, `ToggleFF`, `SetResetFF`, `Stepper`, `Phasor` |
| Scale/tuning lookup | `DegreeToKey`, with bounded immutable `MaxLocalBufs`, `LocalBuf`, `SetBuf` tables |
| Arithmetic | `UnaryOpUGen`, `BinaryOpUGen`, `MulAdd`, `Sum3`, `Sum4` |

- `In.ar(0, 1/2)` is available only in the permanent effect slot. `Out.ar` writes stereo bus 0.
- `UnaryOpUGen` and `BinaryOpUGen` accept the [operators listed below](#supported-operators), including `.abs`, `.squared`, `.cubed`, signed `.pow` / `**`, `.tanh`, pitch/gain conversions, ring modulation, rounding and bounds.
- `Scale` and `Tuning` presets/custom objects run at compilation. `Scale.as(LocalBuf)` plus `DegreeToKey.ar/kr` supports a changing degree during playback, including microtonal pitch values. Tables are read-only and limited to 128 entries; this does not add general sample buffers or automatic Event-scale behaviour.
- `Env.perc`, `Env.adsr`, `Env.asr` and other Env constructors are source-language helpers compiled into `EnvGen` data. All nine EnvGen curve types are implemented, with done actions 0/2 and the documented segment limits.
- `Mix.ar`, `Mix`, `Splay.ar`, `LinLin` and `PMOsc` can compile into existing supported nodes; they are not additional runtime types. Mix/Splay now have 15 compiled comparisons against explicit sums, stereo panners and rate converters. `DynKlang`/`DynKlank` likewise expand into supported oscillator/resonator graphs within the documented limits. This does not guarantee every argument/rate combination or CPU load.

The development firmware adds a three-bar battery indicator and USB-power symbol in the top-right corner, including the FX code view.

See [implementation status and remaining additions](docs/UGEN-ROADMAP.md). Three [slot-0 reverb examples](docs/PATCHES.md#development-reverb-alternatives) are included, with knob 5 for wet/dry and knob 6 for room size or decay. Try the new [Dust chimes source](fm1-synth/examples/dust-chimes.scd) in the development host preview: knob 1 controls event density and knob 2 decay time. It is not installed into the example bank automatically.

## Supported operators

The development engine supports **33 unary operators** (one input) and **40 binary operators** (two inputs), listed below. These run through `UnaryOpUGen` and `BinaryOpUGen` at initial, control or audio rate, according to the compiled graph. The published 0.1.0-preview firmware has an earlier subset; use the development build for this complete list.

### Unary operators

| Category | Operators |
| --- | --- |
| Sign and magnitude | `.neg`, `.abs`, `.sign` |
| Rounding and fraction | `.ceil`, `.floor`, `.frac` |
| Powers and exponential | `.squared`, `.cubed`, `.sqrt`, `.exp`, `.reciprocal` |
| Logarithms | `.log`, `.log2`, `.log10` |
| Pitch and frequency | `.midicps`, `.cpsmidi`, `.midiratio`, `.ratiomidi`, `.octcps`, `.cpsoct` |
| Gain | `.dbamp`, `.ampdb` |
| Trigonometry | `.sin`, `.cos`, `.tan`, `.asin`, `.acos`, `.atan` |
| Hyperbolic functions | `.sinh`, `.cosh`, `.tanh` |
| Waveshaping | `.distort`, `.softclip` |

For example: `sig.abs`, `sig.squared` or `sig.tanh`.

### Binary operators

| Category | Operators |
| --- | --- |
| Arithmetic | `+`, `-`, `*`, `/`, `.div`, `.mod`, `.pow` |
| Comparison | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Minimum and maximum | `.min`, `.max` |
| Integer relationships | `.lcm`, `.gcd` |
| Quantization | `.round`, `.roundUp`, `.trunc` |
| Angles and distances | `.atan2`, `.hypot`, `.hypotApx` |
| Ring modulation | `.ring1`, `.ring2`, `.ring3`, `.ring4` |
| Sums and differences | `.difsqr`, `.sumsqr`, `.sqrsum`, `.sqrdif`, `.absdif` |
| Thresholds and shaping | `.thresh`, `.amclip`, `.scaleneg` |
| Bounds | `.clip2`, `.excess`, `.fold2`, `.wrap2` |
| First operand | `.firstArg` |

For example: `sig * amp`, `sig.pow(0.5)` or `sig.clip2(0.8)`. `%` is the modulo spelling and `**` is the power spelling.

**Signal comparisons:** `==` and `!=` in the table identify the compiled equality/inequality selectors. Ordinary `==` / `!=` on UGen objects are language-level comparisons during compilation. Use `a |==| b` for signal equality, or `BinaryOpUGen('!=', a, b)` for the signal inequality selector.

**Limits and semantics:** `.div` uses floor division; `.trunc` uses floor quantization. `.sqrt` and `.pow` retain SuperCollider's signed-input behaviour. `.gcd` / `.lcm` truncate their inputs to integers and require absolute inputs no greater than 2²⁴. Nonfinite results or signal magnitudes above 1e12 retire the affected voice. Mixed audio/control arithmetic interpolates control inputs across each block.

Bitwise/shift selectors, random arithmetic selectors such as `.rand` / `.rrand`, demand-rate operators and selectors absent from these tables remain unsupported. `Rand`, `ExpRand`, `TRand`, `TExpRand`, `TIRand`, `TWindex`, `CoinGate` and noise UGens are available separately. See the [engine profile](synthdef-engine/README.md#supported-profile) for detailed domains and the documented SuperCollider 3.14.1 edge differences.

## Start here

Download the complete **0.1.0-preview** archive from the repository's Releases page. It contains source, pinned platform sources, the firmware, example patches, licences and checksums. A Git clone also contains everything except the generated release firmware and SDK/compiler downloads.

1. Follow [Mac setup and installation](docs/QUICKSTART.md).
2. Try the [included patches and controls](docs/PATCHES.md).
3. Read the [compatibility and resource limits](docs/COMPATIBILITY.md) before importing other patches.
4. For development, use [building and verification](docs/DEVELOPMENT.md).

Three additional [Gendy examples](fm1-synth/examples/gendy3.scd) use knobs 1–4 for amplitude movement, duration movement, active breakpoints and cutoff. They are provided as `.scd`, `.scsyndef` and `.fmslot` files alongside the other examples; importing them is optional and does not alter the example bank.

[Formant](fm1-synth/examples/formant.scd) and [Formlet](fm1-synth/examples/formlet.scd) examples are also supplied with four live knob controls, compiled graphs and importable `.fmslot` files.

Try the [just-intonation Formant patch](fm1-synth/examples/just-intonation.scd): each keyboard semitone advances one scale degree. Knobs control scale transposition, formant, attack and release. Its compiled graph and importable `.fmslot` are supplied alongside the source.

## Example bank

| Slot | Patch | Knobs |
| --- | --- | --- |
| FX1 | Master echo | 5: amount; 6: delay time |
| FX2–FX4 | FreeVerb / FreeVerb2 / MiVerb | 5: wet/dry; 6: room/decay |
| 1 | Keys | 1: cutoff; 2: resonance |
| 2 | Addy, five harmonics | No knob arguments |
| 3 | Triple saw | 1: attack/release; 2: detune; 3: vibrato; 4: cutoff |
| 4 | Two-op FM | 1: frequency ratio; 2: FM depth |
| 5 | Dusty | 1: burst decay; 2: dust density; five channels spread with Splay |
| 6–8 | Available | Defined by your patch |

**PRESET** selects synths 1–8. **ALGORITHM** selects bypass or FX1–FX4. Press **FX** once to latch effect control: its LED stays on, the screen shows effect code, and the four knobs send `knob5`–`knob8`. Press again to return to synth code and `knob1`–`knob4`. Toggling preserves both sets of values and held notes. Synth changes preserve the effect and its tail; effect changes briefly fade the output, replace the effect, and fade back in while keeping synth voices alive. The old effect tail ends when it is replaced.

SELECT scrolls the focused code, OCT± transposes subsequent keyboard notes, and HOME stops notes and clears the effect tail. On boot, synth 1 and FX1 load with FX control off. Knob and selection changes do not write flash. See [upgrade and storage details](fm1-synth/README.md#four-master-effect-presets).

## Status

The development engine passes **1,444 reference cases on three rendering backends**: 1,378 waveform/timing checks and 66 labeled statistical checks, supported by independent identical-state source oracles. It also passes **80 API test groups and 1,560,000 sanitizer mutation/truncation cases**. All 108 stock Scale and 35 Tuning presets pass live lookup checks; each maths backend passes 1,835,103 direct operator comparisons. Instrument emulation covers two voices, live knobs, stealing, release and persistent effects, including three new playable patches and a stereo frequency-shift/limiter bus. High-feedback and chaotic generators can produce different long waveforms from desktop SuperCollider; their calculations are checked separately from long-render statistics. The earlier 77-type build passed selected hardware checks; the latest 77 additions still need hardware timing and listening checks. See [verification and limitations](docs/VERIFICATION.md).

Passing the graph and memory checks does **not** guarantee that an arbitrary patch fits the audio processing budget. The overload guard stops notes and disables effects when the device is over budget. Final-release power-cycle persistence and recovery are not yet part of the acceptance record. This is an independent project, not an official M-VAVE or SuperCollider product.

## Source layout

- `synthdef-engine/`: graph loader, DSP kernels, profile, fixtures and reference tests.
- `fm1-synth/`: instrument, screen, slot journal, protocol, browser manager and examples.
- `fm1-base/`: unchanged, pinned upstream platform and `bsp/` source snapshots.
- `setup/`: dependency pins and build setup.
- `docs/`: setup, compatibility, demo and release notes.
- `evidence/`: selected machine-readable verification records.

## Credits and licences

New engine and instrument code is GPL-3.0-or-later. SuperCollider-derived DSP retains its attribution. The fm1-base platform and felucca-bsp retain MPL-2.0 and their included notices; fonts and SDK packaging data retain their respective licences. See [THIRD_PARTY.md](THIRD_PARTY.md), [LICENSE](LICENSE) and the original per-file notices.
