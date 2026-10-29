# Pinned reference material

Reference implementation: [SuperCollider 3.14.1](https://github.com/supercollider/supercollider/tree/Version-3.14.1), installed binary commit `426edf6`. These files were downloaded from that tag:

- `server/plugins/GendynUGens.cpp`: Gendy1/2/3 stochastic oscillators, Nick Collins (GPL-2.0-or-later).
- `server/plugins/LFUGens.cpp`: envelope and Line/XLine definitions.
- `server/plugins/FilterUGens.cpp`: filter, decay and resonator recurrences, including Formlet.
- `server/plugins/TriggerUGens.cpp`: Latch/Gate trigger and constructor behavior.
- `server/plugins/DelayUGens.cpp`, `include/plugin_interface/SC_InlineUnaryOp.h`: Delay, Comb and Allpass families and signed decay/feedback semantics.
- `server/plugins/OscUGens.cpp`: oscillator phase (including Formant) and Klang/Klank recurrences.
- `server/plugins/PanUGens.cpp`: Pan2 table quantization and interpolation.
- `server/plugins/IOUGens.cpp`: LagControl/TrigControl initialization and processing.
- `server/plugins/NoiseUGens.cpp`, `include/plugin_interface/SC_RGen.h`, `include/plugin_interface/Hash.h`: noise kernels, taus88 generator and integer seed hash.
- `server/plugins/BinaryOpUGens.cpp`, `server/plugins/MulAddUGens.cpp`: mixed-rate arithmetic interpolation.
- `common/Samp.cpp`, `server/scsynth/SC_World.cpp`: shared sine/cosecant table definitions and plugin table wiring.

The source headers credit James McCartney and license the code under GPL-2.0-or-later. The engine's adaptations are distributed under GPL-3.0-or-later; `../LICENSE` is the GPLv3 text downloaded from the same tag's `COPYING`. These reference C++ files are not linked into the new C runtime.

Format reference: [Synth Definition File Format](https://doc.sccode.org/Reference/Synth-Definition-File-Format.html). The runtime accepts a bounded subset of format version 2; see the engine README for deliberate exclusions.

Fixtures were exported by the installed `sclang` 3.14.1 and rendered by `scsynth` 3.14.1. The export programs are `../tools/export-fixtures.scd`, `export-stage2.scd`, and `export-stage3.scd`, `export-delays.scd`, `export-random.scd`, `export-extensions.scd`, orchestrated by `../tools/export-all.py`; `fixtures.json` records file hashes. `sources.json` records reference-source hashes. Tests restrict the server's plug-in search to the official installation directory, so user extension plug-ins are excluded from the renderer.

On this installation, offline renders containing Klang/Klank crashed in `Graph_Dtor` during final world cleanup. The scores explicitly free test group 1 at 0.51 seconds before the final marker at 0.52 seconds. Comparisons use the first 345 blocks (about 0.501 seconds); this workaround therefore does not replace or truncate the compared sound. Every reference invocation must exit successfully and without server errors.

The seeded `m5_addy` integration score ends at 1.21 seconds; its comparison uses 827 blocks (about 1.20 seconds) to cover the complete voice lifetime and subsequent silence.

The m6 extension fixtures cover 21 additional types and five pitch/gain selectors, including seeded Dust and triggered random streams, held triggers, coefficient sign changes, negative-frequency Impulse, changing control values, and zero/equal/near-equal/very short Decay2 times. See the engine README for the one-sample FM impulse timing oracle and pinned TExpRand audio-trigger behavior.

## Reverb references

`ReverbUGens.cpp` is the SuperCollider 3.14.1 FreeVerb/FreeVerb2 implementation (blackrain, GPL-2.0-or-later). `mi-ugens/` contains the minimal unmodified MiVerb wrapper, class and DSP dependencies from commit `10f6824f68ad63475f67dfe3fe5016e2d2634f84`; its own manifest records every source hash. The wrapper is GPL-3.0-or-later and Émilie Gillet's DSP headers retain their full MIT notices. `sc-headers/` contains the minimal pinned 3.14.1 headers needed to build the reference plugin; each retains its notice and is covered by its local hash manifest.

`tools/build-miverb-reference.py` verifies those manifests and builds an isolated plugin with C++17, `TEST` (portable saturation), no fast-math and no floating-point contraction. Reverb reference invocations admit only stock plugins plus this build. The bundled class is compiled alongside stock SC classes with user extensions excluded. No user/system plugin or language configuration is modified.

`export-reverbs.scd` adds 37 approximately four-second comparisons covering mono/stereo impulses, wet/dry, parameter clamps, noise bursts, live changes, freeze, diffusion and overdrive. `tests/test_miverb_oracle.py` additionally feeds identical float input to the C port and unmodified upstream C++ DSP, isolating reverb accuracy from upstream oscillator differences. See the engine README for limits and the overdrive integration tolerance.

## October 30 utility and modulation references

`export-utilities.scd` and `export-modulation.scd` add 252 fixtures for 32 UGen types, seven binary selectors and SelectX. `DynNoiseUGens.cpp` is pinned to SuperCollider 3.14.1 and included in the main source manifest. The MiMu source/class/help are pinned alongside MiVerb; the isolated plugin builder builds both plugins. `tests/test_mimu_oracle.py` checks 1,048,576 identical-input samples bit for bit, covering control/audio gain and changing bypass.

`tools/probe-mirings.py --upstream PATH --output RESULT.json` builds a host-only feasibility probe from a separately checked-out mi-UGens commit `10f6824f68ad63475f67dfe3fe5016e2d2634f84`. `mirings-probe-sources.json` verifies its upstream Rings/stmlib sources. The probe does not provide MiRings support in the engine and cannot establish hardware timing.

`export-gendy.scd` supplies 81 Gendy graphs. `tests/gendy_oracle.cpp` compiles the unmodified pinned source and compares distribution values, constructor arrays, RNG consumption and identical-state one-sample transitions against both C math backends. This isolates chaotic drift from implementation errors. Long NRT comparisons are explicitly labeled waveform or stochastic-summary; neither is a hardware timing measurement.

`export-formants.scd` supplies 95 Formant/Formlet waveform fixtures using the already-pinned OscUGens.cpp and FilterUGens.cpp sources. The C port retains Formant’s wrapped integer phase resets and uses a compensated, frequency-shifted difference-state realization for Formlet.

`export-operators-scales.scd` supplies 267 waveform fixtures for 28 additional operators, compile-time Scale/Tuning objects and live DegreeToKey tables. UnaryOpUGens.cpp is pinned to 3.14.1 alongside the existing binary/operator/oscillator sources. `test_operators_oracle.py` checks 1,835,103 direct values on each math backend; `test_scale_catalog.py` compiles all 108 Scale and 35 Tuning presets and checks 6,514 live degree lookups against their exported pitch data. Three upstream edge bugs are tested mathematically instead of copying them: negative exact-octave DegreeToKey indexing, held-divisor idiv_ak and firstArg_ka's addition fallback. See the profile for exact semantics.

## Fixed-model Rings

`mi-rings/` retains the required original mi-UGens/eurorack sources at commit `10f6824f68ad63475f67dfe3fe5016e2d2634f84`. `sources.json` pins every original file. The MIT Rings/stmlib DSP is by Emilie Gillet (2014–2015); original source notices and LICENSE are retained. The original mi-UGens wrapper/class files retain Volker Bohm's notices. The runtime is a separate C adaptation with model-specific arena storage, explicit triggers and optional compact string buffers; it does not compile the original C++ DSP into firmware. `tools/generate-rings-tables.py` reproduces the exact lookup/chord data from verified pinned sources. `tests/rings_oracle.cpp` uses unmodified upstream Part; `tests/rings_reference_plugin.cpp` is a test-only SC adapter to that Part, restricted to full buffers. See [the Rings guide](../../docs/RINGS.md).
