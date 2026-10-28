# FM1 SynthDef

**SuperCollider patches on the M-VAVE FM-1.**

FM1 SynthDef is an experimental embedded SynthDef runtime and programmable instrument. Write a patch in the local browser editor, compile it with SuperCollider on your Mac, send it over USB, and play it on the FM-1.

The Mac compiles SuperCollider source into a UGen graph. The FM-1 loads and executes that graph using a bounded set of 53 supported UGen types. This is not the full SuperCollider language, scsynth server or plugin ecosystem.

## What it does

- Two simultaneous voices with velocity, pitch, gate and live controls.
- Eight persistent synth slots, plus **slot 0**, a permanent stereo master effect.
- Four shared knobs: patch argument names determine which controls a synth or effect uses.
- Source code on the device screen, syntax colours, stereo meters and scrolling.
- A local browser editor with paste, compile, preview, import/export and verified USB transfers.
- Oscillators, filters, noise, envelopes, resonators, delay/comb/allpass filters and initial random values.
- USB audio and MIDI. Bluetooth is disabled.

## Supported UGens

The current firmware supports these **53 compiled UGen types**. Support is bounded by the calculation rates, input rules and operator selectors in the [engine compatibility profile](synthdef-engine/README.md#supported-profile); listing a type does not imply every SuperCollider usage is supported.

| Category | UGen types |
| --- | --- |
| Oscillators | `SinOsc`, `Saw`, `Pulse`, `LFSaw`, `LFTri`, `LFPulse` |
| Noise | `WhiteNoise`, `PinkNoise`, `LFNoise0`, `LFNoise1`, `LFNoise2` |
| Random values and seed | `Rand`, `ExpRand`, `RandSeed` |
| Envelopes and ramps | `EnvGen`, `Line`, `XLine` |
| Filters and DC removal | `LPF`, `HPF`, `RLPF`, `BPF`, `LeakDC` |
| Resonators and oscillator banks | `Resonz`, `Ringz`, `Klang`, `Klank` |
| Delay and feedback filters | `DelayN`, `DelayL`, `DelayC`, `CombN`, `CombL`, `CombC`, `AllpassN`, `AllpassL`, `AllpassC` |
| Controls and smoothing | `Control`, `LagControl`, `TrigControl`, `Lag`, `LagUD` |
| Routing, selection and panning | `In`, `Out`, `Select`, `Pan2` |
| Constants, rate conversion and bounds | `DC`, `K2A`, `A2K`, `Clip` |
| Arithmetic | `UnaryOpUGen`, `BinaryOpUGen`, `MulAdd`, `Sum3`, `Sum4` |

- `In.ar(0, 1/2)` is available only in the permanent effect slot. `Out.ar` writes stereo bus 0.
- `UnaryOpUGen` and `BinaryOpUGen` accept only the [documented selectors](synthdef-engine/README.md#supported-profile). For example, signal `.midiratio` and general `pow` are not currently supported.
- `Env.perc`, `Env.adsr`, `Env.asr` and other Env constructors are source-language helpers compiled into `EnvGen` data. All nine EnvGen curve types are implemented, with done actions 0/2 and the documented segment limits.
- `Mix`, `LinLin` and `PMOsc` can compile into existing supported nodes; they are not additional runtime types. Simple examples were compiled and host-rendered during the compatibility review. `DynKlang`/`DynKlank` likewise expand into supported oscillator/resonator graphs within the documented limits. This does not guarantee every argument/rate combination or CPU load.

See [possible additions and implementation analysis](docs/UGEN-ROADMAP.md) for candidates. Those candidates are **not implemented** in this release.

## Start here

Download the complete **0.1.0-preview** archive from the repository's Releases page. It contains source, pinned platform sources, the firmware, example patches, licences and checksums. A Git clone also contains everything except the generated release firmware and SDK/compiler downloads.

1. Follow [Mac setup and installation](docs/QUICKSTART.md).
2. Try the [included patches and controls](docs/PATCHES.md).
3. Read the [compatibility and resource limits](docs/COMPATIBILITY.md) before importing other patches.
4. For development, use [building and verification](docs/DEVELOPMENT.md).

## Example bank

| Slot | Patch | Knobs |
| --- | --- | --- |
| 0 | Master echo | 3: amount; 4: delay time |
| 1 | Keys | 1: cutoff; 2: resonance |
| 2 | Addy, five harmonics | No knob arguments |
| 3 | Triple saw | 1: attack/release; 2: detune; 3: vibrato; 4: cutoff |
| 4 | Two-op FM | 1: frequency ratio; 2: FM depth |
| 5–8 | Available | Defined by your patch |

Knobs 3–4 also affect echo while playing Triple saw. PRESET can display slot 0's code while leaving the last synth playable. SELECT scrolls code, OCT± transposes subsequent keyboard notes, and HOME stops notes and clears the effect tail.

## Status

Experimental preview, tested on one FM-1 and an Apple silicon Mac. The engine has 326 SuperCollider reference comparisons, 26 API test groups and 220,000 sanitizer mutation/truncation cases. Recorded hardware checks cover two voices, stealing, release, live knobs, persistent effects and code browsing. See [verification and limitations](docs/VERIFICATION.md).

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
