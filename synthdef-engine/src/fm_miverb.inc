/* SPDX-License-Identifier: GPL-3.0-or-later
 * C adaptation of MiVerb (Volker Bohm, GPL-3.0-or-later) and Emilie Gillet's
 * MIT-licensed reverb, fx_engine and cosine oscillator. Original notices and
 * pinned source are retained under reference/mi-ugens/.
 * 32768 packed signed-16 samples, accessed via memcpy to respect float-arena
 * aliasing. All filters/LFOs reuse this node's existing fm_state fields.
 */
typedef struct { float acc,previous;uint32_t pos;unsigned char *mem;fm_voice *voice; } mi_context;
static float mi_read(mi_context *c,unsigned offset) {
 int16_t sample;memcpy(&sample,c->mem+2*((c->pos+offset)&32767u),2);
 return (float)sample*(1.0f/32768.0f);
}
static void mi_write(mi_context *c,unsigned offset,float scale) {
 float x=c->acc;int16_t sample;
 if(!isfinite(x)){c->voice->fault=1;x=0;}
 x=fminf(32767.0f/32768.0f,fmaxf(-1.0f,x));sample=(int16_t)(int32_t)(x*32768.0f);
 memcpy(c->mem+2*((c->pos+offset)&32767u),&sample,2);c->acc*=scale;
}
static void mi_ap(mi_context *c,unsigned base,unsigned length,float gain) {
 c->previous=mi_read(c,base+length-1);c->acc+=c->previous*gain;
 mi_write(c,base,-gain);c->acc+=c->previous;
}
static void mi_interpolate(mi_context *c,unsigned base,float offset,float scale) {
 unsigned whole=(unsigned)offset;float frac=offset-whole;
 float a=mi_read(c,base+whole),b=mi_read(c,base+whole+1);
 c->previous=a+(b-a)*frac;c->acc+=c->previous*scale;
}
static float mi_coefficient(float frequency) {
 /* Both supported LFO rates are < 0.25 at every accepted sample rate. */
 float f=0.25f-frequency;return 16.0f*f*(1.0f-2.0f*f);
}
static float mi_lfo(float *y1,float *y0,float coefficient,int advance) {
 if(advance){float old=*y0;*y0=coefficient*(*y0)-*y1;*y1=old;}
 return *y1+0.5f;
}
static float mi_clip(float x) {
 float x2;if(x < -3.0f)return -1;if(x>3.0f)return 1;
 x2=x*x;return x*((27.0f+x2)/(27.0f+9.0f*x2));
}
static void render_miverb(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,float *mem,unsigned count,int initializing) {
 unsigned j;float time=input(g,b,n,0,0),amount=input(g,b,n,1,0),lp=input(g,b,n,2,0),hp=input(g,b,n,3,0);
 float gain=0.2f,kap=input(g,b,n,5,0);int freeze=input(g,b,n,4,0)>0;
 mi_context c;c.mem=(unsigned char*)mem;c.voice=v;
 if(initializing){
  s->freq=mi_coefficient((0.5f/v->sample_rate)*32.0f);s->width=mi_coefficient((0.3f/v->sample_rate)*32.0f);
  s->phase=s->freq*0.25f;s->last=0.5f;s->a0=s->width*0.25f;s->b1=0.5f;
  b->signal[n->output][0]=b->signal[n->output+1][0]=0;return;
 }
 time=fminf(1.25f,fmaxf(0,time));lp=fminf(1,fmaxf(0,1.01f-lp));
 if(freeze){time=1;lp=1;gain=0;}
 amount=fminf(1,fmaxf(0,amount));kap=fminf(0.95f,fmaxf(0,kap));hp=fminf(1,fmaxf(0,hp));hp=1-hp*hp;
 for(j=0;j<count;j++) {
  float left=input(g,b,n,6,j),right=n->inputs==8?input(g,b,n,7,j):left;
  float lfo1,lfo2,apout,wet;
  s->phase_acc=(s->phase_acc-1)&32767u;c.pos=s->phase_acc;c.acc=0;c.previous=0;
  lfo1=mi_lfo(&s->phase,&s->last,s->freq,(c.pos&31)==0);
  lfo2=mi_lfo(&s->a0,&s->b1,s->width,(c.pos&31)==0);
  c.acc+=(left+right)*gain;
  mi_ap(&c,0,150,kap);mi_ap(&c,151,214,kap);mi_ap(&c,366,319,kap);mi_ap(&c,686,527,kap);
  apout=c.acc;
  mi_interpolate(&c,15314,6261.0f+50.0f*lfo2,time);
  s->previous[0]+=lp*(c.acc-s->previous[0]);c.acc=s->previous[0];
  mi_ap(&c,1214,2182,-kap);mi_ap(&c,3397,2690,kap);mi_write(&c,6088,2);
  s->previous[3]=s->previous[3]*hp+c.acc-s->previous[2];s->previous[2]=c.acc;wet=s->previous[3];
  left+=(wet-left)*amount;
  c.acc=apout;
  mi_interpolate(&c,6088,4460.0f+40.0f*lfo1,time);
  s->previous[1]+=lp*(c.acc-s->previous[1]);c.acc=s->previous[1];
  mi_ap(&c,10590,2525,kap);mi_ap(&c,13116,2197,-kap);mi_write(&c,15314,2);
  s->y2=s->y2*hp+c.acc-s->y1;s->y1=c.acc;wet=s->y2;
  right+=(wet-right)*amount;
  b->signal[n->output][j]=mi_clip(left);b->signal[n->output+1][j]=mi_clip(right);
 }
}
