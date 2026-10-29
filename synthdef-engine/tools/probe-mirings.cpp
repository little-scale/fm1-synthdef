/* GPL-3.0-or-later. Host feasibility probe against the pinned upstream Rings.
 * Build using the upstream eurorack include root and DSP sources; see roadmap.
 * This is not an FM-1 implementation or hardware performance measurement. */
#include <cstdio>
#include <cstring>
#include <cmath>
#include "rings/dsp/part.h"
#include "rings/dsp/string_synth_part.h"
#include "rings/dsp/strummer.h"
float rings::Dsp::sr=44100.f;
float rings::Dsp::a3=440.f/44100.f;
int main(){
 const size_t bytes=sizeof(rings::Part)+sizeof(rings::StringSynthPart)+sizeof(rings::Strummer)+sizeof(rings::PerformanceState)+sizeof(rings::Patch)+65536+512;
 printf("{\"part_bytes\":%zu,\"string_synth_bytes\":%zu,\"strummer_bytes\":%zu,\"minimum_instance_bytes\":%zu,\"runs\":[",sizeof(rings::Part),sizeof(rings::StringSynthPart),sizeof(rings::Strummer),bytes);
 for(int egg=0;egg<2;egg++)for(int model=0;model<6;model++){
  rings::Part part; rings::StringSynthPart string_synth;rings::Strummer strummer;uint16_t reverb[32768]={0};
  std::memset(&part,0,sizeof(part));std::memset(&string_synth,0,sizeof(string_synth));std::memset(&strummer,0,sizeof(strummer));
  part.Init(reverb);string_synth.Init(reverb);strummer.Init(.01f,44100.f/32);part.set_polyphony(1);string_synth.set_polyphony(1);
  part.set_model(static_cast<rings::ResonatorModel>(model));string_synth.set_fx(static_cast<rings::FxType>(model));
  rings::Patch patch={.4f,.6f,.5f,.4f};rings::PerformanceState ps={};ps.internal_exciter=true;ps.internal_strum=false;ps.internal_note=false;ps.tonic=12;ps.note=48;ps.chord=4;
  float input[32]={0},left[32],right[32];double sum=0;float peak=0;bool finite=true;
  for(int block=0;block<4096;block++){
   ps.strum=block%1024==0;strummer.Process(egg?nullptr:input,32,&ps);
   if(egg)string_synth.Process(ps,patch,input,left,right,32);else part.Process(ps,patch,input,left,right,32);
   for(int i=0;i<32;i++){finite=finite&&std::isfinite(left[i])&&std::isfinite(right[i]);peak=std::fmax(peak,std::fmax(std::fabs(left[i]),std::fabs(right[i])));sum+=left[i]*left[i]+right[i]*right[i];}
  }
  printf("%s{\"model\":%d,\"easter_egg\":%s,\"frames\":131072,\"peak\":%.8g,\"rms\":%.8g,\"passed\":%s}",egg||model?",":"",model,egg?"true":"false",peak,std::sqrt(sum/(131072*2)),finite&&sum>0?"true":"false");
  if(!finite || sum<=0)return 1;
 }
 puts("]}");
}
