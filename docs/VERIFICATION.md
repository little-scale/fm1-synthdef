# Verification record

## Development reverbs — 28 October 2026

FreeVerb, FreeVerb2 and MiVerb bring the development profile to **77 UGen types**. No FM-1 USB MIDI port was available, so these results cover emulation and compilation only; physical playback/timing is pending. The published preview firmware is unchanged.

- **553/553 reference graphs** pass on ordinary host, optimized host and freestanding device-math backends. The 37 new graphs run for approximately four seconds each, covering wet/dry, stereo excitation, parameter clamps, noise bursts, live changes, diffusion, freeze and saturation.
- An isolated MiVerb plugin is built from pinned upstream source and pinned SC headers. No installed user plugins are used by the reference renderer.
- An additional identical-input comparison matches the unmodified upstream MiVerb DSP exactly for **819,200 stereo frames** across 8 kHz, 44.1 kHz, 48 kHz and 192 kHz, at ordinary and overdriven input levels, with freeze and parameter changes. The oscillator-driven saturation integration case has a separate peak 2e-4 / RMS 2e-5 tolerance because tiny upstream oscillator differences enter quantized feedback.
- **34 API groups**, including reverb capacity, exact buffer guards, failed binding, missing memory, restart, preparation, scratch sharing and invalid rates/layouts.
- **440,000 sanitizer mutation/truncation cases** across 22 graph families. Instrument journal/core sanitizer checks, six bank/package tests and screen/transport checks pass.
- **114 reverb panel assertions** across six simulations (three reverbs × Keys/Dust chimes), with two voices, stealing, live knobs, release tails, effect browsing and panic. All produce nonzero stereo recordings and measurable post-release tails without numerical faults. The earlier 23 + 31 panel assertions also pass.
- Firmware builds at **226,972 bytes**, with Bluetooth disabled, no software-double routines, unchanged static RAM **78,384/98,304 bytes**, and unchanged pool use **325,992/344,064 bytes**.

Reverb storage is reserved from the existing arena only when used: FreeVerb 50,492 bytes; FreeVerb2 102,088 bytes; MiVerb 65,536 bytes. Memory admission does not prove realtime performance. The examples are available as source, compiled SynthDefs and slot packages; none was sent to a device. See `evidence/reverbs-emulation.json` and its companion comparison records.

## Development UGen expansion — 28 October 2026

The 74-type development source adds 21 UGens and five unary conversions. These results are **emulation/build verification**, not new hardware acceptance. The connected FM-1 and v0.1.0-preview release binaries remain unchanged.

- 516/516 SuperCollider 3.14.1 comparisons passed on the ordinary host, optimized host and freestanding device-math backends.
- 31 engine API groups passed, including multiple sample rates for timing information, restart/preparation, shared scratch, invalid rates/domains and timing-oracle checks.
- 380,000 engine sanitizer mutation/truncation cases passed across 19 graph families.
- Instrument core and sanitizer checks passed, including 68 synth and 34 effect interrupted-write boundaries; six package/bank tests and screen/protocol tests passed.
- The existing panel simulation passed 23 assertions. A new temporary-bank simulation passed 31 assertions with two voices, stealing, knob extremes, release, FX code browsing and master echo. Its stereo recording peaks at approximately 0.214 full scale, with no numerical faults.
- New development firmware compiled with Bluetooth disabled and no software-double routines. Application size: 215,956 bytes. Static RAM remains 78,384/98,304 bytes; pool remains 325,992/344,064 bytes.

Dust/Decay/Decay2 are covered at audio and control rates, including seeded random density changes and zero/equal/near-equal/very short Decay2 times. One sinusoidally frequency-modulated Impulse test compares pulse count/amplitude and allows at most one sample of timing difference; all other fixtures use waveform limits. Exact-valued Impulse frequency modulation has a separate waveform comparison. The engine README records the pinned TExpRand audio-trigger quirk and all input restrictions.

See `evidence/ugen-expansion-emulation.json` and the accompanying per-fixture comparison records for source/build hashes and results. Host render speed does not measure FM-1 realtime capacity. Physical playback/timing and power-cycle tests of this new build remain outstanding; the release packager still requires hardware-accepted hashes.

## Published 0.1.0-preview

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
