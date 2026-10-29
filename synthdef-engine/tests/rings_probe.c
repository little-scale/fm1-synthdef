/* GPL-3.0-or-later. Direct model adapter for upstream comparisons. */
#include "../src/fm_render.c"
unsigned rp_words(unsigned model,unsigned compact){return rings_words(model,compact);}
void rp_init(float *mem,unsigned model,float sr,unsigned seed,unsigned compact){rg_init(mem,model,sr,seed,compact);}
void rp_process(float *mem,unsigned model,float sr,const float *in,float *out,float *aux,float pit,const float *p,int trig,int internal,unsigned compact){fm_voice voice;memset(&voice,0,sizeof voice);rg_process(mem,model,sr,in,out,aux,pit,p,trig,internal,&voice,compact);}
