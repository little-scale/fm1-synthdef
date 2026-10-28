# FM1 SynthDef engine

A bounded C99 runtime that reads SuperCollider version-2 `.scsyndef` files and executes their UGen graphs. It powers the Mac preview and the installed FM-1 instrument. The development profile implements 77 UGen types, with explicit limits on rates, operators, memory and graph size. It is an embedded SynthDef runtime, not the full SuperCollider language or server. See [the project overview](../README.md) for installation and [the instrument](../fm1-synth/README.md) for controls and hardware behaviour.

## Try it

From this directory:

```sh
make
make test
make reference-test
make sanitize
build/fm-inspect fixtures/fm1_voice.scsyndef
python3 tools/render-wav.py fixtures/fm1_voice.scsyndef build/fm1-voice.wav
```

`make test` needs C99 and C++17 compilers and Python 3, with no Python packages. `make reference-test` additionally uses the installed SuperCollider **3.14.1** offline server, without opening audio hardware. Override `SCSYNTH` to change its location. The recorded fixtures are sufficient to run these tests; regeneration uses `python3 tools/export-all.py` with the installed `sclang` 3.14.1. It runs all seven export scripts with timeouts and refreshes the fixture integrity manifest. The Makefile currently targets macOS shared libraries.

The example in `examples/subtractive.scd` combines Saw, Pulse, a modulated RLPF, envelopes and panning. `build/subtractive-voice.wav` is its host-rendered companion test voice.

The simpler example in `examples/voice.scd` exports a playable gated sine voice with `freq`, `amp`, `gate` and `pan` controls. `build/fm1-voice.wav` is a one-second host render with release at 0.25 seconds. `build/fm-render` emits raw native-endian stereo float samples at 44.1 kHz; `render-wav.py` converts them to portable 16-bit WAV.

## Supported profile

| UGen | Calculation rates | Current limits |
| --- | --- | --- |
| Control | initial, control | Up to 64 parameters; initial parameters freeze after rendering starts |
| LagControl, TrigControl | control | Per-output lag state; triggers reset after one block; lag times are initial rate |
| SinOsc | audio, control | Frequency and phase inputs; bounded integer phase accumulator |
| Saw, Pulse | audio, control | Band-limited impulse kernels with harmonic-count crossfades; parameters initial/control rate |
| LFSaw, LFTri, LFPulse | audio, control | Frequency may be audio rate; initial phase is initial rate; pulse width initial/control rate |
| WhiteNoise, PinkNoise | audio, control | SuperCollider taus88 random stream per voice |
| LFNoise0, LFNoise1, LFNoise2 | audio, control | Initial/control frequency, sampled when generating the next segment |
| Rand / ExpRand | initial | Bounds sampled once per voice; initial/control bounds accepted. ExpRand requires nonzero bounds of the same sign |
| RandSeed (additional) | initial, control, audio | Rising-edge reseeding; seed must be a literal integer within ±2²⁴ |
| FreeVerb, FreeVerb2 | audio | Audio signal input(s), initial/control mix/room/damp, parameters clipped to 0–1; caller-owned memory |
| MiVerb | audio | One/two audio inputs, stereo output; six initial/control parameters, pinned upstream limits; caller-owned memory |
| SampleRate, SampleDur, ControlRate, ControlDur, BlockSize, RadiansPerSample | initial | Information for this voice; control rate uses engine block size 64 |
| Delay1, Delay2 | audio, control | One/two calculation-sample delay; explicit initial values are initial/control inputs |
| OnePole, OneZero | audio, control | Initial/control coefficient in [-1,1], interpolated across audio blocks |
| LPZ1, HPZ1 | audio, control | One-sample average/difference; reference constructor state |
| Latch, Gate | audio, control | Rising-edge capture / positive-trigger tracking; inputs no faster than output |
| TRand, TExpRand | audio, control | Initial/control bounds; audio outputs require audio triggers; exponential bounds nonzero and same sign |
| Dust, Dust2 | audio, control | Initial/control density; deterministic per-voice random stream; nonpositive density produces silence |
| Impulse | audio, control | Frequency may be audio rate; phase must be initial rate; zero and negative frequency supported |
| Decay, Decay2 | audio, control | Initial/control times, nonnegative; -60 dB time constants; zero and equal-time cases supported |
| EnvGen | audio, control | All nine curve codes; up to 32 segments; details below |
| Pan2 | audio, control | Position can be audio rate; level must be initial/control rate |
| In | audio | Bus 0, one or two channels from the immutable input mix; effect slot only in the instrument |
| Out | audio | Bus 0, literal or initial/control expression; one or two audio channels; additive mixing |
| UnaryOpUGen | initial, control, audio | Explicit selector subset below |
| BinaryOpUGen | initial, control, audio | Explicit selector subset below |
| MulAdd, Sum3, Sum4 | initial, control, audio | Optimizer-generated arithmetic nodes |
| DC | initial, control, audio | Samples its value at initialization |
| K2A | audio | Initial/control input; block interpolation |
| A2K | control | Takes the first input sample of each block |
| Line, XLine | audio, control | Initial start/end/duration; doneAction 0/2 |
| Resonz, Ringz | audio, control | Initial/control frequency and bandwidth/decay; block coefficient interpolation |
| LPF, HPF, RLPF, BPF, LeakDC | audio, control | Initial/control cutoff, resonance/bandwidth or coefficient; block interpolation |
| Lag, LagUD | audio, control | Initial/control lag times; changing coefficients are interpolated |
| Select | initial, control, audio | Audio output requires audio choices; index is truncated and clamped |
| Clip | audio, control | Bounds may be audio rate; control bounds interpolate |
| Klang, Klank | audio | Initial-rate bank parameters; 64 total partials per graph |
| DelayN/L/C, CombN/L/C, AllpassN/L/C | audio, control | Literal maxdelaytime; delay time may be audio rate; decay initial/control rate; bounded caller-owned memory |

This is a tested compatibility subset, not full scsynth compatibility or bit-identical output. `DynKlang` expands to supported oscillators and arithmetic. `DynKlank` expands to Ringz and arithmetic, including control-rate frequency/decay modulation. Fixed Klang/Klank banks retain initial-rate parameter restrictions.

Unary selectors: neg (0), abs (5), ceil (8), floor (9), frac (10), sign (11), squared (12), cubed (13), sqrt (14), exp (15), reciprocal (16), midicps (17), cpsmidi (18), midiratio (19), ratiomidi (20), dbamp (21), ampdb (22), log (25), log2 (26), log10 (27), sin (28), cos (29), tan (30), asin (31), distort (42), softclip (43).

Mixed audio/control arithmetic interpolates control operands across the block, including optimizer-generated MulAdd/Sum3/Sum4 nodes. A separate constructor pass initializes upstream outputs before the first audio block.

Binary selectors: add (0), subtract (1), multiply (2), divide (4), equal (6), unequal (7), less (8), greater (9), less-or-equal (10), greater-or-equal (11), min (12), max (13). Other selectors are rejected. Supporting the arithmetic UGen names does not imply support for every SuperCollider operator.

### Reverbs

FreeVerb (mono), FreeVerb2 (stereo) and MiVerb (mono/stereo input to stereo output) are implemented at **audio rate only**. Controls must be initial/control rate. Their entire memory reservation is charged to the existing caller-owned delay arena, including FreeVerb recurrence/counter state. `fm_delay_required` therefore budgets both delay and reverb storage. The common graph/voice/scratch structures have not grown.

| Type | Memory per instance | Parameters |
| --- | --- | --- |
| FreeVerb | 50,492 bytes | mix, room, damp, clipped to 0–1 |
| FreeVerb2 | 102,088 bytes | mix, room, damp, clipped to 0–1 |
| MiVerb | 65,536 bytes | time 0–1.25, drywet 0–1, damp mapped as upstream, hp 0–1, freeze > 0, diffusion 0–0.95 |

FreeVerb follows SuperCollider 3.14.1, including constructor processing, fixed delay lengths and calculation order. `tools/generate-freeverb.py` regenerates its arena-based C kernels from the pinned source. MiVerb follows mi-UGens commit `10f6824f68ad63475f67dfe3fe5016e2d2634f84`: packed signed-16 delay samples, floating-point filters/modulation, freeze input closure and the wrapper's output soft clipping (also applied to dry output). It accepts one or two input channels; arbitrary multichannel folding is outside this profile. Delay tap lengths remain fixed as upstream; sample rate changes modulation rates but do not rescale the rings.

Prefer one reverb in slot 0. FreeVerb2 exceeds the instrument's per-synth-voice 64 KiB limit but fits its shared effect allocation. The total remains `2 * synth memory + effect memory <= 128 KiB`. FreeVerb2 plus the supplied echo exceeds it. MiVerb plus that echo exactly fills the arena and leaves no synth delay memory; CPU feasibility is a separate hardware check.

The pinned MiVerb class is bundled for compilation. The manager/exporter now use the stock SuperCollider class library plus this class, without loading user extensions. No installed MiVerb plugin is needed for FM1 playback or host preview. Reference tests build an isolated upstream MiVerb plugin using the included pinned headers; the reference sources retain their licences. These additions have passed emulation/build tests, but have not been flashed or timed on an FM-1.

### Trigger and decay compatibility

These additions are verified in host emulation and the freestanding device-math backend; they are not in the published 0.1.0-preview firmware and have not been hardware-tested. `Dust` emits positive random impulses; `Dust2` emits bipolar impulses. `Decay` smooths its input with one exponential tail; `Decay2` subtracts attack and decay recurrences. Overlapping triggers add, so they are not amplitude-normalized envelopes. Equal attack/decay times yield zero; an attack longer than decay can yield negative output. Negative times fault the voice. Compensated single-precision state preserves long-tail accuracy without double arithmetic or extra allocation.

`TRand`/`TExpRand` draw once at construction and subsequently on triggers, using the shared seeded voice stream. For `.ar`, provide an audio trigger; wrap control triggers with `K2A.ar` and constant triggers with `DC.ar`. Bounds may change at control rate. `TExpRand.ar` deliberately matches the pinned **SuperCollider 3.14.1** implementation: throughout an audio block, it compares triggers with the previous block's final trigger. A positive held trigger can therefore cause several draws in that block. `.kr` uses ordinary rising-edge behavior. This version-specific quirk is reference-tested.

The new logarithmic conversions require positive input; zero/negative domains fault and silence the voice. Information UGens use the actual voice sample rate, including constructor-time caching. Delay1/Delay2 need no delay arena; at control rate their unit is one control block.

### Envelopes

`Env` constructors become EnvGen input data during export. The suite includes `Env.perc`, `adsr`, `dadsr`, `asr`, `cutoff`, `linen`, `triangle`, `sine` and custom breakpoints. The nine curves are step, linear, exponential, sine, Welch, numeric curvature, squared, cubed and hold. All nine have both audio- and control-rate reference tests.

Supported behaviour includes sustain/release nodes, looping, retrigger, audio-rate gates, forced release (`gate <= -1`), fractional segment durations, zero-duration segments and doneAction 0/2. A doneAction of 2 retires this voice after its current block; it does not manage a scsynth node tree.

Segment count, shape codes, release/loop indices and doneAction must be literal constants. Segment levels/times, numeric curve values and scale/bias may use initial/control signals; only the gate may be audio rate in this profile. Shape domain requirements still apply (for example, avoid exponential segments that cross zero). Non-finite or excessively large results fault and silence the voice rather than reaching the output.

Sine/Welch curves use elapsed-sample formulas instead of a nearly-unit recursive coefficient, avoiding loss of precision on a single-precision FPU. Compensated float products preserve fractional segment boundaries without introducing software double arithmetic.

### Delays and feedback filters

The N/L/C variants use no, linear or cubic interpolation. Comb/Allpass decay is the signed time to decay by 60 dB; negative values invert feedback. All nine have audio/control-rate references, changing delay and decay, negative feedback, zero-delay cases, and audio-rate delay modulation. The implementation also reproduces the reference cubic-allpass startup behaviour. Maximum delay must be a nonnegative literal so memory can be checked before playing; non-finite constants (including infinite decay) remain outside this profile. Actual delay time is clamped using the reference ring-length/minimum-sample rules. `Delay*` has the reference initial-zero bypass; feedback delays retain their minimum delay.

Rand and ExpRand use the per-voice random stream once at construction and retain their values across blocks, even when their control-valued bounds change. The instrument chooses a new seed per note; `fm_seed` or `RandSeed` makes tests repeatable. `randamp` therefore affects new notes only. The seeded five-harmonic `addy` integration fixture compares the entire 1.05-second voice lifetime plus silence against SuperCollider.

The output defaults to bus **0**, the first stereo pair (left 0, right 1). `Out.ar(output, signal)` works with `output=0`; no parameter name is special to the renderer. Literal nonzero buses are rejected on import. A nonzero computed/control bus faults and silences that voice, preserving other voices; it is never silently redirected. Internal bus routing is not implemented.

## Bounds and runtime contract

One definition per version-2 file; at most 65,536 file bytes, 160 nodes, 192 output signals, 1,024 input references, 512 constants, 64 control values/names, 32 segments per envelope and 64 bank partials in total. Names are limited to 63 bytes. Variants, multiple definitions, unknown nodes/selectors/rates, forward references, malformed/truncated files and trailing bytes are rejected.

The render block is 64 samples. Host references use 44.1 kHz. `fm_start` accepts 8–192 kHz, but those other sample rates do not yet have reference suites. A graph occupies 13,620 bytes, a voice 35,752 bytes, and shared scratch 49,152 bytes on this host. The band-limited oscillators additionally use 65,544 bytes of read-only sine/cosecant tables, generated at build-development time by `tools/generate-sine-tables.py`. These are compiled host sizes, not measurements of final device RAM, stack or CPU usage.

`fm_load` runs outside the audio callback and invalidates the destination graph on failure. Never load into a graph while a voice uses it. Treat a validated graph as immutable. `fm_start` resets all per-voice state and uses deterministic seed 0. For graphs containing delay UGens, call `fm_delay_required` to budget memory and `fm_bind_delay` after start, before the first render. The caller supplies a non-overlapping float array that stays alive for that voice. The bind clears used memory; rendering allocates nothing. Each line rounds its maximum delay plus block overhead up to a power of two. Total delay memory is bounded to 65,536 samples per voice by the general host API; insufficient binding faults the voice before output. This memory is additional to the voice/scratch sizes above. The instrument adapter uses a smaller 16,384-sample budget per voice. Call `fm_seed` before the first render to choose a distinct seed per note/voice; explicit RandSeed nodes can override it. Noise nodes share the voice’s random stream in graph order, including initialization draws. A seeded graph is repeatable across restarts. Call `fm_set` between render blocks on the owning audio thread; cross-thread queuing is the firmware adapter's responsibility. Array controls can be addressed by index by a future adapter; the current named setter changes the named first value only.

`fm_render` adds a block to the caller's interleaved stereo buffer. Clear it once before mixing voices. Scratch can be shared serially among voices; each voice owns its own oscillator, envelope and resonator state. No allocation, file access or logging occurs inside rendering. The current numerical fault policy discards the faulting voice's whole block, preserves other voices, and deactivates that voice until restart. Voice allocation, stealing and MIDI event scheduling are not implemented here.

## Validation and remaining work

Verified on 2026-10-28: **553/553 SuperCollider reference graphs, 34 API test groups, and 440,000 sanitizer mutation/truncation cases passed**. All 76 types other than In appear in the reference suite; stereo In is covered by dedicated API checks. The saved verification record is in `../evidence/`.

The reference suite compares exported graphs against SuperCollider 3.14.1 and writes per-fixture errors to `build/reference-results.json`. Envelope tests use a peak-error limit of 5e-6 and RMS limit of 1e-6. New control/filter/noise/oscillator/delay fixtures use peak 5e-6 / RMS 1e-6, except audio-rate LagUD (peak 2e-5 / RMS 8e-6) for accumulated single-precision rounding. Earlier non-envelope fixtures use peak 1e-4 / RMS 3e-5, except audio-rate panning (peak 1e-3) because small floating-point differences can select adjacent pan-table entries. The 37 m7 reverb graphs run for approximately four seconds and use peak 5e-6 / RMS 1e-6, except the overdriven MiVerb oscillator integration case (peak 2e-4 / RMS 2e-5): tiny oscillator differences enter quantized nonlinear feedback. A separate identical-input oracle compares 819,200 frames across four sample rates and two input amplitudes, including control changes, freeze and overdrive; it matches the unmodified upstream DSP exactly on the tested host. The m6 additions use peak 1e-4 / RMS 3e-5. One sinusoidally frequency-modulated Impulse fixture uses a pulse-count/amplitude oracle with a maximum one-sample timing difference, because upstream oscillator rounding can move a discontinuous wrap. A separate exact-valued frequency-modulation fixture retains ordinary waveform comparison. The timing oracle is tested to reject missing, extra and two-sample-shifted pulses. Passing these finite fixtures is not an exhaustive guarantee over every possible graph or parameter range.

API tests cover controls, restart, done actions, mixing, scratch reuse, invalidation on failed load, malformed references, every truncation of a fixture, numerical faults and deterministic mutations. `make sanitize` exercises 440,000 mutations/truncations across twenty-two graph families with bounds, undefined-behaviour and float-conversion checks. Apple's installed AddressSanitizer deadlocked in its own startup before `main`; no ASan success is claimed. A stack sample is retained under `build/asan-startup-sample.txt`.

All 49 planned UGen names are implemented. This does not imply support for every scsynth graph, operator selector or parameter combination. Unsupported types/rates/selectors are rejected; numerical faults retire the voice. In particular:

- Saw/Pulse frequency must be positive, no greater than half the calculation rate, and require at most 2²⁴ harmonics. Pulse width must be in 0..1. Invalid runtime parameters fault the voice.
- Filter and lag parameters, band-limited oscillator parameters and LFNoise frequency cannot be audio-rate signals in this profile. Fixed bank parameters remain initial rate.
- RandSeed supports literal integer seeds only. Demand-rate seed expressions and RandID are excluded.
- There is no automatic voice seed selection, server node tree, bus routing beyond stereo bus zero, shared buffer system, general feedback graph or plugin loading.

The seeded white/pink noise and mixed-noise reference cases match the reference samples exactly at audio rate. Other kernels are assessed by the explicit tolerances above; the full runtime is not claimed bit-identical.

The companion `../fm1-synth/` provides the two-voice instrument, eight playable slots plus a permanent effect, browser manager, physical flash journal and USB MIDI transfer. Both the host and freestanding device math backends have passed the 553 reference comparisons. The supplied patches have separate hardware timing checks; admitted graph size is not a guarantee of realtime performance.

## Provenance

New engine code is GPL-3.0-or-later. DSP, noise, interpolation and oscillator-table algorithms were adapted from SuperCollider's GPL-2.0-or-later sources; this distribution uses the compatible GPL-3.0-or-later option. See `LICENSE` and `reference/README.md`. Existing fm1-base files keep their original licenses.

## Device renderer work (28 October 2026)

`fm_fast.inc` binds inputs once per node, specializes arithmetic, uses the existing interpolated sine table, and calculates control-rate panning gains once per block. Numeric envelope curves in [-20,20] use a compensated exponential recurrence; extreme curvature retains the direct equation. An independent analytic test checks 36 combinations of curvature and endpoints, including nonzero starts and negative levels. The original SuperCollider fixture checks still apply.

The device backend and adapter are in `../fm1-synth/firmware/`; their hardware test status and measured memory are documented by that project. The device and Mac preview allocate two voices. The device uses the optional `fm_prepare` constructor pass and a 128-frame startup interval to reduce note-on bursts. The hardware acceptance record, rather than graph admission alone, determines whether a patch meets the realtime budget.

The polyphony update adds a 25th API test group verifying idempotent early preparation and identical later audio with shared scratch. Scalar/audio arithmetic loops bind their rates outside the sample loop; scalar-gated numeric envelopes process runs up to the next segment boundary; unchanged Pan2 positions reuse their gains. IEEE-754 magnitude checks retain NaN/infinity and excessive-output rejection.

## Stereo input and permanent effects

`In.ar(0, 1)` reads the left channel and `In.ar(0, 2)` reads both channels from the immutable stereo input passed to `fm_render_input`. Missing input is silence. Nonzero buses fault the voice. The instrument admits this UGen only in its dedicated effect slot; ordinary note SynthDefs keep using `Out.ar(0, ...)`. Effect output replaces the instrument mix before final gain/metering; it is not recursively fed into the same input bus. The 26th API test group verifies channel mapping, silence and bus/rate rejection. The full 553-graph SuperCollider suite also passes on host and device-math backends.
