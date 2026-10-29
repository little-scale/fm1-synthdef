/* SPDX-License-Identifier: GPL-3.0-or-later
 * Model-specific C adaptation of Emilie Gillet's MIT-licensed Rings DSP.
 * Original sources/notices: reference/mi-rings/. One internal voice; no heap.
 * Exactly 32 samples per kernel call. Per-instance RNG; explicit trigger. */
#include "fm_rings_tables.inc"
static float rg_clamp(float x,float lo,float hi){return fminf(hi,fmaxf(lo,x));}
static float rg_table(const float *t,float x,float scale){float p=x*scale;if(scale>1&&p>=scale)return t[(unsigned)scale];unsigned i=(unsigned)p;return t[i]+(t[i+1]-t[i])*(p-i);}
static float rg_ratio(float semi){float p=semi+128.0f;unsigned i=(unsigned)p;return rg_lut_pitch_ratio_high[i]*rg_lut_pitch_ratio_low[(unsigned)((p-i)*256.0f)];}
static float rg_random(rg_common *c){c->rng=c->rng*1664525u+1013904223u;return (float)c->rng*(1.0f/4294967296.0f);}
static void rg_svf_set(rg_svf *s,float f,float q,int mode){
 float f2=f*f,g;
 /* Constants retain stmlib's single-precision polynomial evaluation. */
 if(mode==0)g=f*(PI+11.5839453f*f*f);
 else if(mode==1)g=f*(PI+f2*(10.1080465f+55.7873878f*f2));
 else g=f*(PI+f2*(10.3353662f+f2*(40.8206978f+f2*(161.205307f+f2*(86.4620361f+f2*2799.88354f)))));
 s->g=g;s->r=1.0f/q;s->h=1.0f/(1.0f+s->r*g+g*g);
}
static float rg_svf_tick(rg_svf *s,float x,int bp){float h=(x-s->r*s->s1-s->g*s->s1-s->s2)*s->h,b=s->g*h+s->s1;s->s1=s->g*h+b;float l=s->g*b+s->s2;s->s2=s->g*b+l;return bp?b:l;}
static float rg_naive_tick(rg_naive *s,float x){float n=x-s->bp*s->damp;s->lp+=s->f*s->bp;float h=n-s->lp;s->bp+=s->f*h;return h;}
static void rg_cos_init(rg_cos *s,float f){float sign=16;f-=.25f;if(f<0)f=-f;else if(f>.5f)f-=.5f;else sign=-16;s->coefficient=sign*f*(1-2*f);s->y1=s->coefficient*.25f;s->y0=.5f;}
static float rg_cos_next(rg_cos *s){float t=s->y0;s->y0=s->coefficient*s->y0-s->y1;s->y1=t;return t+.5f;}
static float rg_read(const float *m,uint32_t w,unsigned mask,float d){unsigned i=(unsigned)d;float a=m[(w+i)&mask],b=m[(w+i+1)&mask];return a+(b-a)*(d-i);}
/* Full-size strings use a mask; compact strings wrap a bounded index once. */
static unsigned rg_string_index(unsigned index,unsigned capacity){return capacity==2048?(index&2047u):(index>=capacity?index-capacity:index);}
static float rg_string_read(const float *m,uint32_t w,unsigned capacity,float d){unsigned i=(unsigned)d,t=rg_string_index(w+i,capacity);float a=m[t],b=m[rg_string_index(t+1,capacity)];return a+(b-a)*(d-i);}
static float rg_hermite(const float *m,uint32_t w,unsigned capacity,float d){
 if(!isfinite(d)||d<0||d>1e8f||(capacity!=2048&&d>=capacity))return NAN;
 unsigned i=(unsigned)d,t=rg_string_index(w+i,capacity);
 float xm=m[rg_string_index(t+capacity-1,capacity)],x=m[t],x1=m[rg_string_index(t+1,capacity)],x2=m[rg_string_index(t+2,capacity)],c=(x1-xm)*.5f,v=x-x1,z=c+v,a=z+v+(x2-x)*.5f,b=z+a,f=d-i;
 return (((a*f)-b)*f+c)*f+x;
}
static float rg_dc(float x,float *px,float *py,float pole){float y=*py*pole+x-*px;*px=x;*py=y;return y;}
static float rg_soft(float x){return x*(27+x*x)/(27+9*x*x);}
static void rg_init(float *mem,unsigned model,float sr,uint32_t seed,unsigned compact){
 memset(mem,0,rings_words(model,compact)*4u);rg_common c;memset(&c,0,sizeof c);c.rng=seed;c.peak=.5f;c.note=69;
 c.fast=1.0f/(.001f*(sr/32));c.slow=1.0f/(.010f*(sr/32));c.lag=1.0f/(.050f*(sr/32));c.coefficient=c.fast;
 for(unsigned i=0;i<4;i++)c.history[i]=69;
 rg_svf_set(&c.excite,.01f,100,0);rg_svf_set(&c.pluck,.01f,100,0);memcpy(mem,&c,sizeof c);float *state=mem+RG_WORDS(rg_common);
 if(model==1){rg_modal m;memset(&m,0,sizeof m);for(unsigned i=0;i<64;i++)rg_svf_set(&m.filter[i],.01f,100,0);memcpy(state,&m,sizeof m);}
 else if(model==4){rg_fm f;memset(&f,0,sizeof f);f.carrier=f.modulator=220/sr;f.brightness=.5f;
  f.low.f=2*PI*fminf(160/sr,.158f);f.high.f=2*PI*fminf(1600/sr,.158f);f.low.damp=f.high.damp=2;
  f.attack[0]=160/sr;f.attack[1]=sqrtf((160/sr)*(1600/sr));f.attack[2]=sqrtf((1600/sr)*.5f);
  f.decay[0]=sqrtf((160/sr)*(8/sr));f.decay[1]=160/sr;f.decay[2]=sqrtf((1600/sr)*(160/sr));memcpy(state,&f,sizeof f);
 }else{
  state+=256;unsigned strings=model==2||model==5?8:1;const float lf[8]={.5f,.4f,.35f,.23f,.211f,.2f,.171f,0};
  for(unsigned i=0;i<strings;i++){rg_string s;memset(&s,0,sizeof s);s.frequency=220/sr;s.delay=1/s.frequency;rg_svf_set(&s.filter,.01f,100,0);rg_cos_init(&s.lfo,(32/sr)*lf[i]);memcpy(state,&s,sizeof s);state+=RG_WORDS(rg_string)+rings_capacity(model,compact)+(strings==1?1024:0);}
  if(model==6){rg_reverb r;memset(&r,0,sizeof r);rg_cos_init(&r.lfo[0],(.5f/48000)*32);rg_cos_init(&r.lfo[1],(.3f/48000)*32);memcpy(state,&r,sizeof r);}
 }
}
static float rg_note(rg_common *c,float note,int trigger){
 if(fabsf(note-c->note)>.4f||trigger){c->note=note;c->coefficient=c->fast;for(unsigned i=0;i<4;i++)c->history[i]=note;}
 else{float x[4];for(unsigned i=0;i<3;i++)c->history[i]=c->history[i+1];c->history[3]=note;memcpy(x,c->history,sizeof x);
  for(unsigned i=1;i<4;i++)for(unsigned j=i;j>0&&x[j]<x[j-1];j--){float t=x[j];x[j]=x[j-1];x[j-1]=t;}
  c->note+=c->coefficient*((x[1]+x[2])*.5f-c->note);c->coefficient+=c->lag*(c->slow-c->coefficient);
 }return c->note;
}
static void rg_modal_process(float *mem,float *in,float *out,float *aux,float freq,const float *p){
 rg_modal m;memcpy(&m,mem,sizeof m);float stiffness=rg_table(rg_lut_stiffness,p[0],256),harmonic=freq,stretch=1,q=500*rg_table(rg_lut_4_decades,p[2],256);
 float a=1-p[0];a*=a;a*=a;a*=a;float bright=(p[1]*p[1])*(1-.2f*a),loss=bright*(2-bright)*.85f+.15f,rate=p[0]*(2-p[0])*.1f;unsigned modes=0;
 for(unsigned i=0;i<60;i++){float f=harmonic*stretch;if(f>=.49f)f=.49f;else modes=i+1;rg_svf_set(&m.filter[i],f,1+f*q,1);stretch+=stiffness;stiffness*=stiffness<0?.93f:.98f;loss+=rate*(1-loss);harmonic+=freq;q*=loss;}
 float pos=m.position,dp=(p[3]-pos)/32;
 for(unsigned j=0;j<32;j++){pos+=dp;rg_cos osc;rg_cos_init(&osc,pos);float x=in[j]*.125f,odd=0,even=0;for(unsigned i=0;i<modes;){odd+=rg_cos_next(&osc)*rg_svf_tick(&m.filter[i++],x,1);even+=rg_cos_next(&osc)*rg_svf_tick(&m.filter[i++],x,1);}out[j]=odd;aux[j]=even;}
 m.position=pos;memcpy(mem,&m,sizeof m);
}
static float rg_sine_fm(uint32_t phase,float fm){
 /* Modulo phase conversion avoids undefined float-to-uint overflow. */
 if(!isfinite(fm))return NAN;
 float turns=(fm+4.0f)*.125f;turns-=floorf(turns);phase+=((uint32_t)(turns*4294967296.0f))<<3;
 unsigned i=phase>>20;float f=(float)(phase<<12)*(1.0f/4294967296.0f);return rg_lut_sine[i]+(rg_lut_sine[i+1]-rg_lut_sine[i])*f;
}
static void rg_fm_process(float *mem,const float *in,float *out,float *aux,float freq,const float *p,float sr,int trigger){
 rg_fm s;memcpy(&s,mem,sizeof s);if(trigger)s.amp_env=s.bright_env=1;
 float env=p[2]<.9f?1:(1-p[2])*10,ad=1-powf(.001f,1/(.1f*rg_ratio(p[2]*96)*sr)),bd=1-powf(.001f,1/(.1f*rg_ratio(p[2]*84)*sr));
 float mf=fminf(freq*rg_ratio(rg_table(rg_lut_fm_frequency_quantizer,p[0],128)),.5f),feedback=(p[3]-.5f)*2;
 float dc=(freq-s.carrier)/32,dm=(mf-s.modulator)/32,db=(p[1]-s.brightness)/32,df=(feedback-s.feedback)/32;
 for(unsigned j=0;j<32;j++){
  float bands[3];bands[2]=rg_naive_tick(&s.high,in[j]);bands[1]=rg_naive_tick(&s.low,s.high.lp);bands[0]=s.low.lp;float weighted=0,total=0,f=0;
  for(unsigned k=0;k<3;k++){float x=fabsf(bands[k]);s.detector[k]+=(x>s.detector[k]?s.attack[k]:s.decay[k])*(x-s.detector[k]);weighted+=s.detector[k]*f;total+=s.detector[k];f+=.5f;}
  float error=weighted/(total+.001f)-s.centroid;s.centroid+=error*(error>0?.05f:.001f);float be=s.centroid*(2*total*(2-total));
  s.amp_env+=(total>s.amp_env?.05f:ad)*(total-s.amp_env);s.bright_env+=(be>s.bright_env?.01f:bd)*(be-s.bright_env);
  s.brightness+=db;float bright=s.brightness*s.brightness,min=bright<.5f?0:2*bright-1,max=bright<.5f?2*bright:1,target=(min+max*(.5f+env*(s.bright_env-.5f)))*2;
  s.fm+=rg_clamp(target-s.fm,-(.005f+max*.015f),.005f+max*.015f);
  float pf=feedback<0?.5f*feedback*feedback:0;s.modulator+=dm;s.carrier+=dc;
  float mi=s.modulator*(1+s.previous*pf);mi-=floorf(mi);float ci=s.carrier-floorf(s.carrier);
  if(!isfinite(mi)||!isfinite(ci)){for(unsigned k=j;k<32;k++)out[k]=aux[k]=NAN;break;}
  s.modulator_phase+=(uint32_t)(mi*4294967296.0f);s.carrier_phase+=(uint32_t)(ci*4294967296.0f);
  s.feedback+=df;float fb=s.feedback>0?.25f*s.feedback*s.feedback:0,mod=rg_sine_fm(s.modulator_phase,fb*s.previous),car=rg_sine_fm(s.carrier_phase,s.fm*mod);
  s.previous+=.1f*(car-s.previous);float gain=1+env*(s.amp_env-1);s.gain+=(.005f+.045f*s.fm)*(gain-s.gain);out[j]=(car+.5f*mod)*s.gain;aux[j]=.5f*mod*s.gain;
 }memcpy(mem,&s,sizeof s);
}
static void rg_pluck(rg_common *c,float *ring,float *out,float freq,float cutoff,float pos,int trigger){
 if(trigger){float d=(1/freq)*(pos*.9f+.05f);c->pluck_remaining=(uint32_t)d;for(unsigned k=0;k<24&&d>=255;k++)d*=.5f;c->pluck_period=d;c->pluck_gain=(1-pos)*.8f;rg_svf_set(&c->pluck,fminf(cutoff*8,.499f),1,0);}
 for(unsigned j=0;j<32;j++){float x=0;if(c->pluck_remaining){x=2*rg_random(c)-1;c->pluck_remaining--;}out[j]=x+c->pluck_gain*rg_read(ring,c->pluck_write,255,c->pluck_period);ring[c->pluck_write]=out[j];c->pluck_write=(c->pluck_write-1)&255;}
 for(unsigned j=0;j<32;j++)out[j]=rg_svf_tick(&c->pluck,out[j],0);
}
static void rg_string_process(float *mem,const float *in,float *out,float *aux,float freq,float glide,float dispersion,float bright,float damp,float position,float sr,int dispersive,rg_common *common,unsigned capacity){
 rg_string s;memcpy(&s,mem,sizeof s);float *ring=mem+RG_WORDS(rg_string),*stretch=ring+capacity;
 s.frequency+=glide*(freq-s.frequency);float delay=rg_clamp(1/s.frequency,4,(float)capacity-4),ratio=delay*s.frequency;
 if(ratio>=.9999f){s.phase=1;ratio=1;}
 float pos=.5f-.98f*fabsf(position-.5f),dd=(delay-s.delay)/32,dp=(pos-s.position)/32,ds=(dispersion-s.dispersion)/32;
 float lf=damp*(2-damp),rt=.07f*rg_ratio(lf*96)*sr,rtsemi=fmaxf(-120*delay/ratio/rt,-127),coef=rg_ratio(rtsemi),b=bright*bright,nf=rg_ratio((bright-1)*48);
 float cutoff=fminf(24+damp*damp*48+bright*bright*24,84),f=fminf(s.frequency*rg_ratio(cutoff),.499f);
 if(damp>=.95f){float x=20*(damp-.95f);coef+=x*(1-coef);b+=x*(1-b);f+=x*(.4999f-f);cutoff+=x*(128-cutoff);}
 float dfb=(b-s.fir_bright)/32,dfd=(coef-s.fir_damp)/32;rg_svf_set(&s.filter,f,.5f,2);
 float dcomp=(1-rg_table(rg_lut_svf_shift,cutoff,1)-s.compensation)/32;
 for(unsigned j=0;j<32;j++){
  s.phase+=ratio;if(s.phase>1){s.phase-=1;s.delay+=dd;s.position+=dp;float d=s.delay,comb=d*s.position;s.compensation+=dcomp;d*=s.compensation;d-=1;float sample;
   if(dispersive){float noise=(2*rg_random(common)-1)/(0.2f+nf);s.dispersion_noise+=nf*(noise-s.dispersion_noise);s.dispersion+=ds;
    float point=s.dispersion<=0?0:s.dispersion*(2-s.dispersion)*.475f,amount=s.dispersion>.75f?4*(s.dispersion-.75f):0,curve=s.dispersion<0?-s.dispersion:0,ac=curve;
    amount=amount*amount*.025f;curve=curve*curve*.01f;float apgain=-.618f*s.dispersion/(.15f+fabsf(s.dispersion));
    float fm=1;fm+=s.dispersion_noise*amount;fm-=s.bridge*curve;d*=fm;float ap=d*point,main=d-ap;
    if(ap>=4&&main>=4){sample=rg_hermite(ring,s.write,capacity,main);if(!isfinite(ap)||ap>1e8f)sample=NAN;else{float read=stretch[(s.stretch_write+(unsigned)ap)&1023],write=sample+apgain*read;stretch[s.stretch_write]=write;s.stretch_write=(s.stretch_write-1)&1023;sample=-write*apgain+read;}}
    else sample=rg_hermite(ring,s.write,capacity,d);
    float sac=rg_dc(sample,&s.dc_x,&s.dc_y,1-20/sr);sample+=ac*(sac-sample);float value=fabsf(sample)-.025f,sign=sample>0?1:-1.5f;s.bridge=(fabsf(value)+value)*sign;
   }else sample=rg_hermite(ring,s.write,capacity,d);
   sample+=in[j];float filtered=s.fir_damp*((1+s.fir_bright)*.5f*s.fir_x+(1-s.fir_bright)*.25f*(sample+s.fir_xx));s.fir_xx=s.fir_x;s.fir_x=sample;s.fir_bright+=dfb;s.fir_damp+=dfd;sample=rg_svf_tick(&s.filter,filtered,0);
   ring[s.write]=sample;s.write=s.write?s.write-1:capacity-1;s.out[1]=s.out[0];s.aux[1]=s.aux[0];s.out[0]=sample;s.aux[0]=rg_string_read(ring,s.write,capacity,comb);
  }
  out[j]+=s.out[1]+(s.out[0]-s.out[1])*s.phase;aux[j]+=s.aux[1]+(s.aux[0]-s.aux[1])*s.phase;
 }memcpy(mem,&s,sizeof s);
}
static float rg_squash(float x){if(x<.5f){x*=2;x*=x;x*=x;x*=x;x*=x;return x*.5f;}x=2-2*x;x*=x;x*=x;x*=x;x*=x;return 1-.5f*x;}
static void rg_sympathetic(float note,float structure,unsigned model,float *f){

 if(model==5){unsigned chord=(unsigned)floorf(structure*10+.5f);for(unsigned i=0;i<8;i++)f[i]=note+rg_chords[chord][i];return;}
 float notes[9]={12,note-12,note-7.01955f,note,note+7.01955f,note+12,note+19.01955f,note+24,note+24};const float detune[4]={.013f,.011f,.007f,.017f};
 for(unsigned i=0;i<5;i++){float p=3;if(i){p=structure*7;structure+=(1-structure)*.2f;}unsigned k=(unsigned)p;float frac=rg_squash(p-k);f[i]=notes[k]+(notes[k+1]-notes[k])*frac;if(i+5<8)f[i+5]=f[i]+detune[i&3];}
}
/* Unused padding removed from the reverb ring; delay lengths and 16-bit
 * quantisation are unchanged. A multiple of 32 preserves LFO tick phase. */
typedef struct {float acc,previous;uint32_t pos;unsigned char *mem;fm_voice *voice;} rg_fx_context;
static unsigned rg_fx_address(unsigned index){if(index>=RG_REVERB_SAMPLES)index-=RG_REVERB_SAMPLES;return 2*index;}
static float rg_fx_read(rg_fx_context *c,unsigned offset) {
 unsigned at=rg_fx_address(c->pos+offset);
 unsigned bits=c->mem[at]|((unsigned)c->mem[at+1]<<8);
 int32_t sample=bits>=32768u?(int32_t)bits-65536:(int32_t)bits;
 return (float)sample*(1.0f/32768.0f);
}
static void rg_fx_write(rg_fx_context *c,unsigned offset,float scale) {
 float x=c->acc;uint16_t sample;unsigned at=rg_fx_address(c->pos+offset);
 if(!isfinite(x)){c->voice->fault=1;x=0;}
 x=fminf(32767.0f/32768.0f,fmaxf(-1.0f,x));sample=(uint16_t)(int16_t)(int32_t)(x*32768.0f);
 c->mem[at]=(unsigned char)sample;c->mem[at+1]=(unsigned char)(sample>>8);c->acc*=scale;
}
static void rg_fx_ap(rg_fx_context *c,unsigned base,unsigned length,float gain) {
 c->previous=rg_fx_read(c,base+length-1);c->acc+=c->previous*gain;
 rg_fx_write(c,base,-gain);c->acc+=c->previous;
}
static void rg_fx_interpolate(rg_fx_context *c,unsigned base,float offset,float scale) {
 unsigned whole=(unsigned)offset;float frac=offset-whole;
 float a=rg_fx_read(c,base+whole),b=rg_fx_read(c,base+whole+1);
 c->previous=a+(b-a)*frac;c->acc+=c->previous*scale;
}
static void rg_reverb_process(float *mem,float *out,float *aux,const float *p,fm_voice *voice){
 rg_reverb r;memcpy(&r,mem,sizeof r);rg_fx_context c;c.mem=(unsigned char*)(mem+RG_WORDS(rg_reverb));c.voice=voice;float amount=.1f+p[2]*.5f,time=.35f+.63f*p[2],lp=.3f+p[1]*.6f,kap=.625f;
 for(unsigned j=0;j<32;j++){
  float left=out[j]*p[3]+(1-p[3])*aux[j],right=aux[j]*p[3]+(1-p[3])*out[j];r.write=r.write?r.write-1:RG_REVERB_SAMPLES-1;c.pos=r.write;c.acc=0;c.previous=0;
  if(!(c.pos&31)){rg_cos_next(&r.lfo[0]);rg_cos_next(&r.lfo[1]);}float lf1=r.lfo[0].y1+.5f,lf2=r.lfo[1].y1+.5f;
  c.acc+=(left+right)*.2f;rg_fx_ap(&c,0,150,kap);rg_fx_ap(&c,151,214,kap);rg_fx_ap(&c,366,319,kap);rg_fx_ap(&c,686,527,kap);float apout=c.acc;
  rg_fx_interpolate(&c,15314,6261+50*lf2,time);r.lp[0]+=lp*(c.acc-r.lp[0]);c.acc=r.lp[0];rg_fx_ap(&c,1214,2182,-kap);rg_fx_ap(&c,3397,2690,kap);rg_fx_write(&c,6088,2);left+=(c.acc-left)*amount;
  c.acc=apout;rg_fx_interpolate(&c,6088,4460+40*lf1,time);r.lp[1]+=lp*(c.acc-r.lp[1]);c.acc=r.lp[1];rg_fx_ap(&c,10590,2525,kap);rg_fx_ap(&c,13116,2197,-kap);rg_fx_write(&c,15314,2);right+=(c.acc-right)*amount;out[j]=left;aux[j]=-right;
 }memcpy(mem,&r,sizeof r);
}
static __attribute__((noinline)) void rg_process(float *mem,unsigned model,float sr,const float *in,float *out,float *aux,float pit,const float *p,int trigger,int internal,fm_voice *voice,unsigned compact){
 rg_common c;memcpy(&c,mem,sizeof c);float note=rg_note(&c,pit,trigger)+12,freq=rg_ratio(note-69)*(440/sr),cut=p[1]*(2-p[1]),fc=fminf(internal?freq*rg_ratio((cut-.5f)*96):.4f*rg_ratio((cut-1)*108),.499f);
 rg_svf_set(&c.excite,fc,internal?1.5f:.8f,0);float input[32];memcpy(input,in,sizeof input);float *state=mem+RG_WORDS(rg_common);
 if(model==1){if(internal&&trigger)input[0]+=.25f*rg_ratio(fc*fc*24)/fc;for(unsigned j=0;j<32;j++)input[j]=rg_svf_tick(&c.excite,input[j],0);rg_modal_process(state,input,out,aux,freq,p);}
 else if(model==4)rg_fm_process(state,input,out,aux,freq,p,sr,internal&&trigger);
 else{
  unsigned strings=model==2||model==5?8:1;float f[8];if(strings==8){rg_sympathetic(note,p[0],model,f);for(unsigned i=0;i<8;i++)f[i]=rg_ratio(f[i]-69)*(440/sr);}else f[0]=freq;
  float gain=1/sqrtf((float)strings*2),burst[32],sympathetic[32];for(unsigned j=0;j<32;j++)input[j]=rg_svf_tick(&c.excite,input[j]*gain,0);
  if(internal){rg_pluck(&c,state,burst,freq,fc,p[3],trigger);for(unsigned j=0;j<32;j++)input[j]+=burst[j];}
  for(unsigned j=0;j<32;j++){input[j]=rg_dc(input[j],&c.dc_x,&c.dc_y,1-10/sr);out[j]=aux[j]=0;sympathetic[j]=0;}
  state+=256;float dispersion=p[0]<.24f?(p[0]-.24f)*4.166f:(p[0]>.26f?(p[0]-.26f)*1.35135f:0);
  for(unsigned i=0;i<strings;i++){
   rg_string s;memcpy(&s,state,sizeof s);float lfo=rg_cos_next(&s.lfo);memcpy(state,&s,sizeof s);
   float b=p[1],d=p[2],pos=p[3],glide=1;const float *x=input;if(model==6)d*=2-d;
   if(i&&internal){b*=2-b;b*=2-b;d=.7f+p[2]*.27f;pos=p[3]+lfo*((.5f-fabsf(.5f-p[3]))*.9f);glide=rg_ratio((b-1)*36);x=sympathetic;}
   d+=((float)i/(float)strings)*(.95f-d);rg_string_process(state,x,out,aux,f[i],glide,dispersion,b,d,pos,sr,strings==1,&c,rings_capacity(model,compact));
   if(!i)for(unsigned j=0;j<32;j++)sympathetic[j]=(.2f/(float)strings)*(out[j]-aux[j]);
   state+=RG_WORDS(rg_string)+rings_capacity(model,compact)+(strings==1?1024:0);
  }
  if(model==6)rg_reverb_process(state,out,aux,p,voice);
 }
 const float gains[6]={1.4f,1,1.4f,.7f,1,1.4f};
 for(unsigned j=0;j<32;j++){float l=out[j]*gains[model-1],r=aux[j]*gains[model-1],peak=fmaxf(fmaxf(fabsf(l),fabsf(r)),fabsf(r-l));c.peak+=(peak>c.peak?.05f:.00002f)*(peak-c.peak);float gain=c.peak<=1?1:1/c.peak;out[j]=rg_soft(l*gain*.8f);aux[j]=rg_soft(r*gain*.8f);}
 memcpy(mem,&c,sizeof c);
}
static __attribute__((noinline)) void rings_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,int initializing){
 if(v->sample_rate<32000){v->fault=1;return;}
 unsigned model=n->type-FM_MiRings1+1,compact=(unsigned)g->constant[g->ref[n->input+9].output];float *mem;
 if(s->total!=rings_words(model,compact)||s->phase_offset>v->delay_capacity||s->total>v->delay_capacity-s->phase_offset||!v->delay_memory){v->fault=1;return;}
 mem=v->delay_memory+s->phase_offset;
 if(initializing){rg_init(mem,model,v->sample_rate,random_word(v),compact);b->signal[n->output][0]=b->signal[n->output+1][0]=0;s->initialized=1;return;}
 float p[4];for(unsigned i=0;i<4;i++)p[i]=rg_clamp(input(g,b,n,3+i,0),0,1);float pit=rg_clamp(input(g,b,n,2,0),0,114);
 int trig=input(g,b,n,1,0)>0,internal=input_rate(g,n,0)!=2||input(g,b,n,7,0)>0,bypass=input(g,b,n,8,0)>0;
 int fire=trig&&!s->last;s->last=(float)trig;
 for(unsigned offset=0;offset<FM_BLOCK;offset+=32){float x[32],l[32],r[32];for(unsigned j=0;j<32;j++)x[j]=input_rate(g,n,0)==2?input(g,b,n,0,offset+j):0;
  if(bypass){memcpy(l,x,sizeof l);memcpy(r,x,sizeof r);}else rg_process(mem,model,v->sample_rate,x,l,r,pit,p,fire&&offset==0,internal,v,compact);
  for(unsigned j=0;j<32;j++){checked_signal(v,&b->signal[n->output][offset+j],l[j]);checked_signal(v,&b->signal[n->output+1][offset+j],r[j]);}
 }
}
