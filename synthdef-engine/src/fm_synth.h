/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FM_SYNTH_H
#define FM_SYNTH_H
#include <stddef.h>
#include <stdint.h>
#define FM_BLOCK 64
#define FM_NODES 160
#define FM_OUTPUTS 192
#define FM_INPUTS 1024
#define FM_CONSTANTS 512
#define FM_CONTROLS 64
#define FM_SEGMENTS 32
#define FM_PARTIALS 64
#define FM_GENDY_POINTS 64
#define FM_SCALE_ENTRIES 128
#define FM_SCALE_TABLES 16
#define FM_FILE_BYTES 65536
#define FM_DELAY_LIMIT 65536u /* maximum samples across all delay lines of one voice */

typedef enum {
 FM_Control, FM_SinOsc, FM_EnvGen, FM_Pan2, FM_Out,
 FM_BinaryOpUGen, FM_UnaryOpUGen, FM_MulAdd, FM_Sum3, FM_Sum4,
 FM_DC, FM_K2A, FM_A2K, FM_Line, FM_XLine,
 FM_Resonz, FM_Ringz, FM_Klang, FM_Klank,
 FM_LagControl, FM_TrigControl, FM_LFSaw, FM_LFTri, FM_LFPulse, FM_Lag, FM_LagUD, FM_LPF, FM_HPF, FM_RLPF, FM_BPF, FM_LeakDC, FM_Select, FM_Clip,
 FM_WhiteNoise, FM_PinkNoise, FM_LFNoise0, FM_LFNoise1, FM_LFNoise2, FM_RandSeed, FM_Saw, FM_Pulse,
 FM_DelayN, FM_DelayL, FM_DelayC, FM_CombN, FM_CombL, FM_CombC, FM_AllpassN, FM_AllpassL, FM_AllpassC,
 FM_Rand, FM_ExpRand, FM_In, FM_SampleRate, FM_SampleDur, FM_ControlRate, FM_ControlDur, FM_BlockSize, FM_RadiansPerSample, FM_Delay1, FM_Delay2, FM_OnePole, FM_OneZero, FM_LPZ1, FM_HPZ1, FM_Latch, FM_Gate, FM_TRand, FM_TExpRand, FM_Dust, FM_Dust2, FM_Impulse, FM_Decay, FM_Decay2, FM_FreeVerb, FM_FreeVerb2, FM_MiVerb, FM_Fold, FM_Wrap, FM_BrownNoise, FM_ClipNoise, FM_Trig, FM_Trig1, FM_PulseDivider, FM_PulseCount, FM_ToggleFF, FM_SetResetFF, FM_Stepper, FM_XFade2, FM_LinXFade2, FM_Balance2, FM_LinExp, FM_Slew, FM_Lag2, FM_Lag3, FM_MiMu, FM_RHPF, FM_BRF, FM_VarSaw, FM_SyncSaw, FM_LFCub, FM_LFPar, FM_LFDNoise0, FM_LFDNoise1, FM_LFDNoise3, FM_Amplitude, FM_BPeakEQ, FM_BLowShelf, FM_BHiShelf, FM_Gendy1, FM_Gendy2, FM_Gendy3, FM_Formant, FM_Formlet, FM_MaxLocalBufs, FM_LocalBuf, FM_SetBuf, FM_DegreeToKey, FM_MiRings1, FM_MiRings2, FM_MiRings3, FM_MiRings4, FM_MiRings5, FM_MiRings6, FM_Crackle, FM_LFClipNoise, FM_LFDClipNoise, FM_GrayNoise, FM_Hasher, FM_CoinGate, FM_TIRand, FM_TWindex, FM_Logistic, FM_HenonN, FM_HenonL, FM_HenonC, FM_CuspN, FM_CuspL, FM_LatoocarfianN, FM_LatoocarfianL, FM_LatoocarfianC, FM_LorenzL, FM_TYPES
} fm_type;
static inline int fm_is_delay(unsigned type){return type>=FM_DelayN && type<=FM_AllpassC;}
static inline int fm_is_reverb(unsigned type){return type>=FM_FreeVerb && type<=FM_MiVerb;}
static inline int fm_is_gendy(unsigned type){return type>=FM_Gendy1 && type<=FM_Gendy3;}
static inline int fm_is_rings(unsigned type){return type>=FM_MiRings1 && type<=FM_MiRings6;}
static inline int fm_uses_memory(unsigned type){return fm_is_delay(type)||fm_is_reverb(type)||fm_is_gendy(type)||fm_is_rings(type);}
typedef struct { int16_t node; uint16_t output; } fm_ref;
typedef struct {
 uint16_t input, inputs, output, outputs, partial, partials;
 int16_t special;
 uint8_t type, rate;
} fm_node;
typedef struct { char name[64]; uint16_t index; } fm_control_name;
typedef struct {
 uint32_t valid;
 uint16_t nodes, outputs, inputs, constants, controls, names, partials;
 char name[64];
 fm_node node[FM_NODES];
 fm_ref ref[FM_INPUTS];
 uint8_t output_rate[FM_OUTPUTS],used[FM_NODES];
 float constant[FM_CONSTANTS], defaults[FM_CONTROLS];
 fm_control_name control_name[FM_CONTROLS];
} fm_graph;
typedef struct {
 float level, end, residual, previous_gate, grow, start, duration, curve;
 float curve_offset,curve_compensation,curve_progress;
 int32_t remaining, elapsed;
 int16_t stage, shape, stages, release_node, loop_node, done_action;
 uint8_t released, done;
} fm_env;
typedef struct {
 fm_env env;
 float phase, last, y1, y2, a0, b1, b2, freq, width;
 float previous[4],initial_value;
 uint32_t phase_acc, phase_offset, total, dice[16];
 uint8_t initialized;
} fm_state;
typedef struct { float y1, y2, b1, b2, a0; } fm_partial;
typedef struct {
 float control[FM_CONTROLS], sample_rate;
 float control_smooth[FM_OUTPUTS], control_coeff[FM_OUTPUTS];
 fm_state state[FM_NODES];
 fm_partial partial[FM_PARTIALS];
 uint8_t active, fault, prepared;
 uint32_t blocks, random[3];
 float *delay_memory;
 uint32_t delay_capacity;
} fm_voice;
/* Scratch can be shared serially by all voices. No allocation in any function. */
typedef struct { float signal[FM_OUTPUTS][FM_BLOCK]; } fm_scratch;
/* Load on the non-audio thread. Failure invalidates graph; never load into a playing graph. */
int fm_load(fm_graph *, const void *, size_t, char *error, size_t error_size);
int fm_start(fm_voice *, const fm_graph *, float sample_rate);
/* Literal maxdelaytime permits budgeting before playback. UINT32_MAX = too large. */
uint32_t fm_delay_required(const fm_graph *, float sample_rate);
/* After start, before render; caller owns memory for the voice's whole lifetime.
 * Clears used memory; no allocation. Insufficient memory returns 0. */
int fm_bind_delay(fm_voice *, const fm_graph *, float *memory, uint32_t samples);
/* Set the per-voice seed before the first render; default seed is 0. */
int fm_seed(fm_voice *, uint32_t seed);
int fm_set(fm_voice *, const fm_graph *, const char *name, float value);
/* Optional constructor pass without advancing audio; idempotent. Bind delays
 * and set initial controls/seed first. fm_render prepares automatically. */
void fm_prepare(const fm_graph *, fm_voice *, fm_scratch *);
/* Add one 64-frame block to interleaved stereo. Caller clears destination. */
void fm_render(const fm_graph *, fm_voice *, fm_scratch *, float stereo[FM_BLOCK * 2]);
/* Input bus 0 is an immutable interleaved stereo block; NULL means silence. */
void fm_render_input(const fm_graph *, fm_voice *, fm_scratch *, float *stereo, const float *input_stereo);
const char *fm_type_name(unsigned type);
#endif
