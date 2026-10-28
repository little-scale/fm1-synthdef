/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FMS_PERFORMANCE_H
#define FMS_PERFORMANCE_H
#include "fm_synth.h"
#define FMS_VOICES 2u /* maximum simultaneous voices; each has its own delay arena */
#define FMS_DELAY_SAMPLES 16384u
#define FMS_DELAY_TOTAL (FMS_VOICES*FMS_DELAY_SAMPLES)
typedef struct {
 fm_voice synth;
 uint32_t id,age,released_frames,fade;
 uint8_t held,forced,startup_blocks;
 float last[2],tail[2];
} fms_note;
typedef struct {
 const fm_graph *graph,*effect_graph;
 fm_voice *effect_voice;
 uint32_t voice_delay_samples;
 fm_scratch scratch;
 fms_note voice[FMS_VOICES];
 float delays[FMS_DELAY_TOTAL];
 float knobs[8],master,gain,peak[2];
 uint32_t clip[2],serial,steals,faults;
 unsigned voice_limit; /* 1..FMS_VOICES; defaults to the full allocation */
 unsigned prepare_blocks; /* optional silent startup blocks; device uses two */
 int octave;
 int8_t keys[27]; /* pitch captured at press; -1 = up */
} fms_performance;
/* All calls on one audio owner thread. UI/MIDI adapter must queue commands. */
int fms_init(fms_performance *,const fm_graph *);
void fms_panic(fms_performance *);
int fms_effect_valid(const fm_graph *);
/* Caller owns the persistent effect graph/voice. Attach preserves its state. */
int fms_attach_effect(fms_performance *,const fm_graph *,fm_voice *);
int fms_reset_effect(fms_performance *);
void fms_note_on(fms_performance *,uint32_t id,unsigned midi,unsigned velocity);
void fms_note_off(fms_performance *,uint32_t id);
void fms_key(fms_performance *,unsigned key,unsigned velocity,int down);
void fms_octave(fms_performance *,int delta);
void fms_knob(fms_performance *,unsigned knob,float value);
void fms_render(fms_performance *,float stereo[FM_BLOCK*2]);
#endif
