/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from SC 3.14.1 DelayUGens.cpp, James McCartney (GPL-2.0-or-later).
 * Caller-owned bounded rings. No heap allocation, including at note start.
 */
static uint32_t delay_length(const fm_graph *g,const fm_node *n,float sr) {
 /* State and exact-size rings, in float-sized allocation units. MiVerb's
  * packed 16-bit ring is accessed through memcpy, without type aliasing. */
 if(fm_is_rings(n->type))return rings_words(n->type-FM_MiRings1+1,(unsigned)g->constant[g->ref[n->input+9].output]);
 if(fm_is_gendy(n->type)){uint32_t cp=(uint32_t)g->constant[g->ref[n->input+(n->type==FM_Gendy3?7:8)].output];return n->type==FM_Gendy1?2*cp:n->type==FM_Gendy2?4*cp:4*cp+2;}
 if(n->type==FM_FreeVerb)return 12623;
 if(n->type==FM_FreeVerb2)return 25522;
 if(n->type==FM_MiVerb)return 16384;
 float x=g->constant[g->ref[n->input+1].output]*(n->rate==2?sr:sr/FM_BLOCK)+1;
 uint32_t want,length=1;
 if(!isfinite(x) || x>FM_DELAY_LIMIT-FM_BLOCK || x<1)return UINT32_MAX;
 want=(uint32_t)ceilf(x)+(n->rate==2?FM_BLOCK:1);
 while(length<want)length*=2;
 return length;
}
uint32_t fm_delay_required(const fm_graph *g,float sr) {
 uint32_t total=0;unsigned i;
 if(g->valid!=FM_VALID || !isfinite(sr) || sr<8000 || sr>192000)return UINT32_MAX;
 for(i=0;i<g->nodes;i++)if(fm_uses_memory(g->node[i].type)) {
  uint32_t n=delay_length(g,&g->node[i],sr);
  if(n>FM_DELAY_LIMIT-total)return UINT32_MAX;
  total+=n;
 }
 return total;
}
int fm_bind_delay(fm_voice *v,const fm_graph *g,float *memory,uint32_t capacity) {
 uint32_t need=fm_delay_required(g,v->sample_rate),offset=0;unsigned i;
 if(v->prepared || v->blocks || !v->active || need==UINT32_MAX || need>capacity || (need && !memory))return 0;
 v->delay_memory=memory;v->delay_capacity=need;
 if(need)memset(memory,0,need*sizeof(float));
 for(i=0;i<g->nodes;i++)if(fm_uses_memory(g->node[i].type)) {
  fm_state *s=&v->state[i];s->phase_offset=offset;s->total=delay_length(g,&g->node[i],v->sample_rate);offset+=s->total;
 }
 return 1;
}
static float delay_feedback(float delay,float decay) {
 if(delay==0 || decay==0)return 0;
 return (decay>0?1.0f:-1.0f)*expf(-6.907755278982137f*delay/fabsf(decay));
}
static float delay_cubic(float x,float a,float b,float c,float d) {
 float c0=b,c1=0.5f*(c-a),c2=a-2.5f*b+2*c-0.5f*d,c3=0.5f*(d-a)+1.5f*(b-c);
 return ((c3*x+c2)*x+c1)*x+c0;
}
static void delay_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int init) {
 unsigned kind=(n->type-FM_DelayN)/3,interp=(n->type-FM_DelayN)%3,j;
 float time=input(g,b,n,2,0),decay=kind?input(g,b,n,3,0):0,min=interp==2?2:1;
 float ds,fb,dstep,fstep;int changed=time!=s->freq || decay!=s->width;
 int audio=input_rate(g,n,2)==2;
 int bypass=!kind && input_rate(g,n,2)==0 && time==0;
 if(!s->total || s->phase_offset>v->delay_capacity || s->total>v->delay_capacity-s->phase_offset || !v->delay_memory){v->fault=1;for(j=0;j<count;j++)b->signal[n->output][j]=0;return;}
 ds=fminf(s->total,fmaxf(min,time*sr));fb=kind?delay_feedback(time,decay):0;
 if(init){s->last=ds;s->a0=fb;s->freq=time;s->width=decay;s->initialized=1;b->signal[n->output][0]=bypass?input(g,b,n,0,0):0;return;}
 dstep=(ds-s->last)/count;fstep=(fb-s->a0)/count;ds=s->last;fb=s->a0;
 for(j=0;j<count;j++) {
  float x=input(g,b,n,0,j),frac,y,write;uint32_t index,mask=s->total-1;
  float *ring=v->delay_memory+s->phase_offset;
  if(bypass){b->signal[n->output][j]=x;continue;}
  if(audio){float t=input(g,b,n,2,j);ds=fminf(s->total,fmaxf(min,t*sr));fb=kind?delay_feedback(t,decay):0;}
  else if(changed){ds+=dstep;if(interp || time!=s->freq)fb+=fstep;}
  index=s->phase_acc-(uint32_t)ds;frac=ds-(uint32_t)ds;
  if(!kind)ring[s->phase_acc & mask]=x;
  y=ring[index & mask];
  if(interp==1)y+=frac*(ring[(index-1)&mask]-y);
  else if(interp==2)y=delay_cubic(frac,ring[(index+1)&mask],y,ring[(index-1)&mask],ring[(index-2)&mask]);
  write=x+fb*y;
  if(kind)ring[s->phase_acc & mask]=write;
  if(kind==2)y-=fb*write;
  /* SC's checked cubic allpass emits zero until its first read tap exists. */
  if(kind==2 && interp==2 && (uint32_t)s->env.elapsed<s->total && s->phase_acc+1<(uint32_t)ds)y=0;
  if(!isfinite(y) || !isfinite(write) || fabsf(y)>1e12f || fabsf(write)>1e12f){v->fault=1;y=0;}
  b->signal[n->output][j]=y;s->phase_acc++;
  if((uint32_t)s->env.elapsed<s->total)s->env.elapsed++;
  if(!audio && changed && !interp && time==s->freq)fb+=fstep;
 }
 s->last=ds;s->a0=fb;s->freq=time;s->width=decay;
}
