# Compatibility and limits

The on-device program executes a compiled SynthDef graph. Source-language constructs such as `Array.fill`, `.sum` and Env constructors run in SuperCollider on the Mac during compilation; their resulting graph must fit this profile. The device does not interpret arbitrary SuperCollider source.

The authoritative type/operator list is [profile.json](../synthdef-engine/profile.json), with per-UGen rates and parameter rules in the [engine README](../synthdef-engine/README.md). The development source supports 118 compiled types, including Formant, Formlet, Gendy1/2/3, the three reverbs, MiMu and bounded Scale/Tuning pitch tables. The published v0.1.0-preview firmware has the earlier 53 types and rejects the new nodes. Host compilation success does not update the connected device. New firmware has passed emulation and build checks, but still needs hardware playback/timing verification.

All nine EnvGen curve types are supported, with sustain, loop, retrigger and forced-release behaviour. Env.perc, adsr, dadsr, asr, cutoff, linen, triangle, sine and custom breakpoints are covered. Done actions are 0 and 2; effects require 0. This is not a server node tree.

| Resource | Limit |
| --- | --- |
| Playable slots / stored master-effect presets | 8 / 4 (one effect active) |
| Simultaneous voices | 2 |
| Synth slot package | 16,128 bytes including source and metadata |
| Effect packages | 7,916 bytes shared across four presets, including source and metadata |
| Nodes / output signals / input references | 160 / 192 / 1,024 per graph |
| Constants / controls | 512 / 64 per graph |
| Immutable pitch tables | At most 16 mono tables, 1–128 literal entries each, within graph limits |
| Envelope segments / bank partials | 32 per envelope / 64 total partials per graph |
| Sample rate / engine block | 44.1 kHz / 64 samples |
| Device callback | 256 samples |
| Delay arena | 128 KiB shared across both voices and the effect |
| Delay memory per synth voice | At most 64 KiB; `2 * synth + effect` must fit the shared arena |

Each delay line rounds up to a power of two with overhead. Reducing the number of flash slots does not increase working audio RAM. Graph validation checks memory, syntax and supported operations; it does not estimate realtime CPU cost.

`Out.ar(0, ...)` is the stereo output. A dedicated `In.ar(0, 1 or 2)` is available only in slot 0, reading the immutable instrument mix. Arbitrary internal buses, external audio processing through this In, buffer playback, general feedback graphs, demand-rate processing and plugins are not implemented. Unknown UGens, rates and operators are rejected.

Note arguments are filled when present: `freq` in Hz; `pitch` and `midinote` in MIDI units; `note` = MIDI minus 60; `amp` and `velocity` = velocity/127; `gate` = 1 then 0. This is a small explicit mapping, not the entire SuperCollider Event model.

`knob1`–`knob4` in synths and `knob5`–`knob8` in effects must be control-rate parameters. The FX button latches the physical knobs between these independent banks, including updates during held notes. Values are 0–1 in steps of 0.02. Initial random values remain fixed for each voice. Envelope segment parameters are read when that segment begins. Released gated voices have a ten-second lifetime cap.

PRESET selects synths; ALGORITHM selects FX1–FX4 or bypass. FX toggles code view and knob bank without changing held notes. Effect replacement fades output briefly and ends the old tail. HOME stops voices and clears the effect tail. A numerical fault retires a voice; an overload stops notes and disables the effect until reset/reloaded.

Reverbs allocate from the same 128 KiB delay arena: FreeVerb 50,492 bytes, FreeVerb2 102,088 bytes, MiVerb 65,536 bytes per instance. Parameters are initial/control rate and signal inputs are audio rate. Prefer slot 0; FreeVerb2 exceeds the per-synth-voice limit. MiVerb supports mono/stereo input, freeze, diffusion and its upstream output soft clipping. The bundled MiVerb class supports compilation without installing extra plugins.

Scale and Tuning are evaluated by the Mac compiler. Live `DegreeToKey.ar/kr` uses immutable LocalBuf tables; it does not add general sample buffers or automatic keyboard scale mapping. See the [Scale/Tuning profile](../synthdef-engine/README.md#scale-and-tuning) and complete supported operator list, including the documented bounded integer domain and SC 3.14.1 edge differences.
