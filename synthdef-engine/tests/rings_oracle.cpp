/* GPL-3.0-or-later. Compare model ports to unmodified pinned MIT Rings Part. */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <memory>
#include "rings/dsp/part.h"
#include "stmlib/utils/random.h"
float rings::Dsp::sr=44100.f;
float rings::Dsp::a3=440.f/44100.f;
extern "C" unsigned rp_words(unsigned,unsigned);
extern "C" void rp_init(float*,unsigned,float,unsigned,unsigned);
extern "C" void rp_process(float*,unsigned,float,const float*,float*,float*,float,const float*,int,int,unsigned);
int main(){unsigned failures=0,cases=0;std::printf("[\n");
 for(float sr:{32000.f,44100.f,48000.f,192000.f})for(unsigned model=1;model<=6;model++)for(unsigned compact=0;compact<(model==2||model==5?2u:1u);compact++)for(int mode=0;mode<5;mode++){ rings::Dsp::setSr(sr);
  auto part=std::make_unique<rings::Part>();std::memset(part.get(),0,sizeof(*part));std::vector<uint16_t> reverb(32768);
  stmlib::Random::Seed(0x21);part->Init(reverb.data());part->set_polyphony(1);part->set_model(static_cast<rings::ResonatorModel>(model-1));
  std::vector<float> memory(rp_words(model,compact)+16,123456.0f);rp_init(memory.data()+8,model,sr,0x21,compact);
  double peak=0,error2=0,energy=0;float in[32],l[32],r[32],a[32],b[32];bool finite=true;
  for(unsigned block=0;block<2048;block++){
   float controls[4]={.4f,.6f,.5f,.4f};float pitch=48;
   if(mode==2 && block>=1024){controls[0]=.8f;controls[1]=.3f;controls[2]=.8f;controls[3]=.7f;pitch=55;}
   if(mode==3){pitch=0;controls[0]=.1f;controls[1]=.6f;controls[2]=.8f;controls[3]=.4f;}
   if(mode==4){pitch=96;controls[0]=.99f;controls[1]=.99f;controls[2]=.95f;controls[3]=.95f;}
   bool trigger=block%512==0,internal=mode!=1;
   for(unsigned j=0;j<32;j++)in[j]=mode==1?((block%512==0&&j==0)?.1f:0.f):0.f;
   rings::PerformanceState ps={};ps.internal_exciter=internal;ps.tonic=12;ps.note=pitch;ps.strum=trigger;ps.chord=std::round(controls[0]*10);
   rings::Patch patch={controls[0],controls[1],controls[2],controls[3]};
   part->Process(ps,patch,in,l,r,32);rp_process(memory.data()+8,model,sr,in,a,b,pitch,controls,trigger,internal,compact);
   for(unsigned j=0;j<32;j++){double e=std::fmax(std::fabs(a[j]-l[j]),std::fabs(b[j]-r[j]));peak=std::fmax(peak,e);error2+=(a[j]-l[j])*(a[j]-l[j])+(b[j]-r[j])*(b[j]-r[j]);energy+=l[j]*l[j]+r[j]*r[j];finite=finite&&std::isfinite(a[j])&&std::isfinite(b[j]);}
  }
  bool guards=true;for(unsigned i=0;i<8;i++)guards=guards&&memory[i]==123456&&memory[rp_words(model,compact)+8+i]==123456;
  double rms=std::sqrt(error2/131072),reference_rms=std::sqrt(energy/131072);bool exact_expected=!compact||(sr>=44100&&sr<=48000&&mode<3);bool matched=peak<.001&&rms<.0001;bool passed=finite&&guards&&(!exact_expected||matched);
  failures+=!passed;std::printf("%s{\"model\":%u,\"compact\":%u,\"sample_rate\":%.0f,\"mode\":%d,\"bytes\":%u,\"frames\":65536,\"peak\":%.9g,\"rms\":%.9g,\"reference_rms\":%.9g,\"match_required\":%s,\"passed\":%s}",cases++?",\n":"",model,compact,sr,mode,rp_words(model,compact)*4,peak,rms,reference_rms,exact_expected?"true":"false",passed?"true":"false");
 }std::puts("\n]");return failures?1:0;}
