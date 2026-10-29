# Verification record

## UGen expansion — 30 October 2026

The development engine now implements **109 UGen types**, adding the 32 types listed in [the roadmap](UGEN-ROADMAP.md), seven binary selectors and SelectX helper compatibility. **Hardware checks and installation are deferred at the user's request.** The existing preview release and user slot banks are unchanged.

- **805/805 reference graphs pass on ordinary host, optimized host and freestanding device-math backends.** They cover all existing behavior plus 252 new fixtures: initial/control/audio rates, constructor state, changing controls, reset priority, held triggers, seeded noise, frequency changes, panning, filter coefficient changes, quantization and bypass.
- **1,048,576 identical-input MiMu samples** match the pinned upstream implementation bit for bit, with control/audio gain, both input signs, saturation and changing bypass. The eight existing MiVerb oracle cases still pass.
- **40 engine API groups**, **640,000 hostile graph mutations**, the long-decay regression and instrument core/bank/input/screen/transport tests pass.
- Five two-voice synth/effect scenarios cover all eight knobs at 0, 0.25, 0.5, 0.75 and 1, changes during playback, stealing and note-off. Each runs 140,800 frames. All produced nonzero stereo audio, no numerical faults and silence after release; peak was below 0.69. These are emulation checks, not realtime device measurements.
- An isolated local editor successfully compiled, saved and previewed the new synth through MiMu using a temporary bank. The user's bank and running editor were left untouched. Restart the existing editor to load the updated native engine.

The new filters use a difference-state realization to avoid low-cutoff cancellation. MiMu's PCM conversion and control-rate SinOsc frequency increments retain reference precision without software double helpers. The target image is **246,560 bytes**, with **78,480/98,304 bytes static RAM** and **326,008/344,064 bytes pool use**. The package remains 610,136 bytes, Bluetooth disabled. A successful build establishes fit, not realtime CPU capacity.

The MiRings upstream host probe produced valid audio for six models and six alternate-mode settings, but measured approximately **180,048 bytes per complete instance**, excluding wrapper overhead. Full MiRings remains unsupported because that exceeds the current shared arena and per-voice budgets. See [the feasibility result](../evidence/mirings-feasibility-2026-10-30.json).

Recorded results and source/package hashes: [expansion evidence](../evidence/ugen-expansion-2026-10-30.json). Physical acceptance still needs slot readback, two voices through the chosen master effect, live knobs, voice stealing, release, CPU/overrun checks and a power cycle.

## Dusty decay and density controls — 29 October 2026

Dusty in slot 5 now maps knob 1 to each burst's Decay2 decay (0–1 second, default 0.2), and knob 2 to Dust density (0–10 triggers/second per stream, default 5). All five streams remain independent. The global envelope has a 10 ms attack and five-second decay, with done action 2 retained for automatic cleanup. Its shared carrier/envelope optimization is retained; the graph has 31 nodes and the package is 1,984 bytes. No firmware changes or flash were needed.

The 15 Mix/Splay helper comparisons pass. Comparisons against the unfactored requested graph at six knob settings remain within one 16-bit PCM step, including zero density and minimum/maximum decay. Separate comparisons establish that each knob changes the output. The actual host adapter passes 73 checks, including live encoder changes, two voices, all four effects, bypass, voice stealing and automatic five-second retirement.

The installed patch passed **28 hardware checkpoints** with two voices, all four effects and bypass, live minimum/maximum controls, stealing and envelope cleanup. There were no new overloads, overruns or numerical faults; sampled render time reached 4,788 microseconds. All twelve saved slot payloads were preserved during testing, and the Mac/device slot 5 readbacks match. An initial run stopped on an unacknowledged encoder command; the passing driver retries from actual readback. Default controls (0.2 / 0.5) and physical MASTER were restored. Exact package hashes and results are recorded in [Dusty control evidence](../evidence/dusty-controls.json). The earlier three-second Dusty results below are historical.


## Dusty, Mix and Splay — 29 October 2026

The SynthDef firmware was reinstalled after another firmware had changed the synth bank. Keys, Addy, Triple saw and Two-op FM were restored from saved packages with user approval; Dusty occupies synth slot 5. The four effect presets were retained.

`Mix.ar` and `Splay.ar` already expand into supported nodes. **15 helper comparisons** cover summation, scalar/control conversion, stereo arrays, spreading, centering and compensation. Dusty's five independent Dust/Decay2 streams are retained. Sharing its squared-sine carrier and factoring the common envelope after the stereo sum reduces 42 nodes to **30**, with at most one 16-bit PCM step difference over the two-second comparison. Automatic envelope completion frees voices after 3.01 seconds; the supplied knob and gate arguments remain unused.

Dust/Decay kernels now bind inputs once per block, cache unchanged coefficients and keep recurrence state local to the loop. Sparse zero-input decay uses compensated addition. **6.3 million samples** across 21 long-decay cases stay within **5.95e-8 normalized error** of a double-precision recurrence. All **553 reference graphs** passed on three backends after the sparse-loop changes; the final local-state change also passed all target reference cases and 34 API groups. The eight MiVerb oracle cases pass. **440,000 sanitizer cases** passed before the final arithmetic-equivalent local-state caching change. Instrument core/bank checks pass.

The actual host adapter passed **42 Dusty assertions**. The installed **229,012-byte** build passed **18 hardware checkpoints**, playing two Dusty voices through bypass, echo, FreeVerb, FreeVerb2 and MiVerb, with release, automatic cleanup and voice stealing. USB capture contained nonzero stereo audio. No new overloads, overruns or numerical faults occurred; sampled render time reached **4,781 microseconds**. All 12 saved slots matched before/after testing. Static RAM remains **78,480 bytes**, pool use **326,008 bytes**, and Bluetooth is disabled.

The original 42-node patch overloaded the device; even after renderer optimization it exceeded budget with reverb. The accepted run uses the equivalent 30-node graph and begins with that earlier overload already counted. This is scoped acceptance, not a guarantee for every pitch/control setting or arbitrary synth/FX combination. The previous Addy/FreeVerb limitation remains. Physical listening and a final-build power cycle remain user checks. Exact hashes and results are in [Dusty evidence](../evidence/dusty.json). The published preview binary is unchanged.



## Independent synth / FX controls — 28 October 2026

The installed development build is **227,828 bytes**, with **78,480/98,304 bytes static RAM** and **326,008/344,064 bytes pool use**. PRESET selects eight synths, ALGORITHM selects four FX presets or bypass, and the FX button latches independent knob banks 1–4 / 5–8 and code views. The eight original synth packages were read back unchanged. The legacy echo was preserved with its control names migrated, and three empty effect presets received FreeVerb, FreeVerb2 and MiVerb.

Core tests, seven Python package/bank groups, highlighting/framing tests and undefined-behaviour/bounds/float-cast sanitizers pass. Storage tests cover 68 interrupted synth writes, 34 full effect writes and 32 four-effect commits, including neighbor preservation, shared revisions and capacity refusal. The actual host adapter passes **277 assertions**: original panel 23, extensions 31, reverb matrix 114, battery 42, and new FX-bank controls 67.

The exact installed build passed **28 hardware checkpoints** with stereo USB audio: held two-note Keys chords across all four effects and bypass, latching while holding/releasing FX, independent live knobs, effect-code focus, synth changes preserving effect controls, and note-offs during effect replacement. No new overruns, overloads or numerical faults occurred; sampled render time peaked at **3,118 microseconds**. All twelve saved slots matched before/after testing. An earlier attempt missed one unacknowledged remote encoder command; the final passing driver reads each value back before the next command. This result does not establish a physical encoder fault. One overload was already counted before the final run began, before its notes; the count remained at one throughout acceptance. Its cause is unconfirmed, so cold-start acceptance is still incomplete.

The browser passed toggle/value retention, effect compile → local save/readback → audio preview, and shared-capacity display checks. The effect bank uses 4,255 of its 7,916 package bytes. The final build still needs user listening and power-cycle confirmation. These checks do not certify arbitrary synth/FX combinations: the prior two-Addy-voices + FreeVerb overload remains relevant. Exact hashes and scoped results are in [FX-bank evidence](../evidence/fx-banks.json). Earlier results below refer to their recorded builds; release v0.1.0-preview is unchanged.

## Battery indicator and reverb hardware checks — 28 October 2026

The current development build is installed on the test FM-1. It adds a top-right three-bar battery icon and USB-host power symbol in both synth and effect code views. The physical sensor read 628 (three bars); six simulator states passed 42 assertions. ADC ownership, scan restoration, polling and hysteresis checks passed. A ten-second idle observation showed stable sensor readings and no new ADC timeouts. Battery discharge calibration and the unplugged physical display have not been verified.

FreeVerb, FreeVerb2 and MiVerb each passed **63 hardware assertions (189 total)** with **Keys, triple saw, two-op FM and Dust chimes**, using two voices at default/minimum/maximum knobs, live controls, stealing, release, FX viewing and panic. All produced nonzero stereo USB recordings and measurable post-release tails, with no new overloads, overruns or numerical faults. Tails use separate uninterrupted recordings because USB recordings can omit frames during patch loads. Per-effect sampled render maxima were 3,607 / 3,651 / 3,800 microseconds; these are sampled values, not exhaustive maxima.

**Two Addy voices through FreeVerb exceed the processing budget.** The overload guard silenced that combination at a measured 5,409 microseconds in the optimized build. Two Addy voices with FreeVerb2/MiVerb have not been accepted. Import validation establishes memory capacity, not realtime capacity.

The final application is **226,104 bytes**, with **78,432/98,304 bytes static RAM** and **325,992/344,064 bytes pool use**. All 553 reference comparisons still pass on three backends, the eight identical-input MiVerb comparisons remain exact, and 440,000 sanitizer cases pass. Reverb kernels are kept outside the main renderer; packed MiVerb samples use alias-safe byte access. Performance diagnostics now subtract the wrapping tick counter before converting to microseconds.

Two earlier runs registered extra notes despite the user not touching the instrument. Input-source diagnostics later traced an event to the keyboard-sensing path while battery sampling was disabled. New presses now require contact-matrix confirmation while retaining velocity measurement and analog release. This did not establish the underlying cause: subsequent events appeared through both sensing paths too.

After the user confirmed the keys were clear, the final contact-confirmation build passed **190 two-note chords over at least two minutes**, alternating battery sampling off/on/off/on, with zero unexpected keyboard/MIDI notes, overloads, overruns or numerical faults. Sampling and physical MASTER control were restored. Ten contact regression cases and 210 host panel assertions pass. Physical light-keypress feel remains a user check. The earlier source-traced 189 reverb checks used the preceding diagnostic build; its DSP is unchanged in the final build.

All nine original slot contents were verified after restoration. The restored echo also passed 42 two-voice Keys/Addy chords (60 checks), with no new overloads, overruns or faults. The published preview download is unchanged. Physical listening and a final-build power cycle remain user checks. Exact build/source hashes and scoped results are in [battery evidence](../evidence/battery-indicator.json) and [reverb hardware evidence](../evidence/reverbs-hardware.json).

## Development reverbs — 28 October 2026

FreeVerb, FreeVerb2 and MiVerb bring the development profile to **77 UGen types**. At the initial emulation run no FM-1 USB MIDI port was available. Subsequent device results are recorded above. The published preview firmware is unchanged.

- **553/553 reference graphs** pass on ordinary host, optimized host and freestanding device-math backends. The 37 new graphs run for approximately four seconds each, covering wet/dry, stereo excitation, parameter clamps, noise bursts, live changes, diffusion, freeze and saturation.
- An isolated MiVerb plugin is built from pinned upstream source and pinned SC headers. No installed user plugins are used by the reference renderer.
- An additional identical-input comparison matches the unmodified upstream MiVerb DSP exactly for **819,200 stereo frames** across 8 kHz, 44.1 kHz, 48 kHz and 192 kHz, at ordinary and overdriven input levels, with freeze and parameter changes. The oscillator-driven saturation integration case has a separate peak 2e-4 / RMS 2e-5 tolerance because tiny upstream oscillator differences enter quantized feedback.
- **34 API groups**, including reverb capacity, exact buffer guards, failed binding, missing memory, restart, preparation, scratch sharing and invalid rates/layouts.
- **440,000 sanitizer mutation/truncation cases** across 22 graph families. Instrument journal/core sanitizer checks, six bank/package tests and screen/transport checks pass.
- **114 reverb panel assertions** across six simulations (three reverbs × Keys/Dust chimes), with two voices, stealing, live knobs, release tails, effect browsing and panic. All produce nonzero stereo recordings and measurable post-release tails without numerical faults. The earlier 23 + 31 panel assertions also pass.
- Firmware builds at **226,972 bytes**, with Bluetooth disabled, no software-double routines, unchanged static RAM **78,384/98,304 bytes**, and unchanged pool use **325,992/344,064 bytes**.

Reverb storage is reserved from the existing arena only when used: FreeVerb 50,492 bytes; FreeVerb2 102,088 bytes; MiVerb 65,536 bytes. Memory admission does not prove realtime performance. The examples are available as source, compiled SynthDefs and slot packages. They were initially tested in emulation; the later device run is recorded above. See `evidence/reverbs-emulation.json` and its companion comparison records.

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
