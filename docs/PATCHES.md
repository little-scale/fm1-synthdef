# Included patches

Source, compiled `.scsyndef` and importable `.fmslot` files are in `fm1-synth/examples/`. The package name identifies the patch; the slot number is chosen when importing/sending it.

| Slot | Package | Controls |
| --- | --- | --- |
| FX1 | `master-echo.fmslot` | Knob 5: echo amount; knob 6: delay 40–300 ms, fixed 1.2 s decay |
| 1 | `keys-master.fmslot` | Knob 1: low-pass cutoff; knob 2: resonance |
| 2 | `addy-5.fmslot` | Five detuned sine harmonics with random amplitudes, stereo positions and individual decays |
| 3 | `triple-saw.fmslot` | Knob 1: attack 5 ms–1.505 s and release 60 ms–3.06 s; knob 2: outer detune 0–1.8%; knob 3: depth of 5 Hz vibrato; knob 4: envelope cutoff ceiling |
| 4 | `two-op-fm.fmslot` | Knob 1: continuous modulator/carrier ratio 0.5–8, default 2; knob 2: peak FM index 0–5 |
| 5 | `dusty.fmslot` | Five independent Dust/Decay2 channels, squared sine carrier, Splay stereo spread; no knob mapping yet |

The master echo runs after the summed voices, and continues decaying after note-off and synth preset changes. Press FX to toggle the physical knobs between synth arguments 1–4 and effect arguments 5–8. These values are independent. PRESET changes synths; ALGORITHM changes effects. Two-op FM references only synth knobs 1–2.

Triple saw uses lighter, unbandlimited `LFSaw` oscillators because three band-limited saws per voice exceeded the device budget with two voices and echo. High saw notes with an open filter can alias. High FM ratios/depths on high notes can also alias.

Two-op FM uses a 10 ms attack, 300 ms decay, 0.6 sustain and 450 ms release; its envelope shapes both amplitude and FM depth. At knob 2 = 0 it is a pure sine. Both synth knobs update held notes with 30 ms smoothing.

The older `keys.fmslot` contains its own comb effect and uses all four knobs. It is retained as a separate example; use `keys-master.fmslot` for the recommended master-effect setup.

## Development reverb alternatives

These packages require the new development firmware. They are not compatible with v0.1.0-preview. The migration helper fills empty FX2–FX4 with these packages; ALGORITHM selects one active effect at a time:

| Package | Knob 5 (FX physical knob 1) | Knob 6 (FX physical knob 2) |
| --- | --- | --- |
| `master-freeverb.fmslot` | Wet/dry, up to 60% wet | Room 0.2–0.95 |
| `master-freeverb2.fmslot` | Wet/dry, up to 60% wet | Room 0.2–0.95 |
| `master-miverb.fmslot` | Wet/dry, up to 60% wet | Decay parameter 0.45–0.95 |

Source and compiled forms are beside the packages in `fm1-synth/examples/`. FreeVerb's wet signal is mono while its example preserves dry stereo; FreeVerb2 and MiVerb provide stereo reverb. MiVerb exposes `freeze=0` for custom mappings. FX controls are independent of synth knobs. All three passed hardware playback and tail checks with two voices of Keys, triple saw, two-op FM and Dust chimes. Two Addy voices through FreeVerb overload the device; see [verification](VERIFICATION.md).

## Dusty and multichannel helpers

`dusty.scd` retains the supplied five independent `Dust.ar(10 ! 5)` exciters, `Decay2` defaults, the squared-sine carrier, and `Splay.ar` output. The oscillator product is a squared sine, including a DC component and a doubled-frequency component; this was not changed. Pitch and velocity arrive through `freq` and `amp`. The declared `knob1`, `knob2` and `gate` are currently unused. Notes run through a 10 ms attack and three-second decay even if released early. `doneAction: 2` retires the voice when that envelope ends. `.add` is omitted because the slot compiler needs the SynthDef object rather than registering it with a desktop server.

The common carrier and envelope are calculated once after the stereo sum, reducing the original 42-node graph to 30 nodes without removing any dust streams. A two-second comparison differs by at most one step in 16-bit PCM. The patch uses no delay arena and a 1,924-byte package. Normal `Splay.ar` and `Mix.ar` expand at compile time into existing Pan2, arithmetic, and rate-conversion nodes; they do not increase the engine's 77 compiled UGen types. Mix sums channels without automatic normalization; Splay's default includes equal-power level compensation. Use constant `levelComp` values: a dynamically varying numeric exponent can generate unsupported `pow`. Existing graph-size and CPU limits still apply. `SplayAz` requires unsupported PanAz and is not included.

Run `fm1-synth/tools/test-mix-splay.py` from the repository root for 15 helper-equivalence checks, and `test-dusty-panel.py` for two-voice/effect integration. Hardware results are recorded separately in the verification log.
