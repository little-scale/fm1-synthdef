# Pinned reference material

Reference implementation: [SuperCollider 3.14.1](https://github.com/supercollider/supercollider/tree/Version-3.14.1), installed binary commit `426edf6`. These files were downloaded from that tag:

- `server/plugins/LFUGens.cpp`: envelope and Line/XLine definitions.
- `server/plugins/FilterUGens.cpp`: filter, decay and resonator recurrences.
- `server/plugins/TriggerUGens.cpp`: Latch/Gate trigger and constructor behavior.
- `server/plugins/DelayUGens.cpp`, `include/plugin_interface/SC_InlineUnaryOp.h`: Delay, Comb and Allpass families and signed decay/feedback semantics.
- `server/plugins/OscUGens.cpp`: oscillator phase and Klang/Klank recurrences.
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
