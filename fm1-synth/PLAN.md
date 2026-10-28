# Historical development plan: FM-1 SynthDef instrument checklist

This file preserves design discussions and milestone checklists. For current behaviour, use README.md and the root docs; prospective statements below may have been completed.

User decisions: eight slots (28 October 2026), Bluetooth disabled, code on a white screen, keyboard velocity, four normalized knobs, SELECT scrolls code, PRESET selects slots. Existing verified template remains the recovery baseline.

## Build order

- [x] Portable performance controller: note identity, pitch aliases, velocity, gate/release, octave keys, bounded polyphony, stealing, panic, live knobs and stereo meters. Two voices enabled on the device and host; live audio contracts tested.
- [x] Eight-slot format and power-cut-safe flash journal; capacity and CRC validation; metadata and source bundled with compiled graph. Verified on simulated NOR and a persistent Mac flash image.
- [x] Browser manager: paste/edit/compile source, import binary, rename/clear/export/import slots, visible capacity and compatibility errors, device-sized code preview. Local bank and explicit capability-checked USB transfer controls are implemented.
- [x] Screen layout and lexical highlighting: white/black, blue classes/UGens, red `arg`/`var`, purple numbers, monospace, wrapping and SELECT scrolling, temporary knob overlay. Browser preview and actual fm1-base host display tested.
- [x] DelayN/L/C, CombN/L/C, AllpassN/L/C: bounded per-voice memory, reference tests including modulation and negative decay. 99 additional SuperCollider comparisons passed.
- [x] USB protocol core and browser codec: capability/version negotiation, bounded chunks, checksums, acknowledgements/retries, conflict detection, staged commit and readback. Tested in memory and connected to the hardware slot protocol.
- [x] fm1-base host adapter: real panel, display and audio contracts; reads the same bank image. Bluetooth disabled. Terminus 12 monospace on the simulated display; Monaco/Menlo in the browser.
- [x] Target adapter: serialize panel/MIDI commands onto the audio thread, connect APP_SYSEX to the protocol, validate packages/graphs before commit, implement physical flash callbacks and safe save silence/resume. Do not write flash from rendering.
- [x] Target float maths and firmware memory map; bounded panel queue and physical slot protocol.
- [x] Recorded and sustained two-voice hardware timing/audio acceptance for Keys and five-harmonic Addy. See `../logs/synthdef-polyphony-2026-10-28/verification.json`; other patches require their own timing checks.
- [ ] Hardware acceptance: code scrolling/colours, velocity and held notes across octave changes, knob updates, slots, interruption/reboot recovery and stereo metering.

## Contracts

### Added compatibility target: user's `addy` additive patch

Compiled successfully with SuperCollider 3.14.1 on 2026-10-28. Evidence and original expression: `../logs/addy-compatibility-2026-10-28/`. The original 25-harmonic graph remains outside the current profile. The five-harmonic version is now installed in device slot 2; the original 25-harmonic graph remains a larger stress case.

The user subsequently reduced `max_harmonics` to 5. That version is saved as `examples/addy-5.scd` and compiles to 5,739 bytes, 124 nodes, 135 signals and 336 input references. Host profile 4 now accepts it, with limits of 160 nodes and 192 signals, Rand/ExpRand and parameterized output bus zero. Its 7,062-byte package fits the existing eight-slot format. A seeded reference compares its entire voice lifetime against SuperCollider (peak error below 6e-8), and the unchanged source compiles/previews through the manager. Retain the 25-harmonic measurements below as a larger stress case.

- [x] Add Rand and ExpRand with per-instance initialization semantics, including control-valued bounds sampled at construction (`randamp` in this patch). These are separate from the existing noise UGens and RandSeed.
- [ ] Optimize/budget large expanded graphs: this patch has 590 nodes, 601 signals and 1,622 input references, versus current limits 160/192/1,024. It builds all 25 harmonic branches even when `high_harmonic=5`; multiplication by a control condition does not remove them.
- [ ] Decide compact storage/import representation: its binary is 26,617 bytes before 1,111 bytes of source and metadata, exceeding the 16,128-byte slot payload. Raising the graph limit alone cannot fix this.
- [x] Support conventional parameterized `Out` bus zero with bounded routing. `Out.ar(output, ...)` accepts initial/control bus zero; nonzero runtime values fault and silence that voice. Default bus 0 is the main stereo pair.
- [x] Validate the five-harmonic sound and complete lifetime against SuperCollider, and compile/package/render through the Mac manager.
- [x] Validate target memory placement, two-voice processing budget and USB audio for the five-harmonic patch. Physical listening of this build remains a user check. Eight slots remain; reducing slots does not reduce render RAM.

User-code observations: `pan` and `out` are unused; `output` is the bus actually used. `dur` appears both in Env.perc and in timeScale, so segment times multiply by it again. The independent Line doneAction can truncate long envelopes for some settings. `randamp` affects new voices only under standard ExpRand semantics; harmonic range controls can affect sounding voices. There are no `knob1`–`knob4` bindings in the expression yet.

Eight visible slots are numbered 1–8 (wire/API indexes 0–7). Each has two 16 KiB banks. The first 256 bytes in a bank are reserved for its commit header; its payload is at most **16,128 bytes**, including metadata, UTF-8 source and compiled `.scsyndef`. The slot region is `[0x97000, 0xD7000)` (256 KiB), leaving `[0xD7000, 0xDC000)` unused. Panel calibration at 0xDC000, BLE objects at 0xDE000, update staging at 0xE0000 and settings at 0xFC000 are untouched. This is based on the pinned fm1-base flash map, not a larger replacement chip.

A compiled SynthDef cannot recover original source. Binary-only imports show a clear source-unavailable message and a graph summary. Editing source creates a draft: only an explicit successful compile can pair it with a new binary. Saving an uncompiled draft must not silently relabel old audio as new code.

Each physical press has an identity independent of its current pitch. OCT± shifts subsequent keyboard notes by 12 semitones, bounded to ±2 octaves; held notes retain their original note-off identity. Incoming MIDI uses its original pitch (not panel octave). Note-on velocity 0 is note-off. Standard controls are `freq = 440 * 2^((midinote-69)/12)`, `midinote`, `note = midinote - 60`, `pitch = midinote` (our explicit alias), `amp = velocity/127`, `gate = 1/0`, `velocity = velocity/127`. Missing arguments are harmless. There is no implicit major-scale `degree` mapping or complete Event interpreter.

`knob1`–`knob4` are control-rate arguments, normalized 0–1, initially the SynthDef defaults (or 0.5 when absent), moved in 0.02 increments and clamped. They affect active and future voices. Use `Lag.kr(knob1, 0.02)` or lagged controls to smooth audible changes. Values flash for 1.2 seconds. Runtime values are separate from source text; turning a knob never rewrites code or flash. Explicit save can persist knob defaults later.

PRESET moves through all eight slots, including empty ones, and releases the old patch before loading another. SELECT moves by wrapped display lines. Screen header and L/R output meters stay visible. Meter tap is post-master, pre-output-clamp, with peak decay and clip indication. Empty slots are silent. HOME/panic is available to stop hanging notes. Voice count is provisional until target CPU/RAM measurements.

The current simulator silences the old patch on a slot change. A short transition fade, event queue overflow policy, sustain pedal, power-cycle settings retention and a validated instrument-wide bank export are remaining refinements. Normal no-gate note-off fades over 128 samples; a gate-controlled released voice is retired after at most 10 seconds as a bounded resource policy. The 1.2-second knob overlay and live updates are implemented in the simulator. Browser Listen is an offline render; its knobs affect the next preview, not the WAV already playing.

The first browser manager operates on a clearly labelled local bank and compiles through the installed SuperCollider. Compilation evaluates the code on this Mac only when Compile is requested. A standalone browser cannot compile arbitrary SuperCollider source by itself. Hardware management will use USB MIDI SysEx after capability negotiation with the new firmware; the existing TEMPLATE firmware must never be treated as a slot-capable device.

## Evidence and references

- `../fm1-base/include/plat.h`: 27 keys starting at MIDI 53, encoder/button roles, flash map and app SysEx hooks.
- `../fm1-base/device/storage.c`: existing two-copy journal pattern; larger slot records need their own bounded journal.
- [SuperCollider Event](https://doc.sccode.org/Classes/Event.html): note/frequency hierarchy.
- [SynthDef](https://doc.sccode.org/Classes/SynthDef.html): graph creation and binary export.
- [Web MIDI](https://developer.mozilla.org/en-US/docs/Web/API/Web_MIDI_API): secure-context and SysEx permission requirements.
- [CombC](https://doc.sccode.org/Classes/CombC.html): cubic delay and signed decay/feedback semantics.

## Permanent effect slot (28 October 2026)

- [x] Additional slot 0 without removing synth slots 1–8; 7,936-byte payload and two 8 KiB journal banks in previously unused flash.
- [x] `In.ar(0, 1/2)` and one persistent post-mix effect, before MASTER/metering; dry bypass when empty; tails survive preset changes.
- [x] Shared delay arena, bounds checked for every synth/effect combination at save; interrupted effect writes tested at all 34 erase/program boundaries.
- [x] All four named knobs feed both graphs. Current Keys uses 1–2; echo uses 3–4 for amount/time. No hardwired split.
- [x] Browser compile, save, preview through effect, slot read/upload with full readback, and clear/bypass controls.
- [x] Two-voice Keys/Addy stress tests with effect: 42 chords, no overloads/overruns/faults. USB audio confirms dry release reaches silence and wet tail continues after a preset switch.
- [x] PRESET slot 0 is a code view, separate from the selected playable synth. SELECT scrolls its code; header identifies the last selected synth. Host and device tests preserve two held notes while browsing in and out.

## Independent synth / effect selection (28 October 2026)

- [x] Eight synth slots retained; four FX presets share the old effect journal atomically.
- [x] PRESET selects synth; ALGORITHM selects FX or bypass; FX is a latching toggle with LED.
- [x] Separate live knobs 1–4 for synths and 5–8 for effects; values survive toggling.
- [x] FX code focus and SELECT scrolling; held notes survive effect selection and focus changes.
- [x] Fade-down/swap/fade-up effect replacement; note-offs retained during swap.
- [x] Browser edit/import/export/preview for all twelve slots and shared capacity display.
- [x] Legacy effect migration and original-package backups; unchanged synth storage.
- [x] Core/sanitizer, interrupted-write, host panel and device control tests.

This supersedes the earlier shared-knob and PRESET-to-slot-0 design above.
