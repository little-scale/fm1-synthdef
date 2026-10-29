/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from SuperCollider 3.14.1 GendynUGens.cpp, Nick Collins
 * (GPL-2.0-or-later); Xenakis/Serra stochastic synthesis algorithms.
 * Bounded caller-owned breakpoint arrays; no heap or software doubles.
 * Compensated pairs preserve the upstream double phase accumulator.
 */
static float gendy_atan(float x) {
 /* Positive argument only (distribution parameter is clamped).
  * Reduce to |x| <= tan(pi/8), then the alternating Taylor series. */
 float offset=0,sign=1,z,p;
 if(x>1){x=1/x;offset=1.5707963267948966f;sign=-1;}
 if(x>0.414213562373095f){x=(x-1)/(x+1);offset+=sign*0.7853981633974483f;}
 z=x*x;p=1.0f/21;
 p=-0.05263157894736842f+z*p;
 p=0.058823529411764705f+z*p;
 p=-0.06666666666666667f+z*p;
 p=0.07692307692307693f+z*p;
 p=-0.09090909090909091f+z*p;
 p=0.1111111111111111f+z*p;
 p=-0.14285714285714285f+z*p;
 p=0.2f+z*p;
 p=-0.3333333333333333f+z*p;
 p=1.0f+z*p;
 return offset+sign*x*p;
}
static float gendy_constant(int which,float a) {
 switch(which){
 case 1:return gendy_atan(10*a);
 case 2:{float c=0.5f+0.499f*a;return logf((1-c)/c);}
 case 3:return tanf(1.5692255f*a);
 case 4:return sinf(1.5707963f*a);
 case 5:return logf(1-0.999f*a);
 default:return 0;
 }
}
static float gendy_distribution_ready(int which,float a,float f,float c) {
 float t;
 switch(which){
 case 1:t=(1/a)*tanf(c*(2*f-1));return t*0.1f;
 case 2:f=((f-0.5f)*0.998f*a)+0.5f;return logf((1-f)/f)/c;
 case 3:t=tanf(1.5692255f*a*f)/c;t=logf(t*0.999f+0.001f)*(-0.1447648f);return 2*t-1;
 case 4:return sinf(3.14159265358979323846f*(f-0.5f)*a)/c;
 case 5:t=logf(1-f*0.999f*a)/c;return 2*t-1;
 case 6:return 2*a-1;
 default:return 2*f-1;
 }
}
static float gendy_mirror(float lo,float hi,float x) {
 /* Preserve SC's specific reflection rules, including its lower-bound
  * convention. This is intentionally not the generic Fold UGen. */
 if(x>hi || x<lo){float range=hi-lo;if(x<lo)x=(2*hi-lo)-x;x=fmodf(x-hi,2*range);x=x<range?hi-x:x-range;}
 return x;
}
static void gendy_pair_add(float *hi,float *lo,float x) {
 float sum=*hi+x,back=sum-*hi,low=((*hi-(sum-back))+(x-back))+*lo;
 *hi=sum+low;*lo=low-(*hi-sum);
}
static __attribute__((noinline)) unsigned gendy_reciprocal_floor(float x) {
 /* Correct float division when it rounded up to an integer: SC truncates
  * the DOUBLE reciprocal. x is a finite duration >= 1/192000.
  * Keep 64-bit multiplies out of line: pi32v2 multiply/load erratum. */
 uint32_t bits,m;unsigned q=(unsigned)(1/x),exponent;uint64_t numerator;
 memcpy(&bits,&x,4);exponent=(bits>>23)&255;m=(bits&0x7fffff)|0x800000;
 if(exponent>150 || exponent<104)return q;
 numerator=(uint64_t)1<<(150-exponent);
 if((uint64_t)q*m>numerator)q--;
 else if((uint64_t)(q+1)*m<=numerator)q++;
 return q;
}
static void gendy_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int init) {
 float p[12],*ma,*md,*extra,*phases;unsigned k,cp,cpinput=n->type==FM_Gendy3?7:8;
 int wa,wd,num,index=(int)s->phase_acc;float ca,cd,amp=s->y1,next=s->y2,speed=s->a0;
 if(!s->total || s->phase_offset>v->delay_capacity || s->total>v->delay_capacity-s->phase_offset || !v->delay_memory){v->fault=1;return;}
 for(k=0;k<n->inputs;k++){p[k]=input(g,b,n,k,0);if(!isfinite(p[k]) || fabsf(p[k])>1000000){v->fault=1;return;}}
 cp=(unsigned)p[cpinput];ma=v->delay_memory+s->phase_offset;md=ma+cp;extra=md+cp;phases=n->type==FM_Gendy3?extra+cp+1:NULL;
 wa=(int)p[0];wd=(int)p[1];num=(int)p[cpinput+1];if(num<1 || num>(int)cp)num=(int)cp;
 if(p[4]<0 || (n->type!=FM_Gendy3 && p[5]<0)){v->fault=1;return;}
 if(init){
  s->phase=1;s->last=0;s->a0=100;s->width=1;
  for(k=0;k<cp;k++){
   ma[k]=2*random_uniform(v)-1;md[k]=random_uniform(v);
   if(n->type==FM_Gendy2){extra[k]=2*random_uniform(v)-1;extra[cp+k]=2*random_uniform(v)-1;}
   if(n->type==FM_Gendy3){extra[k]=2*random_uniform(v)-1;phases[k]=1;}
  }
  if(n->type==FM_Gendy3)ma[0]=0;
  b->signal[n->output][0]=0;return;
 }
 p[2]=fminf(1,fmaxf(0.0001f,p[2]));p[3]=fminf(1,fmaxf(0.0001f,p[3]));
 ca=gendy_constant(wa,p[2]);cd=gendy_constant(wd,p[3]);
 for(k=0;k<count;k++){
  float out;
  if(lf_ge(s,1)){
   lf_advance(s,-1);
   if(n->type==FM_Gendy3){
    float sum=0;int j,active=0;
    for(j=0;j<num;j++){
     if(j>0)ma[j]=gendy_mirror(-1,1,ma[j]+p[5]*gendy_distribution_ready(wa,p[2],random_uniform(v),ca));
     md[j]=gendy_mirror(0.01f,1,md[j]+p[6]*gendy_distribution_ready(wd,p[3],random_uniform(v),cd));sum+=md[j];
    }
    if(!isfinite(sum) || sum<=0){v->fault=1;return;}sum=1/sum;speed=p[4]*(1/sr);
    for(j=0;j<num;j++){float dur=md[j]*sum;if(dur>=1/sr){extra[active]=ma[j];phases[active]=dur;active++;}}
    extra[active]=0;phases[active]=2;s->dice[0]=(uint32_t)active;
    s->previous[2]=0;s->previous[3]=0;next=extra[0];index=-1;
   }else{
    float rate,lehmer=0;
    index=(index+1)%num;
    if(n->type==FM_Gendy2)lehmer=fmodf(amp*p[10]+p[11],1);
    amp=next;
    if(n->type==FM_Gendy1){
     next=ma[index]+p[6]*gendy_distribution_ready(wa,p[2],random_uniform(v),ca);
     if(next>1 || next< -1){if(next<0)next+=4;next=fmodf(next,4);if(next>1 && next<3)next=2-next;else if(next>1)next-=4;}
     rate=md[index]+p[7]*gendy_distribution_ready(wd,p[3],random_uniform(v),cd);
     if(rate>1 || rate<0){if(rate<0)rate+=2;rate=fmodf(rate,2);rate=2-rate;}
    }else{
     extra[index]=gendy_mirror(-1,1,extra[index]+gendy_distribution_ready(wa,p[2],fabsf(lehmer),ca));
     next=gendy_mirror(-1,1,ma[index]+p[6]*extra[index]);
     extra[cp+index]=gendy_mirror(-1,1,extra[cp+index]+gendy_distribution_ready(wd,p[3],random_uniform(v),cd));
     rate=gendy_mirror(0,1,md[index]+p[7]*extra[cp+index]);
    }
    ma[index]=next;md[index]=rate;speed=(p[4]+(p[5]-p[4])*rate)*(1/sr);speed*=num;
   }
  }
  if(n->type==FM_Gendy3){
   float diff=(s->phase-s->previous[2])+(s->last-s->previous[3]);
   if(diff>=0){
    index++;if(index<0 || index>=(int)s->dice[0]){v->fault=1;return;}
    amp=next;s->previous[0]=s->previous[2];s->previous[1]=s->previous[3];
    gendy_pair_add(&s->previous[2],&s->previous[3],phases[index]);next=extra[index+1];
    /* Upstream deliberately truncates the interpolation multiplier to int. */
    if(phases[index]<=0 || 1/phases[index]>1000000){v->fault=1;return;}
    s->width=(float)gendy_reciprocal_floor(phases[index]);
   }
   {float interp=((s->phase-s->previous[0])+(s->last-s->previous[1]))*s->width;out=(1-interp)*amp+interp*next;}
  }else out=(1-s->phase)*amp+s->phase*next+s->last*(next-amp);
  checked_signal(v,&b->signal[n->output][k],out);
  lf_advance(s,speed);if(!isfinite(s->phase) || fabsf(s->phase)>1000000){v->fault=1;return;}
 }
 s->phase_acc=(uint32_t)index;s->y1=amp;s->y2=next;s->a0=speed;
}
