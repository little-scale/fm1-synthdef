#include "../src/fm_render.c"

void cp_run(unsigned model,float *values,const float *args,float sr,int init,float *out){chaos_state s={0};
s.x0=values[0];
s.y0=values[1];
s.z0=values[2];
s.x1=values[3];
s.xn=values[4];
s.yn=values[5];
s.zn=values[6];
s.xnm1=values[7];
s.ynm1=values[8];
s.znm1=values[9];
s.xnm2=values[10];
s.xnm3=values[11];
s.frac=values[12];
s.c0=values[13];
s.c1=values[14];
s.c2=values[15];
s.c3=values[16];
s.a=values[17];
s.b=values[18];
s.counter=values[19];
s.stable=((int)values[20]);
chaos_unit unit={&s,args,out,sr};switch(model){
case 0:if(init)chaos_HenonN_Ctor(&unit);else chaos_HenonN_next(&unit,1);break;
case 1:if(init)chaos_HenonL_Ctor(&unit);else chaos_HenonL_next(&unit,1);break;
case 2:if(init)chaos_HenonC_Ctor(&unit);else chaos_HenonC_next(&unit,1);break;
case 3:if(init)chaos_CuspN_Ctor(&unit);else chaos_CuspN_next(&unit,1);break;
case 4:if(init)chaos_CuspL_Ctor(&unit);else chaos_CuspL_next(&unit,1);break;
case 5:if(init)chaos_LatoocarfianN_Ctor(&unit);else chaos_LatoocarfianN_next(&unit,1);break;
case 6:if(init)chaos_LatoocarfianL_Ctor(&unit);else chaos_LatoocarfianL_next(&unit,1);break;
case 7:if(init)chaos_LatoocarfianC_Ctor(&unit);else chaos_LatoocarfianC_next(&unit,1);break;
case 8:if(init)chaos_LorenzL_Ctor(&unit);else chaos_LorenzL_next(&unit,1);break;
}
values[0]=(float)s.x0;
values[1]=(float)s.y0;
values[2]=(float)s.z0;
values[3]=(float)s.x1;
values[4]=(float)s.xn;
values[5]=(float)s.yn;
values[6]=(float)s.zn;
values[7]=(float)s.xnm1;
values[8]=(float)s.ynm1;
values[9]=(float)s.znm1;
values[10]=(float)s.xnm2;
values[11]=(float)s.xnm3;
values[12]=(float)s.frac;
values[13]=(float)s.c0;
values[14]=(float)s.c1;
values[15]=(float)s.c2;
values[16]=(float)s.c3;
values[17]=(float)s.a;
values[18]=(float)s.b;
values[19]=(float)s.counter;
values[20]=(float)s.stable;
}
unsigned cp_mulhi(unsigned a,unsigned b){return stochastic_mulhi(a,b);}
float cp_hash(float x){return stochastic_hash(x);}
/* Exercise the actual runtime dispatch from independently supplied states. */
float cp_noise(unsigned kind,float *state,const float *args){
 static fm_voice v;static fm_graph g;static fm_scratch b;
 fm_node n={0};n.type=kind?FM_Logistic:FM_Crackle;n.rate=2;n.inputs=kind?3:1;n.outputs=1;
 for(unsigned i=0;i<n.inputs;i++){g.ref[i].node=-1;g.ref[i].output=i;g.constant[i]=args[i];}
 fm_state *s=&v.state[0];memset(s,0,sizeof *s);s->initialized=1;s->y1=state[0];s->y2=state[1];v.fault=0;
 stochastic_render(&g,&v,&b,&n,s,1,44100,0);state[0]=s->y1;state[1]=s->y2;return b.signal[0][0];
}
