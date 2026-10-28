/* GPL-3.0-or-later. Test adapter for unmodified pinned MIT-licensed DSP. */
#include <cstring>
#include "reverb.h"
struct Oracle {Reverb dsp;uint16_t buffer[32768];};
extern "C" {
void *mi_create(float sr) {
 Oracle *p=new Oracle;std::memset(p,0,sizeof(*p));p->dsp.Init(p->buffer,sr);return p;
}
void mi_destroy(void *handle){delete static_cast<Oracle*>(handle);}
void mi_process(void *handle,const float *in,float *out,const float *parameters){
 Oracle *p=static_cast<Oracle*>(handle);float l[64],r[64];
 float time=parameters[0],wet=parameters[1],damp=parameters[2],hp=parameters[3],diff=parameters[5];
 bool freeze=parameters[4]>0;
 CONSTRAIN(time,0.f,1.25f);damp=1.01f-damp;CONSTRAIN(damp,0.f,1.f);
 CONSTRAIN(wet,0.f,1.f);CONSTRAIN(hp,0.f,1.f);hp*=hp;hp=1.f-hp;CONSTRAIN(diff,0.f,0.95f);
 p->dsp.set_time(freeze?1.f:time);p->dsp.set_input_gain(freeze?0.f:0.2f);p->dsp.set_lp(freeze?1.f:damp);
 p->dsp.set_amount(wet);p->dsp.set_hp(hp);p->dsp.set_diffusion(diff);
 for(unsigned i=0;i<64;i++){l[i]=in[2*i];r[i]=in[2*i+1];}
 p->dsp.Process(l,r,64);
 for(unsigned i=0;i<128;i++){
  float x=(i&1)?r[i/2]:l[i/2];
  if(x < -3.f)x=-1.f;else if(x>3.f)x=1.f;else {float x2=x*x;x*=(27.f+x2)/(27.f+9.f*x2);}
  out[i]=x;
 }
}
}
