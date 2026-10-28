# FM-1 SynthDef instrument infrastructure

This is a Mac editor, a portable instrument/storage/protocol core, and an fm1-base device and host adapter. Two-voice playback of Keys and Addy passed the recorded and sustained device tests; see [the verification record](../evidence/master-effect.json). Other patches still need their own timing checks. The complete [checklist](PLAN.md) distinguishes tested host features from remaining device work.

## Run

```sh
cd fm1-synth
make test
make serve
```

Open the launch URL printed by the server. It binds only to `127.0.0.1` and uses a session token. The editor saves to `data/local-bank.flash`, a simulated 1 MiB chip image. The local bank is separate from hardware. **Check FM-1** reads the nine device slot names; **Read FM-1 slot** imports the selected device slot as an editor draft; **Send to FM-1 slot N** explicitly writes that device slot and verifies it by readback. Device transfers require the SynthDef capability handshake and a matching slot revision. Stop the server with Ctrl-C; the local bank survives restart. Do not run two manager processes against the same bank file.

The editor supports paste, `.scd`, `.scsyndef`, and exported `.fmslot` packages. Compile evaluates one SynthDef expression using the installed SuperCollider, validates the graph, checks the slot/delay budget and enables Save. Editing code invalidates its compiled state. A binary-only import explicitly has no source. Export packages include the name, source, compiled graph, SHA-256 hashes and a whole-body CRC. No source is reconstructed or falsely labelled as the original.

Listen renders a two-second note with note-off after one second through the native controller. It is an offline preview: slider changes apply to the next render. The browser shows both a code editor and a 240×240 screen preview with fixed header/meters, wrapped code and scrolling.

## Keyboard and knobs

Each press starts a voice with `freq`, `midinote`, `pitch` (MIDI alias), `note` (MIDI minus 60), `amp` and `velocity` (both velocity/127), and `gate=1`, where those arguments exist. Note-off sets `gate=0`; voices without a writable gate get a short release fade. OCT± affects only subsequent panel notes, ±2 octaves. Incoming MIDI is not transposed. Note identity includes MIDI port/channel, or the physical key, so an octave change cannot strand a note.

Use **control-rate arguments** `knob1` through `knob4`, or `NamedControl.kr` with those names. Ordinary local `var` declarations do not expose controls. Every detent changes the value by 0.02, clamped to 0–1. Defaults come from the SynthDef. Updates are passed with `fm_set` to all sounding voices between blocks; new voices receive the current values. Matching knob arguments in slot 0 receive the same values. For smoothing:

```supercollider
arg knob1=0.5;
var cutoff = 200 + (Lag.kr(knob1, 0.02) * 6000);
```

Initial-rate or trigger controls named `knob1`–`knob4` are rejected by instrument validation because they cannot provide this persistent live behaviour. The engine itself still supports these control types under other names. The [Keys + comb example](examples/keys.scd) uses all four knobs.

Two voices are allocated, with oldest released voice stolen first and a 128-sample transition. Audio clipping is bounded; meter values are measured before the final clamp and include a one-second clip indicator. This is not an automatic loudness normalizer or look-ahead limiter. A released gate voice has a ten-second lifetime cap.

## Eight slots and delay RAM

Each slot uses two 16 KiB banks, totaling 256 KiB for all eight slots. Its **16,128-byte payload** includes metadata, source and compiled graph. CRC-checked headers are written last, and saves compare the expected revision before replacing anything. Calibration, update staging and platform settings regions are excluded by the address map. Turning knobs never writes flash.

Flash space and delay RAM are separate. DelayN/L/C, CombN/L/C and AllpassN/L/C use caller-owned buffers. The instrument has a **32,768-float / 128 KiB shared delay arena** for both synth voices plus slot 0. A synth voice is limited to 16,384 samples / 64 KiB, and its actual required size is allocated for each of the two voices. The effect uses the high end of the same arena. Ring lengths round up to powers of two with processing overhead; a single `maxdelaytime: 0.2` at 44.1 kHz uses the full 64 KiB. The engine's general host API allows a larger caller budget. The manager rejects patches exceeding the instrument budget before save.

The host profile accepts **160 nodes and 192 signals**. [Addy, five harmonics](examples/addy-5.scd) uses 124 nodes and 135 signals; its source, binary and metadata occupy 7,062 bytes. `Rand` and `ExpRand` are supported and sample their bounds once per note. The compiled binary, importable `.fmslot` and WAV preview are next to the example. Default output bus is **0**; the example’s `output=0` argument works unchanged. Nonzero buses are unsupported.

The current host performance allocation is 251,936 bytes, including two voices, shared scratch and a shared 128 KiB delay arena; the selected graph adds 13,620 bytes. The other seven slots remain in storage. The device build places these buffers in the dedicated pool region. The current link uses 325,992 of 344,064 pool bytes; two-voice hardware timing acceptance is recorded separately. Reducing the slot count would free flash, not this working RAM.

## Simulator

```sh
python3 tools/test-panel.py
```

This builds `firmware/` using the pinned fm1-base with Bluetooth disabled, creates an isolated test bank and exercises the actual app contract. It does not change the editor bank. Screens and audio are written into `build/panel/`.

The panel adapter draws a white screen with black Terminus 12 text, blue UGen/class names, red `arg`/`var`, purple numbers, green strings/symbols and grey comments. Terminus is a redistributable bitmap monospace face; Apple's Monaco is used only as a locally installed browser font. Non-ASCII source is preserved in storage, but unsupported display glyphs currently show `?` on the device-sized bitmap renderer.

The device adapter uses a bounded queue for panel events. Its audio callback owns voice state; slot loading and commits suspend rendering before touching that state. Overflow triggers panic rather than dropping note-offs silently. Device flash callbacks stay inside the slot region and validate graphs before committing. The hardware buffer is 256 frames, calculated in four 64-sample engine blocks. Device maths are freestanding, single precision, and compared against all 553 SuperCollider fixtures with a separate target tolerance (peak < 0.001, RMS < 0.0001). The additive patch is substantially closer than those limits.

## Protocol

`src/protocol.c` implements staged, bounded transfers over decoded frames. `web/transport.js` implements seven-bit MIDI framing and request retries. Full details are in [PROTOCOL.md](PROTOCOL.md). The Mac manager uses `tools/device.py` and the workspace’s USB MIDI-enabled Python environment to transfer slots. The browser-side Web MIDI codec is retained for a future direct-browser transport. Both use the same framed protocol. `device.py stats` returns slot/load state, CPU, overruns, numerical faults, overloads and active voices.

## Verification

- Development engine: 553 SuperCollider comparisons, including 99 delay-family graphs; 34 API test groups; 440,000 sanitizer mutations/truncations.
- Journal: simulated power failure at all 68 erase/program operations of a full slot replacement; revision conflict, delete, read failure and region isolation.
- Performance: pitch/velocity conversion, held notes across octave changes, note identity, stealing, no-gate release, panic, active/new voice knob updates, stereo output and clipping.
- Browser/core: six package/bank/audio test groups, lexical highlighting and MIDI framing tests. Browser compile → save → listen was exercised with the Keys + comb patch.
- Actual fm1-base host adapter: 23 existing panel/audio assertions plus 31 extension assertions, saved screen images and a stereo recording.

New code is GPL-3.0-or-later, as is the accompanying engine. Refer to `../synthdef-engine/LICENSE`. The pinned fm1-base retains its own MPL-2.0 licensing and font notices.

## Device build and diagnostics

```sh
bash tools/build-device.sh
../.venv/bin/python tools/device.py info
../.venv/bin/python tools/device.py stats
../.venv/bin/python tools/device.py write --slot 2 --file examples/addy-5.fmslot
```

The device and Mac preview allow **two simultaneous voices**. A third note steals the oldest released voice first, or the oldest held voice if both are held. Device startup reserves two 64-frame blocks (128 samples, about 2.9 ms) for preparation before the new voice starts; this avoids adding the full first-render cost to a chord’s startup callback. Preparation does not advance the waveform. Knobs 1–4 move by **0.02 per detent** (50 detents across the full range). PRESET selects the eight persistent slots, SELECT scrolls code, OCT± shifts subsequent keys, and HOME stops notes. Bluetooth stays disabled. The current example setup uses Keys in slot 1, Addy in slot 2, triple saw in slot 3, two-op FM in slot 4 and master echo in slot 0. The earlier Keys + comb example remains available separately.

`tools/device.py benchmark` is an explicit diagnostic: it temporarily silences playback, measures note startup and one/two-voice rendering, then reloads the selected slot. It is not a background operation. The hardware audio test also temporarily sets MASTER low and restores the physical pot afterward.

On 28 October 2026 the installed build passed recorded two-note playback, third-note stealing, stolen-note release, 24 Keys chords with all four live knobs, and 18 Addy chords through attack and decay. No faults, overloads or overruns were recorded. Peak measured app render time was 4,476 microseconds; peak startup was 4,033 microseconds. The USB recording contains nonzero stereo audio. Physical listening and power-cycle confirmation for this particular build remain user checks. This does not certify arbitrary admitted graphs against the CPU budget.

## Permanent master effect: slot 0

Slots 1–8 retain their storage and two-voice playback. Slot 0 is an additional permanent effect instance, with a 7,936-byte source/binary package in two 8 KiB journal banks. Empty slot 0 bypasses the effect. All synth voices mix into stereo bus 0; `In.ar(0, 2)` reads that immutable mix, and `Out.ar(0, processed)` supplies the final stereo signal before MASTER gain, clipping and metering. The effect processes silence after notes end, preserving its tail. It survives synth preset changes. HOME clears both notes and the effect tail. Effect definitions require doneAction 0; a numerical fault bypasses the failed effect, while the platform overload handler stops notes and disables the effect until reloaded.

PRESET can browse **00 FX** without selecting a different synth. The header’s **PLAY nn** line identifies the synth still played by the keyboard. SELECT scrolls the visible effect code. Browsing back to that same synth also preserves held notes; choosing a different synth retains the normal preset-selection behavior.

The current examples are `examples/keys-master.scd` (knobs 1–2: cutoff/resonance) and `examples/master-echo.scd` (knob 3: echo amount; knob 4: echo time, 40–300 ms; fixed 1.2-second decay). Knob routing is determined by argument names, not slot-specific assignments. A synth's declared knob default takes precedence when it loads; knobs used only by the effect retain their current values across synth changes, starting from effect defaults on an effect reload.

Slot 3 now contains `examples/triple-saw.scd`: three detuned `LFSaw` oscillators, gentle 5 Hz vibrato, and an RLPF driven by the same ADSR as amplitude. Knob 1 jointly sets attack (5 ms–1.505 s) and release (60 ms–3.06 s), with a quadratic response; knob 2 sets outer-saw detune (0–1.8% either side); knob 3 sets vibrato depth (0–0.6%); knob 4 sets the envelope's maximum cutoff (780–10,780 Hz, from a 180 Hz floor). Knobs 3–4 also continue to affect the master echo. Envelope segment times are read when a segment starts; turning knob 1 during an existing attack does not restart it.

This patch uses unbandlimited `LFSaw` to fit two voices plus echo within the device budget; high notes with an open filter can alias. The initial band-limited `Saw` version overloaded with two voices and is retained only in the diagnostic log directory. The installed version passed 17 hardware checks on 28 October 2026, including live knob extremes, low/high chords and short/long releases, with nonzero stereo USB audio and no additional overloads, overruns or numerical faults. Sampled render time peaked at 3,301 microseconds. Evidence is in `../evidence/triple-saw.json`; reproduce with `tools/test-triple-saw.py --output <directory>` using the workspace Python environment.

Slot 4 contains `examples/two-op-fm.scd`: a sine modulator frequency-modulates a sine carrier. Knob 1 continuously sweeps the modulator/carrier ratio from 0.5 to 8 (default 2); knob 2 sets the peak FM index from 0 to 5 with a quadratic response (default 0.6125). Both controls are smoothed over 30 ms and update sounding notes. The ADSR (10 ms attack, 300 ms decay, 0.6 sustain, 450 ms release) shapes amplitude and FM depth, so the sound softens as it decays. Knobs 3–4 remain available to the echo. High ratios/depths on high notes can alias. The 18-node patch uses no delay RAM and passed host previews at default/minimum/maximum controls plus 13 device checks, including two voices through echo, live knob extremes, low/high chords, release completion and nonzero stereo USB audio. No additional overloads, overruns or numerical faults occurred; sampled render time peaked at 1,910 microseconds. Evidence is in `../evidence/two-op-fm.json`.

The browser shows slot 0 first and supports paste, compile, import/export, local save, read/send, and device clear. Preview processes the selected synth through local slot 0; previewing slot 0 uses local synth slot 1 as its source. Local and device banks remain separate. Device saves compare revisions and verify full readback. Clearing device slot 0 restores dry playback.

Admission checks require `2 * synth_delay_samples + effect_delay_samples <= 32768`. Saving an effect checks every stored synth, so switching presets cannot overwrite effect delay memory. The supplied mono echo uses 64 KiB and preserves the dry stereo channels. The new Keys patch uses no per-voice delay; the old Keys + comb example remains available separately.

## Development UGen expansion

The source now supports 77 types. Dust, Dust2, Decay, Decay2, Impulse, Latch/Gate, TRand/TExpRand and simple filters are available at audio and control rate, with the engine profile's input restrictions. The five new pitch/gain conversions include `.midiratio` and `.dbamp`.

`examples/dust-chimes.scd` is a 19-node example using Dust → TRand → SinOsc with Decay2 excitation. Knob 1 sets 2–40 events/second; knob 2 sets a 30–730 ms tail. Its outer gated envelope releases the note normally. Paste it into a spare local slot for host preview. It requires a development firmware build on the device; it has not been sent to the connected FM-1.

Run `../.venv/bin/python tools/test-extensions-panel.py` for a temporary two-patch bank exercising the real panel adapter, two voices, stealing, live controls, release and permanent master echo. The test preserves user banks. Host simulation does not measure FM-1 realtime CPU load. See [the development verification record](../docs/VERIFICATION.md).

## Development slot-0 reverbs

`examples/master-freeverb.fmslot`, `master-freeverb2.fmslot` and `master-miverb.fmslot` are alternatives for slot 0. Knob 3 sets wet/dry (0–60% wet); knob 4 sets room/decay. The mono FreeVerb example preserves the dry stereo image and adds a shared mono wet signal. MiVerb also exposes a `freeze` control, default 0; its example stays below self-sustaining feedback.

All three passed temporary-bank simulations with Keys and Dust chimes, two voices, stealing, live knobs, release tails and FX browsing (114 assertions), plus the earlier 54 panel assertions. Run `tools/test-reverbs-panel.py` to reproduce. The new firmware builds at 226,972 bytes with unchanged static RAM/pool allocation. Device timing and listening checks remain pending because the FM-1 was not connected. These packages require the development firmware; v0.1.0-preview cannot load them.

Compilation uses the stock SuperCollider class library plus our pinned MiVerb class. User extension/quark classes are not loaded into this compiler. The ordinary SuperCollider application configuration is unchanged.
