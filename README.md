# FM1 SynthDef

**SuperCollider patches on the M-VAVE FM-1.**

FM1 SynthDef is an experimental embedded SynthDef runtime and programmable instrument. Write a patch in the local browser editor, compile it with SuperCollider on your Mac, send it over USB, and play it on the FM-1.

The Mac compiles SuperCollider source into a UGen graph. The FM-1 loads and executes that graph using a bounded set of 109 supported UGen types in the development source. This is not the full SuperCollider language, scsynth server or plugin ecosystem.

## What it does

- Two simultaneous voices with velocity, pitch, gate and live controls.
- Eight persistent synth slots and **four selectable master-effect presets**, with one effect running at a time.
- Four physical knobs with a latching **FX** toggle: `knob1`–`knob4` for synths, `knob5`–`knob8` for effects.
- Source code on the device screen, syntax colours, stereo meters and scrolling.
- A local browser editor with paste, compile, preview, import/export and verified USB transfers.
- Oscillators, filters, noise, envelopes, resonators, delay/comb/allpass filters and initial random values.
- USB audio and MIDI. Bluetooth is disabled.

## Supported UGens

The latest expansion adds 32 types, including **MiMu.ar**. [MiMu drive](fm1-synth/examples/mimu-drive.scd) and [modulation lab](fm1-synth/examples/modulation-lab.scd) are ready to compile and preview. Full **MiRings is not supported**: its upstream host probe works, but its roughly 176 KiB instance exceeds our 128 KiB shared DSP arena. See the [roadmap](docs/UGEN-ROADMAP.md).

The development source supports these **109 compiled UGen types**, verified in emulation. The published **v0.1.0-preview firmware contains 53 types**; the additions below require a development build; the previous 77-type build has passed selected hardware tests. The latest 32 additions are awaiting hardware checks. Support is bounded by the calculation rates, input rules and operator selectors in the [engine compatibility profile](synthdef-engine/README.md#supported-profile); listing a type does not imply every SuperCollider usage is supported.

| Category | UGen types |
| --- | --- |
| Oscillators | `SinOsc`, `Saw`, `Pulse`, `LFSaw`, `LFTri`, `LFPulse`, `Impulse`, `VarSaw`, `SyncSaw`, `LFCub`, `LFPar` |
| Noise | `WhiteNoise`, `PinkNoise`, `LFNoise0`, `LFNoise1`, `LFNoise2`, `Dust`, `Dust2`, `BrownNoise`, `ClipNoise`, `LFDNoise0`, `LFDNoise1`, `LFDNoise3` |
| Random values and seed | `Rand`, `ExpRand`, `RandSeed`, `TRand`, `TExpRand` |
| Envelopes and ramps | `EnvGen`, `Line`, `XLine`, `Decay`, `Decay2` |
| Filters and DC removal | `LPF`, `HPF`, `RLPF`, `BPF`, `LeakDC`, `OnePole`, `OneZero`, `LPZ1`, `HPZ1`, `RHPF`, `BRF`, `BPeakEQ`, `BLowShelf`, `BHiShelf` |
| Distortion | `MiMu` |
| Reverbs | `FreeVerb`, `FreeVerb2`, `MiVerb` |
| Resonators and oscillator banks | `Resonz`, `Ringz`, `Klang`, `Klank` |
| Delay and feedback filters | `Delay1`, `Delay2`, `DelayN`, `DelayL`, `DelayC`, `CombN`, `CombL`, `CombC`, `AllpassN`, `AllpassL`, `AllpassC` |
| Controls and smoothing | `Control`, `LagControl`, `TrigControl`, `Lag`, `LagUD`, `Latch`, `Gate`, `Slew`, `Lag2`, `Lag3`, `Amplitude` |
| Routing, selection and panning | `In`, `Out`, `Select`, `Pan2`, `XFade2`, `LinXFade2`, `Balance2` |
| Constants, rate conversion and bounds | `DC`, `K2A`, `A2K`, `Clip`, `Fold`, `Wrap`, `LinExp` |
| Timing information (initial rate) | `SampleRate`, `SampleDur`, `ControlRate`, `ControlDur`, `BlockSize`, `RadiansPerSample` |
| Triggers and counters | `Trig`, `Trig1`, `PulseDivider`, `PulseCount`, `ToggleFF`, `SetResetFF`, `Stepper` |
| Arithmetic | `UnaryOpUGen`, `BinaryOpUGen`, `MulAdd`, `Sum3`, `Sum4` |

- `In.ar(0, 1/2)` is available only in the permanent effect slot. `Out.ar` writes stereo bus 0.
- `UnaryOpUGen` and `BinaryOpUGen` accept only the [documented selectors](synthdef-engine/README.md#supported-profile). Pitch/gain conversions `.cpsmidi`, `.midiratio`, `.ratiomidi`, `.dbamp` and `.ampdb` are now supported. Rounding, modulo, `clip2`, `fold2` and `wrap2` selectors are supported; general `pow` remains unsupported.
- `Env.perc`, `Env.adsr`, `Env.asr` and other Env constructors are source-language helpers compiled into `EnvGen` data. All nine EnvGen curve types are implemented, with done actions 0/2 and the documented segment limits.
- `Mix.ar`, `Mix`, `Splay.ar`, `LinLin` and `PMOsc` can compile into existing supported nodes; they are not additional runtime types. Mix/Splay now have 15 compiled comparisons against explicit sums, stereo panners and rate converters. `DynKlang`/`DynKlank` likewise expand into supported oscillator/resonator graphs within the documented limits. This does not guarantee every argument/rate combination or CPU load.

The development firmware adds a three-bar battery indicator and USB-power symbol in the top-right corner, including the FX code view.

See [implementation status and remaining additions](docs/UGEN-ROADMAP.md). Three [slot-0 reverb examples](docs/PATCHES.md#development-reverb-alternatives) are included, with knob 5 for wet/dry and knob 6 for room size or decay. Try the new [Dust chimes source](fm1-synth/examples/dust-chimes.scd) in the development host preview: knob 1 controls event density and knob 2 decay time. It is not installed into the example bank automatically.

## Start here

Download the complete **0.1.0-preview** archive from the repository's Releases page. It contains source, pinned platform sources, the firmware, example patches, licences and checksums. A Git clone also contains everything except the generated release firmware and SDK/compiler downloads.

1. Follow [Mac setup and installation](docs/QUICKSTART.md).
2. Try the [included patches and controls](docs/PATCHES.md).
3. Read the [compatibility and resource limits](docs/COMPATIBILITY.md) before importing other patches.
4. For development, use [building and verification](docs/DEVELOPMENT.md).

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

The development engine passes 553 SuperCollider reference comparisons on three rendering backends, 34 API test groups and 440,000 sanitizer mutation/truncation cases. The full instrument simulator also covers the additions with two voices and master echo. The development build also passed 189 hardware checks of the three reverbs with four lighter two-voice synths; two Addy voices through FreeVerb exceed the processing budget. The earlier 53-type preview was tested on one FM-1 and an Apple silicon Mac; its recorded hardware checks cover two voices, stealing, release, live knobs, persistent effects and code browsing. See [verification and limitations](docs/VERIFICATION.md).

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
