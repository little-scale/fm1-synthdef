# Verification record — 0.1.0-preview

Acceptance on 28 October 2026 used one FM-1, Apple silicon macOS, SuperCollider 3.14.1 and the pinned compiler/platform sources. Evidence JSON is in `evidence/`.

- 326 reference graphs passed on the host and device-math backends.
- 26 engine API groups passed, including stereo effect input.
- 220,000 engine sanitizer mutation/truncation cases passed.
- Slot journal interruption checks covered 68 synth and 34 effect write boundaries.
- The actual panel adapter passed 23 simulator assertions.
- Two-voice Keys/Addy playback through master echo passed 42 chords without overloads, overruns or faults.
- Four effect-code view toggles preserved two held notes and synth selection.
- USB recordings measured an echo tail after release and after a synth preset change.
- Triple saw passed 17 hardware checks; two-op FM passed 13. Their checks covered live knob extremes, low/high notes and release completion, with nonzero stereo recordings and no additional overloads or overruns.

Image size was 207,904 bytes; static RAM 78,384 bytes; pool use 325,992 of 344,064 bytes. See `evidence/master-effect.json` for the exact accepted package and application hashes. Release packaging verifies those hashes.

Cumulative overload counters include earlier experiments. The rejected three-band-limited-saw patch overloaded; the supplied triple-saw patch uses LFSaw and passed with no new overloads. Historical maximum telemetry is not represented as the maximum of a later test. Sampled per-callback times are labelled as sampled, not exhaustive maxima.

The user confirmed audible operation of the template and master-effect setup, and a power cycle of the earlier template. A power-cycle persistence/recovery test of the final preview firmware has not been recorded. Tests do not certify every graph, every parameter range, every host OS, or arbitrary CPU/memory combinations. No claim of bit-identical scsynth output is made.

The release preparation also rebuilds/tests from its clean source tree. Its fresh results are recorded in `evidence/release-checks.json` when present.
