# 0.1.0-preview — 28 October 2026

## Development source — battery indicator

Adds a three-bar battery icon and USB-host power symbol in the top-right of synth and effect code views. The indicator uses stock thresholds, smoothing and hysteresis, and samples only idle ADC slots. Six simulator states and the physical USB-connected sensor were checked. New keyboard notes also require contact-matrix confirmation while retaining velocity sensing, and input-source diagnostics help distinguish keyboard events from incoming MIDI. This is installed on the development FM-1; the published preview download is unchanged.

Reverb kernels are kept outside the main renderer, and MiVerb's packed delay storage uses alias-safe byte accesses instead of tiny external memory-copy calls. All 553 reference fixtures still pass on three backends; the eight identical-input MiVerb comparisons remain exact. Performance diagnostics now subtract the wrapping tick counter before converting to microseconds. See [verification](VERIFICATION.md) for hardware limits, including two Addy voices through FreeVerb.

First public preview of FM1 SynthDef, an embedded SuperCollider SynthDef runtime for the M-VAVE FM-1.

Includes two-voice playback, eight synth slots, one permanent master effect, live knobs, octave controls, syntax-coloured on-device source, stereo meters, a Mac browser editor, USB slot management and five example patches. The compatibility profile implements 53 UGen types with explicit operator/rate limits.

The complete archive includes the accepted `FM-1_1200` firmware, source, pinned platform/BSP snapshots, minimal SDK packaging data, licences, examples and a checksum manifest. The firmware is the same accepted device build; release preparation does not reflash a connected instrument or prepopulate its slots.

This is experimental: arbitrary graphs may exceed realtime capacity despite passing import validation. Only the documented Mac setup and supplied patches have hardware acceptance evidence. Final-build power-cycle persistence and recovery remain unverified. See QUICKSTART, COMPATIBILITY and VERIFICATION before use.
