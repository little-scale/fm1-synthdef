# Credits, provenance and licences

FM1 SynthDef combines the following components. Original source headers and licence files remain in place; this document does not replace their terms.

| Component | Provenance | Included licence / notice |
| --- | --- | --- |
| New SynthDef engine, instrument, manager and release tooling | This project | GPL-3.0-or-later; root `LICENSE` and `synthdef-engine/LICENSE` |
| Adapted DSP algorithms and reference material | SuperCollider 3.14.1; source credits include James McCartney and other named contributors | GPL-2.0-or-later sources, adaptations distributed under GPL-3.0-or-later; `synthdef-engine/reference/README.md`, original headers and source hash manifest |
| Gendy1 / Gendy2 / Gendy3 | SuperCollider 3.14.1, Nick Collins; stochastic synthesis by Iannis Xenakis / Marie-Helene Serra | GPL-2.0-or-later original `GendynUGens.cpp`, GPL-3.0-or-later C adaptation; pinned hash in reference manifest |
| FreeVerb / FreeVerb2 kernels | SuperCollider 3.14.1, blackrain | GPL-2.0-or-later original, GPL-3.0-or-later adaptation; pinned ReverbUGens.cpp and generated-kernel notice |
| MiVerb wrapper and language class | Volker Böhm, mi-UGens commit `10f6824f68ad63475f67dfe3fe5016e2d2634f84` | GPL-3.0-or-later; original notices in `synthdef-engine/reference/mi-ugens/` |
| MiMu distortion / μ-law codec | Volker Böhm / Émilie Gillet, mi-UGens commit `10f6824f68ad63475f67dfe3fe5016e2d2634f84` | MIT notice retained in `synthdef-engine/reference/mi-ugens/projects/MiMu/MiMu.cpp`; C adaptation distributed with this GPL-3.0-or-later project |
| MiVerb reverb, delay engine and cosine oscillator | Émilie Gillet / Mutable Instruments, same pinned source tree | MIT, full notices retained in original headers; C adaptation distributed with this GPL-3.0-or-later project |
| SuperCollider plugin headers used only by tests | SuperCollider 3.14.1 | Original per-file notices and hashes in `synthdef-engine/reference/sc-headers/`; not linked into device firmware |
| fm1-base | Charles Vestal and credited contributors, pinned snapshot | `fm1-base/LICENSE`, `LICENSING.md`, `CREDITS.md` and `LICENSES/` |
| felucca-bsp | Hügelton Instruments and credited contributors, pinned snapshot | `fm1-base/bsp/LICENSE` and original headers |
| Terminus and Barlow fonts | Dimitar Toshkov Zhekov; The Barlow Project Authors | SIL OFL 1.1 texts in `fm1-base/assets/fonts/` |
| JieLi SDK package data | JieLi AC79NN SDK V1.2.1, exact hashes in setup | Apache-2.0, `dependencies/ac79-sdk/LICENSE`; three unmodified package data files only |

Apple Monaco is referenced as an optional locally installed browser font; no Monaco font file is distributed. SuperCollider, Python packages, Node, Docker and the JieLi compiler are external build/runtime dependencies and are not bundled. The compiler archive hash and source pins are in `setup/dependencies.json`.

The platform and BSP snapshots are unchanged from the recorded commits. Their original Git repositories and exact revisions are recorded in `setup/dependencies.json`. The snapshots exclude development checkouts' Git metadata and unrelated untracked work. No proprietary Bluetooth or WiFi libraries are linked into this no-radio firmware.

The Addy example was supplied during development and adapted to five harmonics. Example source is distributed with the project's GPL-3.0-or-later code. This independent project does not claim endorsement by M-VAVE, SuperCollider, JieLi or the upstream platform authors.
