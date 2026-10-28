/* SPDX-License-Identifier: GPL-3.0-or-later
 * SC 3.14.1 taus88, seed hash and noise algorithms. See reference/README.md.
 */
static void random_seed(fm_voice *v,uint32_t seed) {
 seed+=~(seed<<15);seed^=seed>>10;seed+=seed<<3;seed^=seed>>6;seed+=~(seed<<11);seed^=seed>>16;
 v->random[0]=1243598713u^seed;if(v->random[0]<2)v->random[0]=1243598713u;
 v->random[1]=3093459404u^seed;if(v->random[1]<8)v->random[1]=3093459404u;
 v->random[2]=1821928721u^seed;if(v->random[2]<16)v->random[2]=1821928721u;
}
static uint32_t random_word(fm_voice *v) {
 uint32_t *r=v->random;
 r[0]=((r[0]&~1u)<<12)^(((r[0]<<13)^r[0])>>19);
 r[1]=((r[1]&~7u)<<4)^(((r[1]<<2)^r[1])>>25);
 r[2]=((r[2]&~15u)<<17)^(((r[2]<<3)^r[2])>>11);
 return r[0]^r[1]^r[2];
}
static float random_bipolar(fm_voice *v) {uint32_t bits=0x40000000u|(random_word(v)>>9);float x;memcpy(&x,&bits,4);return x-3.0f;}
static float random_uniform(fm_voice *v) {uint32_t bits=0x3f800000u|(random_word(v)>>9);float x;memcpy(&x,&bits,4);return x-1.0f;}
static float noise_tick(fm_voice *v,const fm_node *n,fm_state *s,float frequency,float sr) {
 fm_env *e=&s->env;float out;
 if(!s->initialized) {
  if(n->type==FM_PinkNoise) {unsigned k;s->total=0;for(k=0;k<16;k++){s->dice[k]=random_word(v)>>13;s->total+=s->dice[k];}}
  if(n->type==FM_LFNoise1)e->level=random_bipolar(v);
  if(n->type==FM_LFNoise2){e->end=random_bipolar(v);e->start=e->end*0.5f;}
 }
 if(n->type==FM_WhiteNoise)return random_bipolar(v);
 if(n->type==FM_PinkNoise) {
  uint32_t word=random_word(v),bits,copy=word;unsigned k=0;
  /* Define the otherwise undefined ctz(0) case; no data-dependent long loop. */
  if(!copy)k=32;else while(!(copy&1u)){k++;copy>>=1;}
  k &= 15;s->total+=(word>>13)-s->dice[k];s->dice[k]=word>>13;
  bits=(s->total+(random_word(v)>>13))|0x40000000u;memcpy(&out,&bits,4);return out-3.0f;
 }
 if(e->remaining<=0) {
  e->remaining=(int32_t)fminf(1e9f,sr/fmaxf(0.001f,frequency));
  if(e->remaining<(n->type==FM_LFNoise2?2:1))e->remaining=n->type==FM_LFNoise2?2:1;
  if(n->type==FM_LFNoise0)e->level=random_bipolar(v);
  else if(n->type==FM_LFNoise1)e->grow=(random_bipolar(v)-e->level)/e->remaining;
  else {
   float previous=e->end,len=(float)e->remaining;
   e->end=random_bipolar(v);e->level=e->start;e->start=(e->end+previous)*0.5f;
   e->curve=2*(e->start-e->level-len*e->grow)/(len*len+len);
  }
 }
 out=e->level;
 if(n->type==FM_LFNoise2)e->grow+=e->curve;
 if(n->type!=FM_LFNoise0)e->level+=e->grow;
 e->remaining--;return out;
}
