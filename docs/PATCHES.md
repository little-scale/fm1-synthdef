# Included patches

Source, compiled `.scsyndef` and importable `.fmslot` files are in `fm1-synth/examples/`. The package name identifies the patch; the slot number is chosen when importing/sending it.

| Slot | Package | Controls |
| --- | --- | --- |
| FX1 | `master-echo.fmslot` | Knob 5: echo amount; knob 6: delay 40–300 ms, fixed 1.2 s decay |
| 1 | `keys-master.fmslot` | Knob 1: low-pass cutoff; knob 2: resonance |
| 2 | `addy-5.fmslot` | Five detuned sine harmonics with random amplitudes, stereo positions and individual decays |
| 3 | `triple-saw.fmslot` | Knob 1: attack 5 ms–1.505 s and release 60 ms–3.06 s; knob 2: outer detune 0–1.8%; knob 3: depth of 5 Hz vibrato; knob 4: envelope cutoff ceiling |
| 4 | `two-op-fm.fmslot` | Knob 1: continuous modulator/carrier ratio 0.5–8, default 2; knob 2: peak FM index 0–5 |

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
