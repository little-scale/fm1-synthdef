#include "../src/fm_render.c"
void xp_run(unsigned type,fm_state *state,const float *args,float sr,int init,float *out){
 static fm_graph g;static fm_voice v;static fm_scratch b;fm_node n={0};n.type=type;n.rate=2;n.outputs=1;n.inputs=type==FM_SinOscFB?2:4;
 for(unsigned k=0;k<n.inputs;k++){g.ref[k].node=-1;g.ref[k].output=k;g.constant[k]=args[k];}
 v.fault=0;expansion_render(&g,&v,&b,&n,state,1,sr,init);*out=b.signal[0][0];
}
