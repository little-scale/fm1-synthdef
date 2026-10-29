/* GPL-3.0-or-later. Unmodified pinned SC kernels, synchronized from the
 * port BEFORE each sample to test chaotic feedback without trajectory drift.
 * Long unsynchronized renders are separately checked by the NRT suite. */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <memory>
#include "../reference/GendynUGens.cpp"
#include "gendy_probe.h"
static void *allocate(World *,size_t n){return std::calloc(1,n);}
static void release(World *,void *p){std::free(p);}
static double phase(float hi,float lo){return (double)hi+(double)lo;}
int main(){
 InterfaceTable table={};table.fRTAlloc=allocate;table.fRTFree=release;ft=&table;
 RGen rng;Graph graph={};graph.mRGen=&rng;Rate rate={};
 float params[12],*in[12],output[64],*out[]={output};for(int k=0;k<12;k++)in[k]=&params[k];
 double global_peak=0,dist_peak=0;unsigned long long samples=0;int failures=0,quantized=0;double state_peak=0;
 for(int dist=0;dist<7;dist++)for(int a=0;a<101;a++)for(int f=0;f<1001;f++){
  float x=(float)a/100,y=(float)f/1000;
  double err=std::fabs(gp_distribution(dist,x,y)-Gendyn_distribution(dist,x,y));dist_peak=std::max(dist_peak,err);
  if(!std::isfinite(err) || err>0.0001)failures++;
 }
 for(int type=1;type<=3;type++)for(int dist=0;dist<7;dist++)for(int cp:{1,2,12,64})for(int control:{0,1}){
  float sr=control?44100.f/64:44100.f;rate.mSampleDur=1.0/sr;
  float freq=control?1:220;float standard[]={float(dist),float(dist),.7f,.6f,freq,freq*1.5f,.5f,.5f,float(cp),float(cp),1.17f,.31f};
  std::memcpy(params,standard,sizeof params);if(type==3){params[5]=.5;params[6]=.5;params[7]=cp;params[8]=cp;}
  auto p=std::make_unique<gendy_probe>();gp_init(p.get(),type,cp,sr,params);
  Gendy1 u1={};Gendy2 u2={};Gendy3 u3={};Unit *u=type==1?(Unit*)&u1:type==2?(Unit*)&u2:(Unit*)&u3;
  u->mInBuf=in;u->mOutBuf=out;u->mParent=&graph;u->mRate=&rate;rng.init(12345);
  if(type==1)Gendy1_Ctor(&u1);else if(type==2)Gendy2_Ctor(&u2);else Gendy3_Ctor(&u3);
  auto &s=p->v.state[0];float *mem=p->memory;
  auto compare=[&](float *a,float *b,int n){for(int i=0;i<n;i++)if(a[i]!=b[i]){std::fprintf(stderr,"constructor mismatch type%d dist%d cp%d\n",type,dist,cp);failures++;break;}};
  compare(mem,type==1?u1.mMemoryAmp:type==2?u2.mMemoryAmp:u3.mMemoryAmp,cp);
  compare(mem+cp,type==1?u1.mMemoryDur:type==2?u2.mMemoryDur:u3.mMemoryDur,cp);
  if(type==2){compare(mem+cp*2,u2.mMemoryAmpStep,cp);compare(mem+cp*3,u2.mMemoryDurStep,cp);}
  if(type==3)compare(mem+cp*2,u3.mAmpList,cp);
  double peak=0;unsigned count=1;
  for(int block=0;block<(control?512:32768);block++){
   // Step from identical state: include arrays, phase residuals and RNG.
   rng.s1=p->v.random[0];rng.s2=p->v.random[1];rng.s3=p->v.random[2];
   if(type==1){u1.mPhase=phase(s.phase,s.last);u1.mAmp=s.y1;u1.mNextAmp=s.y2;u1.mSpeed=s.a0;u1.mIndex=s.phase_acc;std::memcpy(u1.mMemoryAmp,mem,cp*4);std::memcpy(u1.mMemoryDur,mem+cp,cp*4);Gendy1_next_k(&u1,count);}
   if(type==2){u2.mPhase=phase(s.phase,s.last);u2.mAmp=s.y1;u2.mNextAmp=s.y2;u2.mSpeed=s.a0;u2.mIndex=s.phase_acc;std::memcpy(u2.mMemoryAmp,mem,cp*4);std::memcpy(u2.mMemoryDur,mem+cp,cp*4);std::memcpy(u2.mMemoryAmpStep,mem+cp*2,cp*4);std::memcpy(u2.mMemoryDurStep,mem+cp*3,cp*4);Gendy2_next_k(&u2,count);}
   if(type==3){u3.mPhase=phase(s.phase,s.last);u3.mAmp=s.y1;u3.mNextAmp=s.y2;u3.mSpeed=s.a0;u3.mIndex=s.phase_acc;u3.mLastPhase=phase(s.previous[0],s.previous[1]);u3.mNextPhase=phase(s.previous[2],s.previous[3]);u3.mInterpMult=s.width;std::memcpy(u3.mMemoryAmp,mem,cp*4);std::memcpy(u3.mMemoryDur,mem+cp,cp*4);std::memcpy(u3.mAmpList,mem+cp*2,(cp+1)*4);for(int i=0;i<=cp;i++)u3.mPhaseList[i]=mem[cp*3+1+i];Gendy3_next_k(&u3,count);}
   gp_step(p.get(),sr,params,count);
   if(p->v.fault){std::fprintf(stderr,"fault type%d dist%d cp%d block%d\n",type,dist,cp,block);failures++;break;}
   // Independently compare breakpoint updates and random draw ordering.
   float *refamp=type==1?u1.mMemoryAmp:type==2?u2.mMemoryAmp:u3.mMemoryAmp;
   float *refdur=type==1?u1.mMemoryDur:type==2?u2.mMemoryDur:u3.mMemoryDur;
   for(int j=0;j<cp;j++){state_peak=std::max(state_peak,(double)std::fabs(mem[j]-refamp[j]));state_peak=std::max(state_peak,(double)std::fabs(mem[cp+j]-refdur[j]));}
   if(type==2)for(int j=0;j<cp;j++){state_peak=std::max(state_peak,(double)std::fabs(mem[2*cp+j]-u2.mMemoryAmpStep[j]));state_peak=std::max(state_peak,(double)std::fabs(mem[3*cp+j]-u2.mMemoryDurStep[j]));}
   if(rng.s1!=p->v.random[0] || rng.s2!=p->v.random[1] || rng.s3!=p->v.random[2])failures++;
   for(unsigned i=0;i<count;i++){double err=std::fabs(output[i]-p->b.signal[0][i]);samples++;
    // An integer reciprocal crossing changes Gendy3's interpolation slope by
    // exactly one. Bound that discontinuity analytically, not with a loose
    // global waveform tolerance; breakpoint/RNG checks still apply.
    if(type==3 && std::fabs(s.width-u3.mInterpMult)==1 && err>0.001){
     double t=phase(s.phase,s.last)-s.a0-phase(s.previous[0],s.previous[1]);
     double bound=std::fabs(t*(s.y2-s.y1))+0.001;
     if(err<=bound){quantized++;err=0;}
    }
    peak=std::max(peak,err);if(!std::isfinite(err))failures++;}
  }
  // One-sample state transitions isolate numerical error from chaotic feedback.
  double limit=0.001;
  if(peak>limit){std::fprintf(stderr,"FAIL type%d dist%d cp%d kr%d peak%.9g\n",type,dist,cp,control,peak);failures++;}
  global_peak=std::max(global_peak,peak);
  if(type==1)Gendy1_Dtor(&u1);else if(type==2)Gendy2_Dtor(&u2);else Gendy3_Dtor(&u3);
 }
 if(state_peak>0.00001)failures++;
 std::printf("{\"samples\":%llu,\"distribution_samples\":707707,\"distribution_peak\":%.9g,\"transition_peak\":%.9g,\"state_peak\":%.9g,\"quantized_transitions\":%d,\"failures\":%d}\n",samples,dist_peak,global_peak,state_peak,quantized,failures);
 return failures?1:0;
}
