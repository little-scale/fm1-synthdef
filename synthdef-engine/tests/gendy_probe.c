/* GPL-3.0-or-later: direct kernel adapter for identical-state oracle. */
#include "../src/fm_render.c"
#include "gendy_probe.h"
float gp_distribution(int which,float a,float f){a=fminf(1,fmaxf(.0001f,a));return gendy_distribution_ready(which,a,f,gendy_constant(which,a));}
void gp_step(gendy_probe *p,float sr,const float *params,unsigned count){
 unsigned i;for(i=0;i<p->g.node[0].inputs;i++)p->g.constant[i]=params[i];
 gendy_render(&p->g,&p->v,&p->b,&p->g.node[0],&p->v.state[0],count,sr,0);
}
void gp_init(gendy_probe *p,int type,unsigned cp,float sr,const float *params){
 unsigned i;memset(p,0,sizeof *p);p->g.node[0].type=(uint8_t)(FM_Gendy1+type-1);p->g.node[0].rate=2;
 p->g.node[0].inputs=type==1?10:type==2?12:9;
 for(i=0;i<p->g.node[0].inputs;i++){p->g.ref[i].node=-1;p->g.ref[i].output=(uint16_t)i;p->g.constant[i]=params[i];}
 p->v.delay_memory=p->memory;p->v.delay_capacity=type==1?cp*2:type==2?cp*4:cp*4+2;
 p->v.state[0].total=p->v.delay_capacity;random_seed(&p->v,12345);
 gendy_render(&p->g,&p->v,&p->b,&p->g.node[0],&p->v.state[0],1,sr,1);
}
