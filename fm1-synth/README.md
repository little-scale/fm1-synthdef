# FM-1 SynthDef instrument infrastructure

This is a Mac editor, a portable instrument/storage/protocol core, and an fm1-base device and host adapter. Two-voice playback of Keys and Addy passed the recorded and sustained device tests; see [the verification record](../evidence/master-effect.json). Other patches still need their own timing checks. The complete [checklist](PLAN.md) distinguishes tested host features from remaining device work.

## Run

```sh
cd fm1-synth
make test
make serve
```

Open the launch URL printed by the server. It binds only to `127.0.0.1` and uses a session token. The editor saves to `data/local-bank.flash`, a simulated 1 MiB chip image. The local bank is separate from hardware. **Check FM-1** reads the twelve device slot names; **Read FM-1 slot** imports the selected device slot as an editor draft; **Send to FM-1 slot N** explicitly writes that device slot and verifies it by readback. Device transfers require the SynthDef capability handshake and a matching slot revision. Stop the server with Ctrl-C; the local bank survives restart. Do not run two manager processes against the same bank file.

The editor supports paste, `.scd`, `.scsyndef`, and exported `.fmslot` packages. Compile evaluates one SynthDef expression using the installed SuperCollider, validates the graph, checks the slot/delay budget and enables Save. Editing code invalidates its compiled state. A binary-only import explicitly has no source. Export packages include the name, source, compiled graph, SHA-256 hashes and a whole-body CRC. No source is reconstructed or falsely labelled as the original.

Listen renders a two-second note with note-off after one second through the native controller. It is an offline preview: slider changes apply to the next render. The browser shows both a code editor and a 240×240 screen preview with fixed header/meters, wrapped code and scrolling.

## Keyboard and knobs

Each press starts a voice with `freq`, `midinote`, `pitch` (MIDI alias), `note` (MIDI minus 60), `amp` and `velocity` (both velocity/127), and `gate=1`, where those arguments exist. Note-off sets `gate=0`; voices without a writable gate get a short release fade. OCT± affects only subsequent panel notes, ±2 octaves. Incoming MIDI is not transposed. Note identity includes MIDI port/channel, or the physical key, so an octave change cannot strand a note.

Use **control-rate arguments** `knob1` through `knob4`, or `NamedControl.kr` with those names. Ordinary local `var` declarations do not expose controls. Every detent changes the value by 0.02, clamped to 0–1. Defaults come from the SynthDef. Updates are passed with `fm_set` to all sounding voices between blocks; new voices receive the current values. Press FX to latch the effect control bank; the physical knobs then send `knob5`–`knob8` only to the effect. The FX LED and code view follow the toggle. Values survive toggling and update live. For smoothing:

```supercollider
arg knob1=0.5;
var cutoff = 200 + (Lag.kr(knob1, 0.02) * 6000);
```

Initial-rate or trigger controls named `knob1`–`knob4` in synths or `knob5`–`knob8` in effects are rejected by instrument validation because they cannot provide this persistent live behaviour. The engine itself still supports these control types under other names. The [Keys + comb example](examples/keys.scd) uses all four knobs.

Two voices are allocated, with oldest released voice stolen first and a 128-sample transition. Audio clipping is bounded; meter values are measured before the final clamp and include a one-second clip indicator. This is not an automatic loudness normalizer or look-ahead limiter. A released gate voice has a ten-second lifetime cap.

## Eight slots and delay RAM

Each slot uses two 16 KiB banks, totaling 256 KiB for all eight slots. Its **16,128-byte payload** includes metadata, source and compiled graph. CRC-checked headers are written last, and saves compare the expected revision before replacing anything. Calibration, update staging and platform settings regions are excluded by the address map. Turning knobs never writes flash.

Flash space and delay RAM are separate. DelayN/L/C, CombN/L/C and AllpassN/L/C use caller-owned buffers. The instrument has a **32,768-float / 128 KiB shared delay arena** for both synth voices plus slot 0. A synth voice is limited to 16,384 samples / 64 KiB, and its actual required size is allocated for each of the two voices. The effect uses the high end of the same arena. Ring lengths round up to powers of two with processing overhead; a single `maxdelaytime: 0.2` at 44.1 kHz uses the full 64 KiB. The engine's general host API allows a larger caller budget. The manager rejects patches exceeding the instrument budget before save.

The host profile accepts **160 nodes and 192 signals**. [Addy, five harmonics](examples/addy-5.scd) uses 124 nodes and 135 signals; its source, binary and metadata occupy 7,062 bytes. `Rand` and `ExpRand` are supported and sample their bounds once per note. The compiled binary, importable `.fmslot` and WAV preview are next to the example. Default output bus is **0**; the example’s `output=0` argument works unchanged. Nonzero buses are unsupported.

The current host performance allocation is 251,952 bytes, including two voices, shared scratch and a shared 128 KiB delay arena; the selected graph adds 13,620 bytes. The other seven slots remain in storage. The device build places these buffers in the dedicated pool region. The current link uses 326,008 of 344,064 pool bytes; two-voice hardware timing acceptance is recorded separately. Reducing the slot count would free flash, not this working RAM.

## Simulator

```sh
python3 tools/test-panel.py
```

This builds `firmware/` using the pinned fm1-base with Bluetooth disabled, creates an isolated test bank and exercises the actual app contract. It does not change the editor bank. Screens and audio are written into `build/panel/`.

The panel adapter draws a white screen with black Terminus 12 text, blue UGen/class names, red `arg`/`var`, purple numbers, green strings/symbols and grey comments. Terminus is a redistributable bitmap monospace face; Apple's Monaco is used only as a locally installed browser font. Non-ASCII source is preserved in storage, but unsupported display glyphs currently show `?` on the device-sized bitmap renderer.

The device adapter uses a bounded queue for panel events. Its audio callback owns voice state; slot loading and commits suspend rendering before touching that state. Overflow triggers panic rather than dropping note-offs silently. Device flash callbacks stay inside the slot region and validate graphs before committing. The hardware buffer is 256 frames, calculated in four 64-sample engine blocks. Device maths are freestanding, single precision, and checked against 1,248 SuperCollider reference cases with a separate target waveform tolerance (peak < 0.001, RMS < 0.0001), explicit impulse timing checks and 28 labeled Gendy stochastic-summary cases. The additive patch is substantially closer than those limits.

## Protocol

`src/protocol.c` implements staged, bounded transfers over decoded frames. `web/transport.js` implements seven-bit MIDI framing and request retries. Full details are in [PROTOCOL.md](PROTOCOL.md). The Mac manager uses `tools/device.py` and the workspace’s USB MIDI-enabled Python environment to transfer slots. The browser-side Web MIDI codec is retained for a future direct-browser transport. Both use the same framed protocol. `device.py stats` returns slot/load state, CPU, overruns, numerical faults, overloads and active voices.

## Verification

- Development engine: 1,248 SuperCollider reference cases on three backends; 60 API test groups; 800,000 sanitizer mutations/truncations. Scale/Tuning catalogs and direct operator oracles are also covered; see [current evidence](../docs/VERIFICATION.md).
- Journal: simulated power failure at all 68 erase/program operations of a full slot replacement; revision conflict, delete, read failure and region isolation.
- Performance: pitch/velocity conversion, held notes across octave changes, note identity, stealing, no-gate release, panic, active/new voice knob updates, stereo output and clipping.
- Browser/core: seven package/bank/audio test groups, lexical highlighting and MIDI framing tests. Browser compile → save → listen was exercised with the Keys + comb patch.
- Actual fm1-base host adapter: 23 existing panel/audio assertions plus 31 extension assertions, saved screen images and a stereo recording.

New code is GPL-3.0-or-later, as is the accompanying engine. Refer to `../synthdef-engine/LICENSE`. The pinned fm1-base retains its own MPL-2.0 licensing and font notices.

## Device build and diagnostics

```sh
bash tools/build-device.sh
../.venv/bin/python tools/device.py info
../.venv/bin/python tools/device.py stats
../.venv/bin/python tools/device.py write --slot 2 --file examples/addy-5.fmslot
```

The device and Mac preview allow **two simultaneous voices**. A third note steals the oldest released voice first, or the oldest held voice if both are held. Device startup reserves two 64-frame blocks (128 samples, about 2.9 ms) for preparation before the new voice starts; this avoids adding the full first-render cost to a chord’s startup callback. Preparation does not advance the waveform. Knobs 1–4 move by **0.02 per detent** (50 detents across the full range). PRESET selects the eight persistent synth slots, ALGORITHM selects FX1–FX4 or bypass, FX toggles the knob bank, SELECT scrolls the focused code, OCT± shifts subsequent keys, and HOME stops notes. Bluetooth stays disabled. The current example setup uses Keys in slot 1, Addy in slot 2, triple saw in slot 3, two-op FM in slot 4 and master echo in slot 0. The earlier Keys + comb example remains available separately.

`tools/device.py benchmark` is an explicit diagnostic: it temporarily silences playback, measures note startup and one/two-voice rendering, then reloads the selected slot. It is not a background operation. The hardware audio test also temporarily sets MASTER low and restores the physical pot afterward.

On 28 October 2026 the installed build passed recorded two-note playback, third-note stealing, stolen-note release, 24 Keys chords with all four live knobs, and 18 Addy chords through attack and decay. No faults, overloads or overruns were recorded. Peak measured app render time was 4,476 microseconds; peak startup was 4,033 microseconds. The USB recording contains nonzero stereo audio. Physical listening and power-cycle confirmation for this particular build remain user checks. This does not certify arbitrary admitted graphs against the CPU budget.

## Four master-effect presets

Slots 1–8 keep their existing storage and two-voice playback. The master bus (previously called slot 0) now loads one of **FX1–FX4**, or bypass. PRESET changes only the playable synth. ALGORITHM cycles bypass, FX1, FX2, FX3, FX4. The **FX button is a latching toggle**, including its LED: off shows synth code and routes physical knobs to `knob1`–`knob4`; on shows effect code and routes them to `knob5`–`knob8`. SELECT scrolls whichever code is visible. The header identifies both the synth and effect.

Toggling FX never changes either graph, interrupts held notes, or resets knob values. Synth changes release the old synth but preserve the effect tail and its control values. Effect changes fade the output down over 256 samples, replace just the effect, then fade back up over 256 samples. Held synth notes continue and note-offs remain queued during the swap. The old effect tail ends on replacement; there is no overlapping pair of expensive reverbs. A newly selected effect uses its declared control defaults. Boot selects synth 1, FX1 and synth control mode; these selections are not saved by encoder movement.

All synth voices mix into stereo bus 0. `In.ar(0, 2)` reads that immutable mix, and `Out.ar(0, processed)` supplies stereo output before MASTER, clipping and meters. Effects continue processing silence after notes end. HOME clears notes and the effect tail. Effects require doneAction 0; numerical faults bypass the failed effect, while the overload handler stops notes and disables the effect until reset/reloaded.

The four effect packages **share 7,916 bytes** for metadata, source and compiled graphs. A 20-byte directory plus those packages occupies the existing two 8 KiB journal banks, so no synth slots or protected flash regions are removed. Saving or clearing one effect atomically commits the whole effect bank, preserving its neighbors. All four effects therefore share a revision number. The editor displays the shared storage usage and rejects overflow before writing.

The editor provides eight synth slots plus FX1–FX4. Its FX button switches between the last edited synth and effect, preserving their slider values; its ALGORITHM buttons browse effect presets, with a separate preview-bypass checkbox. Listen renders the last chosen synth through the last chosen effect. Local and device banks remain separate; Send verifies full device readback.

To upgrade an existing nine-slot bank, install the new firmware first. It reads the old effect as FX1 without altering it. Then run from the project root:

```sh
.venv/bin/python fm1-synth/tools/migrate-fx-bank.py --device --backup /tmp/fm1-old-effects
.venv/bin/python fm1-synth/tools/migrate-fx-bank.py --backup /tmp/fm1-old-local-effects
```

The helper preserves all synths, backs up the original effects, maps legacy effect knob arguments consecutively to `knob5` onward, and fills only empty FX2–FX4 with the supplied reverbs. Mixed old/new control-name collisions stop migration for manual review. Old large effects may need shortening to fit the shared budget. Do not use an older editor/firmware to write the new FX bank: downgrade requires restoring the backed-up single effect with the old tools. `device.py --slot 0` remains an alias for FX1; use `--effect 1` through `--effect 4` for explicit access.

The examples use `keys-master.scd` (knobs 1–2: cutoff/resonance), `master-echo.scd` in FX1 (knob 5: echo amount; knob 6: echo time, 40–300 ms, fixed 1.2-second decay), and FreeVerb, FreeVerb2 and MiVerb in FX2–FX4 (knob 5: wet/dry; knob 6: room/decay). With FX on, these are the first and second physical knobs. Custom effects can also use knobs 7–8. Synth knobs 3–4 are independent of the effects.

Slot 3 now contains `examples/triple-saw.scd`: three detuned `LFSaw` oscillators, gentle 5 Hz vibrato, and an RLPF driven by the same ADSR as amplitude. Knob 1 jointly sets attack (5 ms–1.505 s) and release (60 ms–3.06 s), with a quadratic response; knob 2 sets outer-saw detune (0–1.8% either side); knob 3 sets vibrato depth (0–0.6%); knob 4 sets the envelope's maximum cutoff (780–10,780 Hz, from a 180 Hz floor). Synth controls are independent of the master echo. Envelope segment times are read when a segment starts; turning knob 1 during an existing attack does not restart it.

This patch uses unbandlimited `LFSaw` to fit two voices plus echo within the device budget; high notes with an open filter can alias. The initial band-limited `Saw` version overloaded with two voices and is retained only in the diagnostic log directory. The installed version passed 17 hardware checks on 28 October 2026, including live knob extremes, low/high chords and short/long releases, with nonzero stereo USB audio and no additional overloads, overruns or numerical faults. Sampled render time peaked at 3,301 microseconds. Evidence is in `../evidence/triple-saw.json`; reproduce with `tools/test-triple-saw.py --output <directory>` using the workspace Python environment.

Slot 4 contains `examples/two-op-fm.scd`: a sine modulator frequency-modulates a sine carrier. Knob 1 continuously sweeps the modulator/carrier ratio from 0.5 to 8 (default 2); knob 2 sets the peak FM index from 0 to 5 with a quadratic response (default 0.6125). Both controls are smoothed over 30 ms and update sounding notes. The ADSR (10 ms attack, 300 ms decay, 0.6 sustain, 450 ms release) shapes amplitude and FM depth, so the sound softens as it decays. Knobs 3–4 remain available for additional synth controls. High ratios/depths on high notes can alias. The 18-node patch uses no delay RAM and passed host previews at default/minimum/maximum controls plus 13 device checks, including two voices through echo, live knob extremes, low/high chords, release completion and nonzero stereo USB audio. No additional overloads, overruns or numerical faults occurred; sampled render time peaked at 1,910 microseconds. Evidence is in `../evidence/two-op-fm.json`.

Clearing the currently selected FX preset restores dry playback; the other saved effects remain available.

Admission checks require `2 * synth_delay_samples + effect_delay_samples <= 32768`. Saving an effect checks every stored synth, and saving a synth checks all four effects, so switching presets cannot overwrite effect delay memory. The supplied mono echo uses 64 KiB and preserves the dry stereo channels. The new Keys patch uses no per-voice delay; the old Keys + comb example remains available separately.

## Development UGen expansion

The source now supports 77 types. Dust, Dust2, Decay, Decay2, Impulse, Latch/Gate, TRand/TExpRand and simple filters are available at audio and control rate, with the engine profile's input restrictions. The five new pitch/gain conversions include `.midiratio` and `.dbamp`.

`examples/dust-chimes.scd` is a 19-node example using Dust → TRand → SinOsc with Decay2 excitation. Knob 1 sets 2–40 events/second; knob 2 sets a 30–730 ms tail. Its outer gated envelope releases the note normally. Paste it into a spare local slot for host preview. It requires a development firmware build on the device; it has not been sent to the connected FM-1.

Run `../.venv/bin/python tools/test-extensions-panel.py` for a temporary two-patch bank exercising the real panel adapter, two voices, stealing, live controls, release and permanent master echo. The test preserves user banks. Host simulation does not measure FM-1 realtime CPU load. See [the development verification record](../docs/VERIFICATION.md).

## Development slot-0 reverbs

`examples/master-freeverb.fmslot`, `master-freeverb2.fmslot` and `master-miverb.fmslot` are alternatives for slot 0. Knob 5 sets wet/dry (0–60% wet); knob 6 sets room/decay. The mono FreeVerb example preserves the dry stereo image and adds a shared mono wet signal. MiVerb also exposes a `freeze` control, default 0; its example stays below self-sustaining feedback.

All three passed temporary-bank simulations with Keys and Dust chimes, two voices, stealing, live knobs, release tails and FX browsing (114 assertions), plus the earlier 54 panel assertions. Run `tools/test-reverbs-panel.py` to reproduce. The four-effect firmware builds at 227,828 bytes, with 78,480 bytes static RAM and 326,008 bytes pool use. It is installed on the test FM-1. Two Addy voices through FreeVerb exceeded the processing budget; see the current device results and limits in `../docs/VERIFICATION.md`. These packages require the development firmware; v0.1.0-preview cannot load them.

Compilation uses the stock SuperCollider class library plus our pinned MiVerb class. User extension/quark classes are not loaded into this compiler. The ordinary SuperCollider application configuration is unchanged.

## Battery display

The top-right corner shows a three-bar battery icon in both synth and FX code views. A small plug indicates an active USB host connection; it does not claim that the battery is charging. An unknown reading shows `?`. The indicator uses the stock ADC thresholds (531, 561 and 591), a slow average and hysteresis rather than an uncalibrated percentage.

Battery sampling runs once a second and claims only an idle gap in the key scanner's ADC schedule. It leaves audio interrupts enabled and restores the scan configuration immediately afterwards. The pinned platform is unchanged. `tools/device.py battery` reads the measured ADC value, bars, USB state and ADC timeout count. `tools/test-battery-panel.py` checks six simulated states and saves synth/FX screenshots; `tests/battery.c` checks ADC ownership, polling and threshold behaviour.

New keyboard notes require both the velocity sensor and the debounced contact matrix. Once a note starts, its existing analog release behaviour is retained. This prevents an isolated sensor event from starting a note. `tools/device.py inputs` exposes separate keyboard/MIDI note counters and rejected unconfirmed presses; `tools/test-input-device.py --output <directory>` compares playback with battery sampling off and on, then restores normal sampling and panel control. Leave the keys untouched during that automated test.

## Rings models

The development engine includes MiRings1–MiRings6 with independent model memory. See [Rings API, memory and compact mode](../docs/RINGS.md). The six `examples/rings-*.scd` synths and `examples/rings-bus.scd` have compiled graphs and slot packages. Compact models 2/5 permit two voices plus the existing master echo; model 6 needs a lighter/bypassed master effect when used as a two-voice synth. These additions are emulation-tested; hardware checks are deferred.
