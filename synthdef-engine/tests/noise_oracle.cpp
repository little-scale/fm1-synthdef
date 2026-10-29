/* GPL-3.0-or-later. Pinned SC noise helpers and construction. */
#include <cstdio>
#include <cmath>
#include <cstring>
#include "../reference/NoiseUGens.cpp"
extern "C" unsigned cp_mulhi(unsigned,unsigned);
extern "C" float cp_hash(float);
extern "C" float cp_noise(unsigned,float*,const float*);
int main(){unsigned checks=0,failures=0;RGen rng;Graph graph={};graph.mRGen=&rng;float args[3]={-5,5,1},values[64]={};float *in[3]={args,args+1,args+2},*out[1]={values};Wire wire[3]={};Wire *w[3]={wire,wire+1,wire+2};wire[2].mCalcRate=calc_FullRate;TIRand unit={};unit.mInBuf=in;unit.mOutBuf=out;unit.mInput=w;unit.mParent=&graph;unit.mCalcRate=calc_FullRate;
 rng.init(12345);TIRand_Ctor(&unit);float ctor=values[0];
 RGen separate;separate.init(12345);int expected=(int)cp_mulhi(separate.trand(),11)-5;if(ctor!=expected)failures++;checks++;
 // Demonstrate SC's constructor alias bug independently: the negative output overwrites its trigger.
 out[0]=args+2;args[2]=1;rng.init(12345);TIRand_Ctor(&unit);if(unit.m_trig!=-1)failures++;checks++;
 for(int i=0;i<1000000;i++){unsigned x=rng.trand();int range=i%16777215+1;unsigned ref=(unsigned)std::floor((x/4294967296.0)*range);if(cp_mulhi(x,range)!=ref)failures++;checks++;float f;std::memcpy(&f,&x,4);if(std::isfinite(f)){unsigned h=0x40000000u|((unsigned)Hash((int)x)>>9);float v;std::memcpy(&v,&h,4);if(cp_hash(f)!=v-3)failures++;checks++;}}
 float p[3]={1.95f,44100,.5f},a[2]={};float *inputs[3]={p,p+1,p+2};out[0]=values;
 Crackle cr={};cr.mInBuf=inputs;cr.mOutBuf=out;Logistic log={};log.mInBuf=inputs;log.mOutBuf=out;
 for(unsigned kind=0;kind<2;kind++)for(int i=0;i<100000;i++){
  a[0]=rng.frand();a[1]=rng.frand();p[0]=kind?3.9f:1.95f;
  if(kind){log.m_y1=a[0];Logistic_next_1(&log,1);}else{cr.m_y1=a[0];cr.m_y2=a[1];Crackle_next(&cr,1);}
  float result=cp_noise(kind,a,p);if(std::fabs(result-values[0])>2e-6)failures++;checks++;
 }
 std::printf("{\"checks\":%u,\"tirand_negative_constructor\":%.0f,\"failures\":%u}\n",checks,ctor,failures);return failures?1:0;}
