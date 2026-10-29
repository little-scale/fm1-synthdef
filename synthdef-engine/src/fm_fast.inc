/* SPDX-License-Identifier: GPL-3.0-or-later
 * Per-node input binding avoids decoding graph references for every sample.
 */
typedef struct {const float *p;float previous,delta;unsigned rate;} fm_bound_input;
static fm_bound_input bind_input(const fm_graph *g,const fm_scratch *b,const fm_node *n,unsigned k,float previous){
 const fm_ref *r=&g->ref[n->input+k];fm_bound_input x;
 x.p=r->node<0?&g->constant[r->output]:b->signal[g->node[r->node].output+r->output];
 x.rate=r->node<0?0:g->node[r->node].rate;x.previous=previous;x.delta=x.p[0]-previous;if(x.rate==1 && x.delta==0)x.rate=0;return x;
}
static float bound_value(const fm_bound_input *x,unsigned j,float scale){
 return x->rate==2?x->p[j]:x->rate==1 && scale!=1?x->previous+(x->delta*(float)j)*scale:x->p[0];
}
static void checked_signal(fm_voice *v,float *out,float x){
 /* IEEE-754 positive float bit order also rejects infinities and NaNs.
    One integer range check avoids two FPU predicates in every sample loop. */
 union {float f;uint32_t u;} bits,limit;bits.f=x;limit.f=1e12f;
 if((bits.u&0x7fffffffu)>limit.u){v->fault=1;x=0;}*out=x;
}
static __attribute__((noinline)) void fast_arithmetic(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count){
 fm_bound_input x[4];unsigned k,j;float scale=count>1?1.0f/FM_BLOCK:1;float *out=b->signal[n->output];int op=n->type==FM_BinaryOpUGen?n->special:n->type==FM_MulAdd?-1:n->type==FM_Sum3?-2:-3;
 for(k=0;k<n->inputs;k++){float initial=input(g,b,n,k,0);if(!s->initialized)s->previous[k]=initial;x[k]=bind_input(g,b,n,k,s->previous[k]);}
 /* Resolve scalar/audio combinations outside the inner loop. Control ramps
    retain the general path, including their exact interpolation order. */
 if(n->type==FM_BinaryOpUGen && x[0].rate!=1 && x[1].rate!=1){
  const float *a=x[0].p,*bval=x[1].p;
  #define BINARY_LOOP(expr) for(j=0;j<count;j++)checked_signal(v,out+j,(expr))
  #define BINARY_CASE(OP) \
   if(x[0].rate==2 && x[1].rate==2){BINARY_LOOP(a[j] OP bval[j]);} \
   else if(x[0].rate==2){float z=bval[0];BINARY_LOOP(a[j] OP z);} \
   else if(x[1].rate==2){float z=a[0];BINARY_LOOP(z OP bval[j]);} \
   else {float z=a[0] OP bval[0];checked_signal(v,&z,z);for(j=0;j<count;j++)out[j]=z;}
  switch(op){
  case 0:BINARY_CASE(+);break;
  case 1:BINARY_CASE(-);break;
  case 2:BINARY_CASE(*);break;
  case 4:BINARY_CASE(/);break;
  default:goto general_arithmetic;
  }
  #undef BINARY_CASE
  #undef BINARY_LOOP
  goto arithmetic_done;
 }
 general_arithmetic:
 if(n->type==FM_BinaryOpUGen && (op==3 || op==5 || (op>=17 && op<=21) || op==39 || op==42 || op==43 || op==44 || op==45)){
  /* SC accumulates control ramps; discontinuous operators must retain the
     exact accumulated endpoint rather than recomputing a mathematically
     equivalent interpolation, which can cross quantization boundaries. */
  float av=x[0].rate==1 && count>1?x[0].previous:x[0].p[0];
  float bv=x[1].rate==1 && count>1?x[1].previous:x[1].p[0];
  float da=x[0].rate==1 && count>1?x[0].delta*scale:0,db=x[1].rate==1 && count>1?x[1].delta*scale:0;
  for(j=0;j<count;j++){checked_signal(v,out+j,binary(op,x[0].rate==2?x[0].p[j]:av,x[1].rate==2?x[1].p[j]:bv));av+=da;bv+=db;}
  s->previous[0]=x[0].rate==1 && count>1?av:x[0].p[0];s->previous[1]=x[1].rate==1 && count>1?bv:x[1].p[0];s->initialized=1;return;
 }
 #define A(k) bound_value(&x[k],j,scale)
 #define RUN(expr) for(j=0;j<count;j++)checked_signal(v,out+j,(expr))
 switch(op){
 case 0:RUN(A(0)+A(1));break;
 case 1:RUN(A(0)-A(1));break;
 case 2:RUN(A(0)*A(1));break;
 case 4:RUN(A(0)/A(1));break;
 case -1:RUN(A(0)*A(1)+A(2));break;
 case -2:RUN(A(0)+A(1)+A(2));break;
 case -3:RUN(A(0)+A(1)+A(2)+A(3));break;
 default:RUN(binary(op,A(0),A(1)));break;
 }
 #undef RUN
 #undef A
 arithmetic_done:
 for(k=0;k<n->inputs;k++)s->previous[k]=x[k].p[0];s->initialized=1;
}
/* Exact truncated phase ratio without software double helpers. The device
   has a single-precision FPU. Mantissas and a bounded binary division reproduce
   SC's double control-frequency increment, including integer boundaries. */
static uint32_t sine_control_step(float frequency,float sr){
 float f=fmodf(frequency,sr);uint32_t a,b,rem,q;int shift,i,negative=f<0;
 f=fabsf(f);memcpy(&a,&f,4);memcpy(&b,&sr,4);
 if(!f || (a>>23)==0)return 0;
 shift=(int)(a>>23)-(int)(b>>23)+29;if(shift<0)return 0;
 a=(a&0x7fffffu)|0x800000u;b=(b&0x7fffffu)|0x800000u;
 q=a/b;rem=a%b;
 for(i=0;i<shift;i++){rem*=2;q*=2;if(rem>=b){rem-=b;q++;}}
 return negative?0u-q:q;
}
static __attribute__((noinline)) void fast_sine(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int initializing){
 fm_bound_input f=bind_input(g,b,n,0,0),p=bind_input(g,b,n,1,s->last);
 unsigned j;uint32_t step=0,offset=0;int fixed_phase=p.rate!=2 && (p.rate==0 || p.p[0]==s->last);
 if(!s->initialized){s->last=p.p[0];p.previous=s->last;p.delta=0;fixed_phase=p.rate!=2;}
 if(f.rate!=2)step=sine_control_step(f.p[0],sr);
 if(fixed_phase)offset=phase_word(p.p[0]);
 if(fixed_phase && f.rate!=2){
  uint32_t phase=s->phase_acc;
  for(j=0;j<count;j++){b->signal[n->output][j]=table_sine((phase+offset)&0x1fffffffu);phase=(phase+step)&0x1fffffffu;}
  s->phase_acc=phase;
  goto sine_done;
 }
 for(j=0;j<count;j++){
  float phase=p.rate==1?p.previous+p.delta*(float)j/count:p.rate==2?p.p[j]:p.p[0];
  if(!isfinite(phase)){v->fault=1;phase=0;}
  if(!fixed_phase)offset=phase_word(phase);
  b->signal[n->output][j]=table_sine((s->phase_acc+offset)&0x1fffffffu);
  if(f.rate==2)step=(uint32_t)(int32_t)(fmodf(f.p[j],sr)*(536870912.0f/sr));
  s->phase_acc=(s->phase_acc+step)&0x1fffffffu;
 }
 sine_done:
 s->last=p.p[0];s->initialized=1;if(initializing)s->phase_acc=0;
}
static __attribute__((noinline)) void fast_pan(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count){
 fm_bound_input audio=bind_input(g,b,n,0,0);unsigned j;
 float scale=count>1?1.0f/FM_BLOCK:1;
 float pos=fminf(1,fmaxf(-1,input(g,b,n,1,0))),level=input(g,b,n,2,0),left=s->a0,right=s->b1;
 if(!s->initialized || pos!=s->freq || level!=s->width){
  float index=floorf(1024*pos+1024.5f);
  left=sinf((2048-index)*(PI/4096.0f))*level;right=sinf(index*(PI/4096.0f))*level;
 }
 if(!s->initialized){s->a0=left;s->b1=right;}
 if(left==s->a0 && right==s->b1){
  for(j=0;j<count;j++){
   float a=audio.rate==2?audio.p[j]:audio.p[0];
   checked_signal(v,&b->signal[n->output][j],a*left);checked_signal(v,&b->signal[n->output+1][j],a*right);
  }
  goto pan_done;
 }
 for(j=0;j<count;j++){
  float a=audio.rate==2?audio.p[j]:audio.p[0],t=(float)j*scale;
  checked_signal(v,&b->signal[n->output][j],a*(s->a0+(left-s->a0)*t));
  checked_signal(v,&b->signal[n->output+1][j],a*(s->b1+(right-s->b1)*t));
 }
 pan_done:
 s->a0=left;s->b1=right;s->freq=pos;s->width=level;s->initialized=1;
}
