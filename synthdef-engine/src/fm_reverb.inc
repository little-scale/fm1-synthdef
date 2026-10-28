/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fm_freeverb.inc"
#include "fm_miverb.inc"
static __attribute__((noinline)) void reverb_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,int initializing) {
 unsigned j,k;float *mem;
 if(s->total!=delay_length(g,n,v->sample_rate) || s->phase_offset>v->delay_capacity || s->total>v->delay_capacity-s->phase_offset || !v->delay_memory){v->fault=1;for(k=0;k<n->outputs;k++)for(j=0;j<count;j++)b->signal[n->output+k][j]=0;return;}
 mem=v->delay_memory+s->phase_offset;
 if(n->type==FM_FreeVerb)render_freeverb(g,b,n,mem,(int)count);
 else if(n->type==FM_FreeVerb2)render_freeverb2(g,b,n,mem,(int)count);
 else render_miverb(g,v,b,n,s,mem,count,initializing);
 for(k=0;k<n->outputs;k++)for(j=0;j<count;j++)checked_signal(v,&b->signal[n->output+k][j],b->signal[n->output+k][j]);
 s->initialized=1;
}
