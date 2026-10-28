# Pinned reference material

Reference implementation: [SuperCollider 3.14.1](https://github.com/supercollider/supercollider/tree/Version-3.14.1), installed binary commit `426edf6`. These files were downloaded from that tag:

- `server/plugins/LFUGens.cpp`: envelope and Line/XLine definitions.
- `server/plugins/FilterUGens.cpp`: filter and resonator recurrences.
- `server/plugins/DelayUGens.cpp`, `include/plugin_interface/SC_InlineUnaryOp.h`: Delay, Comb and Allpass families and signed decay/feedback semantics.
- `server/plugins/OscUGens.cpp`: oscillator phase and Klang/Klank recurrences.
- `server/plugins/PanUGens.cpp`: Pan2 table quantization and interpolation.
- `server/plugins/IOUGens.cpp`: LagControl/TrigControl initialization and processing.
- `server/plugins/NoiseUGens.cpp`, `include/plugin_interface/SC_RGen.h`, `include/plugin_interface/Hash.h`: noise kernels, taus88 generator and integer seed hash.
- `server/plugins/BinaryOpUGens.cpp`, `server/plugins/MulAddUGens.cpp`: mixed-rate arithmetic interpolation.
- `common/Samp.cpp`, `server/scsynth/SC_World.cpp`: shared sine/cosecant table definitions and plugin table wiring.

The source headers credit James McCartney and license the code under GPL-2.0-or-later. The engine's adaptations are distributed under GPL-3.0-or-later; `../LICENSE` is the GPLv3 text downloaded from the same tag's `COPYING`. These reference C++ files are not linked into the new C runtime.

Format reference: [Synth Definition File Format](https://doc.sccode.org/Reference/Synth-Definition-File-Format.html). The runtime accepts a bounded subset of format version 2; see the engine README for deliberate exclusions.

Fixtures were exported by the installed `sclang` 3.14.1 and rendered by `scsynth` 3.14.1. The export programs are `../tools/export-fixtures.scd`, `export-stage2.scd`, and `export-stage3.scd`, `export-delays.scd`, `export-random.scd`, orchestrated by `../tools/export-all.py`; `fixtures.json` records file hashes. `sources.json` records reference-source hashes. Tests restrict the server's plug-in search to the official installation directory, so user extension plug-ins are excluded from the renderer.

On this installation, offline renders containing Klang/Klank crashed in `Graph_Dtor` during final world cleanup. The scores explicitly free test group 1 at 0.51 seconds before the final marker at 0.52 seconds. Comparisons use the first 345 blocks (about 0.501 seconds); this workaround therefore does not replace or truncate the compared sound. Every reference invocation must exit successfully and without server errors.

The seeded `m5_addy` integration score ends at 1.21 seconds; its comparison uses 827 blocks (about 1.20 seconds) to cover the complete voice lifetime and subsequent silence.
