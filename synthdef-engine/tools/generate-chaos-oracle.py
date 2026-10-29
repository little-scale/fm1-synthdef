from pathlib import Path
import re
r=Path(__file__).resolve().parents[1];src=(r/'reference/ChaosUGens.cpp').read_text()
names=['HenonN','HenonL','HenonC','CuspN','CuspL','LatoocarfianN','LatoocarfianL','LatoocarfianC','LorenzL','FBSineN','FBSineL','FBSineC','StandardN','StandardL'];fields='x0 y0 z0 x1 xn yn zn xnm1 ynm1 znm1 xnm2 xnm3 frac c0 c1 c2 c3 a b counter stable'.split()
c=['#include "../src/fm_render.c"\n']
c.append('void cp_run(unsigned model,float *values,const float *args,float sr,int init,float *out){chaos_state s={0};')
c += [f's.{f}='+('((int)values[%d])'%i if f=='stable' else f'values[{i}]')+';' for i,f in enumerate(fields)]
c.append('chaos_unit unit={&s,args,out,sr};switch(model){')
c += [f'case {i}:if(init)chaos_{n}_Ctor(&unit);else chaos_{n}_next(&unit,1);break;' for i,n in enumerate(names)]
c.append('}')
c += [f'values[{i}]=(float)s.{f};' for i,f in enumerate(fields)]
c.append('}\nunsigned cp_mulhi(unsigned a,unsigned b){return stochastic_mulhi(a,b);}\nfloat cp_hash(float x){return stochastic_hash(x);}\n')
(r/'tests/chaos_probe.c').write_text('\n'.join(c)+'/* Exercise the actual runtime dispatch from independently supplied states. */\nfloat cp_noise(unsigned kind,float *state,const float *args){\n static fm_voice v;static fm_graph g;static fm_scratch b;\n fm_node n={0};n.type=kind?FM_Logistic:FM_Crackle;n.rate=2;n.inputs=kind?3:1;n.outputs=1;\n for(unsigned i=0;i<n.inputs;i++){g.ref[i].node=-1;g.ref[i].output=i;g.constant[i]=args[i];}\n fm_state *s=&v.state[0];memset(s,0,sizeof *s);s->initialized=1;s->y1=state[0];s->y2=state[1];v.fault=0;\n stochastic_render(&g,&v,&b,&n,s,1,44100,0);state[0]=s->y1;state[1]=s->y2;return b.signal[0][0];\n}\n')
cpp=['''/* Independent identical-state comparisons against unmodified SC 3.14.1. */
#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "../reference/ChaosUGens.cpp"
extern "C" void cp_run(unsigned,float*,const float*,float,int,float*);
extern "C" unsigned cp_mulhi(unsigned,unsigned);
extern "C" float cp_hash(float);
int main(){unsigned long checks=0,failures=0;double peak=0;Rate rate={};float params[8]={},outref[2]={},outport[2]={};float *in[8],*out[1]={outref};for(int i=0;i<8;i++)in[i]=params+i;
''']
for i,n in enumerate(names):
 used=set()
 for suffix in ['next','Ctor']:
  start=src.index('void '+n+'_'+suffix+'(');start=src.index('void '+n+'_'+suffix+'(',start+1) # skip prototype
  end=src.index('\n}\n',start)+3
  used.update(re.findall(r'unit->(\w+)',src[start:end]))
 selected=[(k,f) for k,f in enumerate(fields) if f in used]
 params=[4000,1.4,.3,0,0,0,0,0] if i<3 else [4000,1,1.9,0,0,0,0,0] if i<5 else [4000,1,3,.5,.5,.5,.5,0] if i<8 else [4000,10,28,2.667,.05,.1,0,0] if i==8 else [4000,1,.1,1.1,.5,.1,.1,0] if i<12 else [4000,1,.5,.5,0,0,0,0]
 cpp.append('{for(float sr:{8000.f,44100.f,48000.f,192000.f}){rate.mSampleRate=sr;rate.mSampleDur=1.0/sr;float standard[8]={'+','.join(str(v)+'f' if '.' in str(v) else str(v) for v in params)+'};std::memcpy(params,standard,sizeof params);')
 cpp.append(f'{n} u={{}};u.mInBuf=in;u.mOutBuf=out;u.mRate=&rate;float values[21]={{}};{n}_Ctor(&u);cp_run({i},values,params,sr,1,outport);')
 compare=''.join(f'{{double error=std::fabs(values[{k}]-(double)u.{f})/(1+std::fabs(u.{f}));peak=std::max(peak,error);checks++;if(!std::isfinite(error)||error>2e-5)failures++;}}' for k,f in selected)
 cpp.append(compare)
 cpp.append('for(int step=0;step<20000;step++){params[0]=(step/1000)%2?sr*.37f:2000;')
 cpp.extend(f'u.{f}=values[{k}];' for k,f in selected)
 cpp.append(f'{n}_next(&u,1);cp_run({i},values,params,sr,0,outport);'+compare)
 cpp.append('double error=std::fabs(outport[0]-outref[0])/(1+std::fabs(outref[0]));checks++;peak=std::max(peak,error);if(!std::isfinite(error)||error>2e-5)failures++;}}}')
cpp.append('''unsigned rng=12345;for(unsigned i=0;i<1000000;i++){rng=rng*1664525u+1013904223u;unsigned range=(i%16777215)+1;unsigned expected=(unsigned)(((unsigned long long)rng*range)>>32);if(cp_mulhi(rng,range)!=expected)failures++;checks++;}
std::printf("{\\"comparisons\\":%lu,\\"max_normalized_error\\":%.9g,\\"failures\\":%lu}\\n",checks,peak,failures);return failures?1:0;}
''')
(r/'tests/chaos_oracle.cpp').write_text('\n'.join(cpp))
