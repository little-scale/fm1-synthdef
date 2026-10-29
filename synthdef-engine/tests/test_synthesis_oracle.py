#!/usr/bin/env python3
"""Extract unchanged functions from pinned sources for independent float-state checks."""
from pathlib import Path
import re,json,hashlib,subprocess,sys
r=Path(__file__).resolve().parents[1];device='--device' in sys.argv;tag='device' if device else 'host';out=r/'build';manifest=json.loads((r/'reference/sources.json').read_text())['sha256'];sources={}
for file in ['OscUGens.cpp','FilterUGens.cpp']:
 p=r/'reference'/file;assert hashlib.sha256(p.read_bytes()).hexdigest()==manifest[file];sources[file]=p.read_text()
def function(file,name):
 s=sources[file];m=re.search(r'void '+name+r'\([^)]*\) \{',s);assert m;i=m.end();depth=1
 while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[m.start():i]
def structure(file,name):return re.search(r'struct '+name+r' : [\s\S]*?\n};',sources[file])[0]
code='#include "SC_PlugIn.h"\n#include <limits>\n#include <cstdio>\n#include <cmath>\n#include "fm_synth.h"\n#define PI 3.14159265358979323846\nstatic InterfaceTable storage;static InterfaceTable *ft=&storage;static const float uninitializedControl=std::numeric_limits<float>::quiet_NaN();\n'
for n in ['BufUnit','TableLookup','SinOscFB']:code+=structure('OscUGens.cpp',n)+'\n'
code+=structure('FilterUGens.cpp','MoogFF')+'\n'
for name,file in [('SinOscFB','OscUGens.cpp'),('MoogFF','FilterUGens.cpp')]:
 suffix='next_kk' if name=='SinOscFB' else 'next';code+=function(file,name+'_'+suffix)+'\n'+function(file,name+'_Ctor')+'\n'
code+='''extern "C" void xp_run(unsigned,fm_state*,const float*,float,int,float*);
int main(){unsigned checks=0,failures=0;double peak=0;float sine[8193],wavetable[16384];for(unsigned i=0;i<=8192;i++)sine[i]=std::sin(i*(2*3.14159265358979323846/8192));for(unsigned i=0;i<8192;i++){float a=sine[i],b=sine[(i+1)%8192];wavetable[i*2]=2*a-b;wavetable[i*2+1]=b-a;}storage.mSineSize=8192;storage.mSineWavetable=wavetable;
for(float sr:{8000.f,44100.f,48000.f,192000.f}){Rate rate={};rate.mSampleRate=sr;rate.mSampleDur=1.0/sr;rate.mSlopeFactor=1.f/64;float args[4]={220,4,0,0},ref[1],port=0,*in[4]={args,args+1,args+2,args+3},*output[1]={ref};SinOscFB u={};u.mRate=&rate;u.mInBuf=in;u.mOutBuf=output;fm_state state={};SinOscFB_Ctor(&u);xp_run(FM_SinOscFB,&state,args,sr,1,&port);
for(unsigned i=0;i<100000;i++){args[1]=(i/10000)%2?4:.3f;u.m_prevout=state.y1;u.m_feedback=state.last;u.m_phase=(int32)state.phase_acc;SinOscFB_next_kk(&u,1);xp_run(FM_SinOscFB,&state,args,sr,0,&port);double e=std::fabs(port-ref[0]);if(e>2e-6||!std::isfinite(e))failures++;if(e>peak)peak=e;checks++;}
MoogFF m={};m.mRate=&rate;m.mInBuf=in;m.mOutBuf=output;state={};args[0]=.1;args[1]=1000;args[2]=2;args[3]=0;MoogFF_Ctor(&m);xp_run(FM_MoogFF,&state,args,sr,1,&port);
for(unsigned i=0;i<100000;i++){args[0]=std::sin(i*.17)*.2;args[1]=(i/1000)%2?sr*.2:sr*.01;args[2]=(i/10000)%2?3.8:0;args[3]=i%10000==0?1:0;m.m_s1=state.previous[0];m.m_s2=state.previous[1];m.m_s3=state.previous[2];m.m_s4=state.previous[3];m.m_freq=state.freq;m.m_b0=state.a0;m.m_a1=state.b1;m.m_k=state.last;MoogFF_next(&m,1);xp_run(FM_MoogFF,&state,args,sr,0,&port);double e=std::fabs(port-ref[0]);if(e>2e-6||!std::isfinite(e))failures++;if(e>peak)peak=e;checks++;}
}
std::printf("{\\"comparisons\\":%u,\\"maximum_error\\":%.9g,\\"failures\\":%u}\\n",checks,peak,failures);return failures?1:0;}
'''
p=out/f'synthesis-oracle-{tag}.cpp';p.write_text(code);obj=out/f'synthesis-probe-{tag}.o';sc=r/'reference/sc-headers/include';inc=['-I'+str(r/'src'),'-I'+str(sc/'plugin_interface'),'-I'+str(sc/'common')];flags=['-O2','-fno-fast-math','-ffp-contract=off'];extra=['-DPLAT_HOST','-I'+str(r.parent/'fm1-synth/firmware/device_include'),'-I'+str(r.parent/'fm1-base/include')] if device else []
subprocess.run(['cc','-std=c99',*flags,*extra,*inc,'-c',str(r/'tests/synthesis_probe.c'),'-o',str(obj)],check=True)
exe=out/f'synthesis-oracle-{tag}';subprocess.run(['c++','-std=c++17',*flags,*inc,str(p),str(obj),'-o',str(exe)],check=True);run=subprocess.run([str(exe)],capture_output=True,text=True);print(run.stdout,run.stderr);(out/f'synthesis-oracle-{tag}-results.json').write_text(run.stdout);raise SystemExit(run.returncode)
