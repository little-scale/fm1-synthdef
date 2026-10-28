# 0.1.0-preview — 28 October 2026

First public preview of FM1 SynthDef, an embedded SuperCollider SynthDef runtime for the M-VAVE FM-1.

Includes two-voice playback, eight synth slots, one permanent master effect, live knobs, octave controls, syntax-coloured on-device source, stereo meters, a Mac browser editor, USB slot management and five example patches. The compatibility profile implements 53 UGen types with explicit operator/rate limits.

The complete archive includes the accepted `FM-1_1200` firmware, source, pinned platform/BSP snapshots, minimal SDK packaging data, licences, examples and a checksum manifest. The firmware is the same accepted device build; release preparation does not reflash a connected instrument or prepopulate its slots.

This is experimental: arbitrary graphs may exceed realtime capacity despite passing import validation. Only the documented Mac setup and supplied patches have hardware acceptance evidence. Final-build power-cycle persistence and recovery remain unverified. See QUICKSTART, COMPATIBILITY and VERIFICATION before use.
