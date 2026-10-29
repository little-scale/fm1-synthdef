/* GPL-3.0-or-later. Test-only SC wrappers around unmodified pinned MIT Rings Part.
 * Explicit-trigger, single-voice API, matching FM1Rings.sc. No port DSP is used. */
#include "SC_PlugIn.h"
#include "rings/dsp/part.h"
#include "stmlib/utils/random.h"
#include <cstring>
#include <cmath>
static InterfaceTable *ft;
float rings::Dsp::sr=44100.f;
float rings::Dsp::a3=440.f/44100.f;
struct RingsUnit:Unit {rings::Part *part;uint16_t *reverb;uint32_t rng;bool previous;};
static void rings_next(RingsUnit *unit,int count){
 if(!unit->part){ClearUnitOutputs(unit,count);return;}
 const bool trig=IN0(1)>0,fire=trig&&!unit->previous;unit->previous=trig;
 const bool audio=INRATE(0)==calc_FullRate,internal=!audio||IN0(7)>0,bypass=IN0(8)>0;
 float p[4];for(int i=0;i<4;i++)p[i]=std::fmax(0.f,std::fmin(1.f,IN0(3+i)));
 rings::Patch patch={p[0],p[1],p[2],p[3]};
 rings::PerformanceState ps={};ps.tonic=12;ps.note=std::fmax(0.f,std::fmin(114.f,IN0(2)));ps.internal_exciter=internal;ps.chord=std::round(p[0]*10);
 stmlib::Random::Seed(unit->rng);
 for(int offset=0;offset<count;offset+=32){float input[32]={};if(audio)std::memcpy(input,IN(0)+offset,sizeof input);
  if(bypass){std::memcpy(OUT(0)+offset,input,sizeof input);std::memcpy(OUT(1)+offset,input,sizeof input);}
  else {ps.strum=fire&&offset==0;unit->part->Process(ps,patch,input,OUT(0)+offset,OUT(1)+offset,32);}
 }
 unit->rng=stmlib::Random::state();
}
static void rings_ctor(RingsUnit *unit,int model){
 unit->part=nullptr;unit->reverb=nullptr;unit->previous=false;unit->rng=unit->mParent->mRGen->trand();
 SETCALC(rings_next);OUT0(0)=OUT0(1)=0;
 if(unit->mWorld->mFullRate.mBufLength!=64)return;
 if(IN0(9)!=0){Print("ERROR: Rings reference plugin requires compact=0; use the direct compact comparison suite.\n");return;}
 unit->part=static_cast<rings::Part*>(RTAlloc(unit->mWorld,sizeof(rings::Part)));
 unit->reverb=static_cast<uint16_t*>(RTAlloc(unit->mWorld,65536));
 if(!unit->part||!unit->reverb){if(unit->part)RTFree(unit->mWorld,unit->part);unit->part=nullptr;return;}
 std::memset(unit->part,0,sizeof(rings::Part));std::memset(unit->reverb,0,65536);
 rings::Dsp::setSr(SAMPLERATE);unit->part->Init(unit->reverb);unit->part->set_polyphony(1);unit->part->set_model(static_cast<rings::ResonatorModel>(model-1));
}
static void rings_dtor(RingsUnit *unit){if(unit->part)RTFree(unit->mWorld,unit->part);if(unit->reverb)RTFree(unit->mWorld,unit->reverb);}
#define RINGS_UNIT(N) struct MiRings##N:RingsUnit{};static void MiRings##N##_Ctor(MiRings##N*u){rings_ctor(u,N);}static void MiRings##N##_Dtor(MiRings##N*u){rings_dtor(u);}
RINGS_UNIT(1) RINGS_UNIT(2) RINGS_UNIT(3) RINGS_UNIT(4) RINGS_UNIT(5) RINGS_UNIT(6)
PluginLoad(FM1RingsReference){ft=inTable;DefineDtorUnit(MiRings1);DefineDtorUnit(MiRings2);DefineDtorUnit(MiRings3);DefineDtorUnit(MiRings4);DefineDtorUnit(MiRings5);DefineDtorUnit(MiRings6);}
