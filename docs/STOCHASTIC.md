# Random and chaotic generators

The development firmware adds 18 UGen types, bringing the total to **142**. These are bounded ports of SuperCollider 3.14.1 algorithms. Their state fits in the existing node storage; none allocates DSP-arena or delay memory. Hardware timing/listening checks are deferred.

## Available types

| UGen | Rates | Purpose and supported inputs |
| --- | --- | --- |
| `Crackle` | ar, kr | Chaotic crackle; initial/control chaos parameter 0–2 |
| `LFClipNoise` | ar, kr | Random −1/+1 segments; initial/control frequency |
| `LFDClipNoise` | ar, kr | Dynamic random −1/+1 segments; frequency may be audio rate |
| `GrayNoise` | ar, kr | Flips one random bit of a 32-bit state each calculation sample |
| `Hasher` | ar, kr | Deterministic float-bit hash; `.ar` requires audio input |
| `CoinGate` | ar, kr | Passes trigger edges with initial/control probability 0–1; `.ar` requires audio trigger |
| `TIRand` | ar, kr | Inclusive random integer on initialization and rising trigger edges; `.ar` requires audio trigger |
| `TWindex` | ar, kr | Weighted choice of index 0–N−1 on initialization and rising trigger edges |
| `Logistic` | ar, kr | Logistic map; initial/control chaos parameter 0–4, initial value 0–1 |
| `HenonN`, `HenonL`, `HenonC` | ar | Hénon map with no, linear or cubic interpolation |
| `CuspN`, `CuspL` | ar | Cusp map with no or linear interpolation |
| `LatoocarfianN`, `LatoocarfianL`, `LatoocarfianC` | ar | Latoocarfian map with no, linear or cubic interpolation |
| `LorenzL` | ar | Lorenz system using upstream RK4 integration and linear interpolation |

Normal SuperCollider defaults, `mul`/`add` and array expansion work within the ordinary graph limits. Parameters are initial/control rate except where the table allows audio input. `TWindex` accepts an audio trigger too. The nine ChaosUGens types are audio-only, as in SuperCollider; their iteration frequency is still an initial/control input. `Logistic`'s initial value is initial-only. Changing live initial-condition controls on ChaosUGens follows the pinned upstream reset behavior.

`TIRand` bounds are truncated to integers, must be ordered, and each must lie within ±8,388,607. `TWindex` accepts 1–64 weights, all nonnegative, with a positive finite sum. Its normalize flag is 0 or 1: 1 normalizes arbitrary weights; 0 requires a sum of 1 within 0.00001. Weights and normalize can change at control rate. Allocation is fixed by the graph's weight count.

Noise and Logistic frequencies must lie between 0 and four times the calculation rate. `LFClipNoise` and `Logistic` retain the upstream minimum frequency and integer hold counter, capped here at one billion samples. `LFDClipNoise` wraps its fractional phase and responds to frequency changes while holding a value.

## Chaotic precision and bounds

The equations, interpolation, constructor behavior and iteration clock come from the pinned upstream source. The device uses **single-precision state and calculations**, including sine and square root. Small rounding differences in a chaotic recurrence naturally grow into different long waveforms. The aim is the same useful generator and control behavior; identical samples, attractor trajectories and periodic windows are not promised.

Finite precision can eventually create cycles. Tests check selected long renders for activity, nonfinite values and exact repeating periods up to 128 samples; they cannot prove that no longer cycle exists. Musically useful periodic regions are also an intentional property of these maps.

Additional bounds prevent runaway or unreasonable inputs:

| Family | Bounds besides frequency |
| --- | --- |
| Hénon | `abs(a) <= 2`, `abs(b) <= 1`, `abs(x0/x1) <= 1.5` |
| Cusp | `abs(a/b) <= 2`, `abs(xi) <= 4` |
| Latoocarfian | `abs(a/b) <= 3`, `abs(c/d) <= 2`, `abs(xi/yi) <= 4` |
| Lorenz | `s` 0–30, `r` 0–60, `b` 0–8, `h` 0–0.05, initial coordinates ±100 |

ChaosUGens frequency is 0–4×sample rate; the upstream clock performs at most one recurrence per audio sample. Accepted parameters can still describe an unstable system. Nonfinite or excessively large state faults and silences the voice; the bounds do not guarantee stability for every combination. Avoid suddenly pushing feedback coefficients to extremes. `LeakDC` and a smoothed filter cutoff are useful when shaping asymmetric chaotic signals.

## Deliberate TIRand fixes

The pinned SuperCollider audio implementation can draw repeatedly during a held positive trigger because its loop does not update the previous-trigger value per sample. This runtime updates it each sample, producing one random value per rising edge.

Upstream can also read its own negative constructor output as its initial trigger when buffers are reused in place. Our separate graph signals and captured trigger avoid that extra initial draw. Reference fixtures keep the trigger alive as a second output to avoid the upstream buffer alias; a source oracle independently reproduces the alias behavior. These are documented semantic fixes, not precision tolerances.

## Validation and examples

The 80 new reference graphs include audio/control noise, live parameters and stable/chaotic map settings. Stable cases use waveform tolerances. Crackle and selected chaotic cases use explicitly labeled mean, RMS, standard-deviation and first-difference checks. Broad statistical checks alone do not establish correctness: independent C++ oracles compile the unmodified upstream sources and compare transitions from identical states, separating arithmetic mistakes from chaotic drift. Integer random mapping and float-bit hashing are checked directly too.

- [Stochastic plucks](../fm1-synth/examples/stochastic-plucks.scd): probability-gated impulses and random integer pitches. Knobs control rate, probability, decay and pitch range.
- [Chaos voice](../fm1-synth/examples/chaos-voice.scd): Hénon/Cusp blend. Knobs control iteration rate, Hénon coefficient, cutoff and blend.
- [Chaos attractors](../fm1-synth/examples/chaos-attractors.scd): Lorenz/Latoocarfian blend. Knobs control integration step, iteration rate, blend and cutoff.

Each example also has `.scsyndef` and `.fmslot` files. Importing is optional; no saved slots are changed. The companion simulator checks two voices, master echo, live knob changes, voice stealing and release. See [verification](VERIFICATION.md) for measured results and limitations. Restart a running local editor to load the rebuilt engine library.
