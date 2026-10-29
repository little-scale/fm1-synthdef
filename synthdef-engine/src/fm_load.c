/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fm_synth.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define FM_VALID 0x464d5331u
static const char *const types[] = {"Control","SinOsc","EnvGen","Pan2","Out","BinaryOpUGen","UnaryOpUGen","MulAdd","Sum3","Sum4","DC","K2A","A2K","Line","XLine","Resonz","Ringz","Klang","Klank","LagControl","TrigControl","LFSaw","LFTri","LFPulse","Lag","LagUD","LPF","HPF","RLPF","BPF","LeakDC","Select","Clip","WhiteNoise","PinkNoise","LFNoise0","LFNoise1","LFNoise2","RandSeed","Saw","Pulse","DelayN","DelayL","DelayC","CombN","CombL","CombC","AllpassN","AllpassL","AllpassC","Rand","ExpRand","In","SampleRate","SampleDur","ControlRate","ControlDur","BlockSize","RadiansPerSample","Delay1","Delay2","OnePole","OneZero","LPZ1","HPZ1","Latch","Gate","TRand","TExpRand","Dust","Dust2","Impulse","Decay","Decay2","FreeVerb","FreeVerb2","MiVerb","Fold","Wrap","BrownNoise","ClipNoise","Trig","Trig1","PulseDivider","PulseCount","ToggleFF","SetResetFF","Stepper","XFade2","LinXFade2","Balance2","LinExp","Slew","Lag2","Lag3","MiMu","RHPF","BRF","VarSaw","SyncSaw","LFCub","LFPar","LFDNoise0","LFDNoise1","LFDNoise3","Amplitude","BPeakEQ","BLowShelf","BHiShelf","Gendy1","Gendy2","Gendy3","Formant","Formlet","MaxLocalBufs","LocalBuf","SetBuf","DegreeToKey","MiRings1","MiRings2","MiRings3","MiRings4","MiRings5","MiRings6","Crackle","LFClipNoise","LFDClipNoise","GrayNoise","Hasher","CoinGate","TIRand","TWindex","Logistic","HenonN","HenonL","HenonC","CuspN","CuspL","LatoocarfianN","LatoocarfianL","LatoocarfianC","LorenzL","SinOscFB","Blip","Phasor","Pluck","MoogFF","FreqShift","Limiter","FBSineN","FBSineL","FBSineC","StandardN","StandardL"};
const char *fm_type_name(unsigned type) { return type < FM_TYPES ? types[type] : "unknown"; }
typedef struct { const uint8_t *p; size_t left; int failed; } reader;
static uint32_t rd(reader *r, unsigned n) {
 uint32_t x = 0; unsigned i;
 if (r->failed || r->left < n) { r->failed = 1; return 0; }
 for (i=0;i<n;i++) x=(x<<8)|*r->p++;
 r->left-=n; return x;
}
static float rf(reader *r) { uint32_t x=rd(r,4); float f; memcpy(&f,&x,4); if (!isfinite(f)) r->failed=1; return f; }
static void rs(reader *r, char out[64]) {
 unsigned n=rd(r,1),i;
 if (n==0 || n>63 || n>r->left) { r->failed=1; out[0]=0; return; }
 for(i=0;i<n;i++) { out[i]=(char)rd(r,1); if (!out[i]) r->failed=1; }
 out[n]=0;
}
static int fail(fm_graph *g,char *err,size_t cap,const char *why,int node) {
 g->valid=0; if(cap) snprintf(err,cap,"%s (node %d)",why,node); return 0;
}
static int literal(const fm_graph *g,const fm_node *n,unsigned input,float *v) {
 const fm_ref *r=&g->ref[n->input+input];
 if(r->node!=-1) return 0;
 *v=g->constant[r->output]; return 1;
}
static int count(reader *r,unsigned max) { uint32_t n=rd(r,4); if(n>max) {r->failed=1;return 0;} return (int)n; }
int fm_load(fm_graph *g,const void *bytes,size_t size,char *error,size_t cap) {
 reader r={(const uint8_t *)bytes,size,0}; int i,j; unsigned n; float v,w;
 memset(g,0,sizeof *g); if(cap) error[0]=0;
 #define BAD(s) return fail(g,error,cap,s,i)
 i=-1;
 if(!bytes || size>FM_FILE_BYTES || rd(&r,4)!=0x53436766 || rd(&r,4)!=2 || rd(&r,2)!=1) BAD("Expected one version-2 SynthDef, at most 64 KiB");
 rs(&r,g->name);
 g->constants=(uint16_t)count(&r,FM_CONSTANTS);
 for(i=0;i<g->constants;i++) g->constant[i]=rf(&r);
 g->controls=(uint16_t)count(&r,FM_CONTROLS);
 for(i=0;i<g->controls;i++) g->defaults[i]=rf(&r);
 g->names=(uint16_t)count(&r,FM_CONTROLS);
 for(i=0;i<g->names;i++) {
  rs(&r,g->control_name[i].name); n=rd(&r,4);
  if(n>=g->controls) BAD("Control name index out of bounds");
  g->control_name[i].index=(uint16_t)n;
  for(j=0;j<i;j++) if(!strcmp(g->control_name[j].name,g->control_name[i].name)) BAD("Duplicate control name");
 }
 n=rd(&r,4);
 if(n>FM_NODES){if(cap)snprintf(error,cap,"Graph needs %u nodes; limit is %u",n,FM_NODES);return 0;}
 g->nodes=(uint16_t)n;
 if(!g->nodes || r.failed) BAD("Invalid header, count, name or non-finite value");
 for(i=0;i<g->nodes;i++) {
  char name[64]; fm_node *u=&g->node[i];
  rs(&r,name);
  for(j=0;j<FM_TYPES && strcmp(types[j],name);j++);
  if(j==FM_TYPES) { if(cap) snprintf(error,cap,"Unsupported UGen '%s' (node %d)",name,i); return 0; }
  u->type=(uint8_t)j; u->rate=(uint8_t)rd(&r,1);
  if(u->rate>2) BAD("Demand/unknown calculation rate is unsupported");
  u->input=g->inputs; u->inputs=(uint16_t)count(&r,FM_INPUTS-g->inputs); g->inputs+=u->inputs;
  u->output=g->outputs; u->outputs=(uint16_t)count(&r,FM_OUTPUTS-g->outputs); g->outputs+=u->outputs;
  u->special=(int16_t)rd(&r,2);
  if(r.failed) BAD("Input/output count exceeds arena or file is truncated");
  for(j=0;j<u->inputs;j++) {
   int32_t source=(int32_t)rd(&r,4); uint32_t index=rd(&r,4);
   fm_ref *ref=&g->ref[u->input+j];
   if(source==-1) { if(index>=g->constants) BAD("Constant reference out of bounds"); }
   else if(source<0 || source>=i || index>=g->node[source].outputs) BAD("Invalid or forward UGen reference");
   ref->node=(int16_t)source; ref->output=(uint16_t)index;if(source>=0)g->used[source]=1;
  }
  for(j=0;j<u->outputs;j++) {
   n=rd(&r,1); if(n!=u->rate) BAD("Output rate does not match UGen rate");
   g->output_rate[u->output+j]=(uint8_t)n;
  }
  if(r.failed) BAD("Truncated node");
  if(u->type!=FM_Control && u->type!=FM_LagControl && u->type!=FM_TrigControl && u->type!=FM_BinaryOpUGen && u->type!=FM_UnaryOpUGen && u->special) BAD("Unexpected special index");
  if(u->type==FM_Control || u->type==FM_LagControl || u->type==FM_TrigControl) {
   if(u->type==FM_LagControl) {
    if(u->inputs!=u->outputs || u->rate!=1) BAD("Invalid LagControl layout/rate");
    for(j=0;j<u->inputs;j++) {fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate!=0) BAD("LagControl times must be initial rate");}
   } else if(u->inputs || (u->type==FM_TrigControl && u->rate!=1)) BAD("Invalid control inputs/rate");
   if(!u->outputs || u->rate>1 || u->special<0 || u->special+u->outputs>g->controls) BAD("Invalid Control layout/rate");
  } else if(u->type>=FM_SinOscFB && u->type<=FM_StandardL) {
   static const unsigned counts[]={2,2,5,6,4,3,3,7,7,7,4,4};
   if(u->rate<1 || u->outputs!=1 || u->inputs!=counts[u->type-FM_SinOscFB])BAD("Invalid synthesis/effect layout/rate");
   if((u->type==FM_Pluck || u->type==FM_FreqShift || u->type==FM_Limiter || u->type>=FM_FBSineN) && u->rate!=2)BAD("UGen requires audio rate");
   for(j=0;j<u->inputs;j++){
    fm_ref *ref=&g->ref[u->input+j];unsigned rate=ref->node<0?0:g->node[ref->node].rate;
    int audio=(u->type==FM_Phasor&&j<2)||(u->type==FM_Pluck&&(j==0||j==1||j==5))||(u->type==FM_MoogFF&&j==0)||u->type==FM_FreqShift||(u->type==FM_Limiter&&j==0);
    if(rate==2 && (!audio || u->rate!=2))BAD("Parameter requires initial/control rate");
    if(j==0 && u->rate==2 && (u->type==FM_Pluck||u->type==FM_MoogFF||u->type==FM_FreqShift||u->type==FM_Limiter) && rate!=2)BAD("Audio effect requires audio input");
   }
   if(u->type==FM_Phasor && g->ref[u->input+1].node>=0 && g->node[g->ref[u->input+1].node].rate==2 && (g->ref[u->input].node<0 || g->node[g->ref[u->input].node].rate!=2))BAD("Audio Phasor rate requires audio trigger");
   if(u->type==FM_Pluck || u->type==FM_Limiter){if(!literal(g,u,2,&v) || v<0 || (u->type==FM_Limiter && v==0))BAD("Buffer duration must be a positive literal (Pluck permits zero)");}
  } else if(u->type>=FM_Crackle && u->type<=FM_LorenzL) {
   static const unsigned counts[]={1,1,1,0,1,2,3,0,3,5,5,5,4,4,7,7,7,8};
   unsigned want=counts[u->type-FM_Crackle];
   if(u->rate<1 || u->outputs!=1 || (u->type!=FM_TWindex && u->inputs!=want))BAD("Invalid stochastic generator layout/rate");
   if(u->type==FM_TWindex && (u->inputs<3 || u->inputs>66))BAD("TWindex requires 1–64 weights");
   if(u->type>=FM_HenonN && u->rate!=2)BAD("Chaotic map oscillators support audio rate only");
   for(j=0;j<u->inputs;j++){
    fm_ref *ref=&g->ref[u->input+j];unsigned rate=ref->node<0?0:g->node[ref->node].rate;
    int audio=(u->type==FM_LFDClipNoise||u->type==FM_Hasher)||(u->type==FM_CoinGate&&j==1)||(u->type==FM_TIRand&&j==2)||(u->type==FM_TWindex&&j==0);
    if(rate==2 && (!audio || u->rate!=2))BAD("Stochastic parameter requires initial/control rate");
    if(u->rate==2 && ((u->type==FM_Hasher&&j==0)||(u->type==FM_CoinGate&&j==1)||(u->type==FM_TIRand&&j==2)) && rate!=2)BAD("Audio stochastic input/trigger requires audio rate");
    if(u->type==FM_Logistic&&j==2&&rate!=0)BAD("Logistic init must be initial rate");
   }
  } else if(fm_is_rings(u->type)) {
   if(u->rate!=2 || u->inputs!=10 || u->outputs!=2)BAD("MiRings1-6 require ten inputs and two audio outputs");
   if(!literal(g,u,9,&v) || (v!=0 && v!=1))BAD("MiRings compact must be a literal 0 or 1");
   for(j=1;j<9;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate>1)BAD("MiRings parameters and trigger require initial/control rate");}
   fm_ref *ref=&g->ref[u->input];if(ref->node>=0 && g->node[ref->node].rate==1)BAD("MiRings excitation requires audio rate or a scalar placeholder");
  } else if(u->type==FM_MaxLocalBufs) {
   if(u->rate || u->inputs!=1 || u->outputs!=1 || !literal(g,u,0,&v) || v<1 || v>FM_SCALE_TABLES || v!=floorf(v)) BAD("MaxLocalBufs requires a literal count from 1 to 16");
   for(j=0;j<i;j++)if(g->node[j].type==FM_MaxLocalBufs)BAD("Duplicate MaxLocalBufs declaration");
  } else if(u->type==FM_LocalBuf) {
   if(u->rate || u->inputs!=3 || u->outputs!=1 || !literal(g,u,0,&v) || v!=1)BAD("LocalBuf supports mono constant pitch tables only");
   if(!literal(g,u,1,&v) || v<1 || v>FM_SCALE_ENTRIES || v!=floorf(v))BAD("LocalBuf table length must be a literal integer from 1 to 128");
   u->partials=(uint16_t)v;u->partial=UINT16_MAX;
   fm_ref *ref=&g->ref[u->input+2];
   if(ref->node<0 || g->node[ref->node].type!=FM_MaxLocalBufs)BAD("LocalBuf requires MaxLocalBufs dependency");
   float maximum;literal(g,&g->node[ref->node],0,&maximum);unsigned tables=1;
   for(j=0;j<i;j++)if(g->node[j].type==FM_LocalBuf)tables++;
   if(tables>(unsigned)maximum)BAD("LocalBuf count exceeds MaxLocalBufs");
  } else if(u->type==FM_SetBuf) {
   if(u->rate || u->inputs<4 || u->outputs!=1)BAD("SetBuf requires one initial-rate constant table initializer");
   fm_ref *ref=&g->ref[u->input];
   if(ref->node<0 || g->node[ref->node].type!=FM_LocalBuf)BAD("SetBuf requires a direct LocalBuf reference");
   fm_node *table=&g->node[ref->node];
   if(table->partial!=UINT16_MAX)BAD("A constant LocalBuf table may only be initialized once");
   if(!literal(g,u,1,&v) || v!=0 || !literal(g,u,2,&v) || v!=table->partials || u->inputs!=3+table->partials)BAD("SetBuf must initialize the entire table at offset zero");
   for(j=3;j<u->inputs;j++)if(!literal(g,u,j,&v))BAD("SetBuf pitch values must be literal constants");
   table->partial=u->input+3;
  } else if(u->type==FM_DegreeToKey) {
   if(!u->rate || u->inputs!=3 || u->outputs!=1)BAD("DegreeToKey requires three inputs at audio/control rate");
   fm_ref *ref=&g->ref[u->input];
   if(ref->node<0 || g->node[ref->node].type!=FM_LocalBuf || g->node[ref->node].partial==UINT16_MAX)BAD("DegreeToKey requires an initialized constant LocalBuf table");
   u->partial=g->node[ref->node].partial;u->partials=g->node[ref->node].partials;
   ref=&g->ref[u->input+1];if(ref->node>=0 && g->node[ref->node].rate>u->rate)BAD("DegreeToKey input rate exceeds calculation rate");
   if(!literal(g,u,2,&v) || fabsf(v)>16777216.0f)BAD("DegreeToKey octave must be a bounded literal constant");
  } else if(u->type==FM_In) {
   if(u->rate!=2 || u->inputs!=1 || u->outputs<1 || u->outputs>2) BAD("In requires one or two audio channels");
   if(literal(g,u,0,&v)){if(v!=0) BAD("Only input bus 0 is supported");}
   else {fm_ref *ref=&g->ref[u->input];if(g->node[ref->node].rate>1) BAD("In bus requires initial/control rate");}
  } else if(u->type==FM_Out) {
   if(u->rate!=2 || u->outputs || u->inputs<2 || u->inputs>3) BAD("Out requires one or two audio channels");
   if(literal(g,u,0,&v)){if(v!=0) BAD("Only output bus 0 is supported");}
   else {fm_ref *ref=&g->ref[u->input];if(g->node[ref->node].rate>1) BAD("Out bus requires initial/control rate");}
   for(j=1;j<u->inputs;j++) { fm_ref *ref=&g->ref[u->input+j]; if(ref->node<0 || g->node[ref->node].rate!=2) BAD("Out requires audio-rate input"); }
  } else {
   unsigned expected=1;
   if(u->type==FM_Pan2 || u->type==FM_FreeVerb2 || u->type==FM_MiVerb || u->type==FM_Balance2) expected=2;
   if(u->outputs!=expected) BAD("Invalid output count");
   switch(u->type) {
    case FM_Formant: n=3; break;
    case FM_Formlet: n=4; break;
    case FM_Gendy1: n=10; break;
    case FM_Gendy2: n=12; break;
    case FM_Gendy3: n=9; break;
    case FM_VarSaw: case FM_Amplitude: case FM_RHPF: case FM_BRF: n=3; break;
    case FM_SyncSaw: case FM_LFCub: case FM_LFPar: n=2; break;
    case FM_BPeakEQ: case FM_BLowShelf: case FM_BHiShelf: n=4; break;
    case FM_Fold: case FM_Wrap: case FM_PulseDivider: case FM_LinXFade2: case FM_Slew: case FM_MiMu: n=3; break;
    case FM_Trig: case FM_Trig1: case FM_PulseCount: case FM_SetResetFF: case FM_Lag2: case FM_Lag3: n=2; break;
    case FM_Stepper: n=6; break;
    case FM_LinExp: n=5; break;
    case FM_XFade2: case FM_Balance2: n=4; break;
    case FM_BrownNoise: case FM_ClipNoise: n=0; break;
    case FM_FreeVerb: n=4; break;
    case FM_FreeVerb2: n=5; break;
    case FM_MiVerb: n=u->inputs; if(n!=7 && n!=8) BAD("MiVerb requires mono or stereo input"); break;
    case FM_SampleRate: case FM_SampleDur: case FM_ControlRate: case FM_ControlDur: case FM_BlockSize: case FM_RadiansPerSample: n=0; break;
    case FM_Delay1: case FM_OnePole: case FM_OneZero: case FM_Latch: case FM_Gate: case FM_Impulse: case FM_Decay: n=2; break;
    case FM_TRand: case FM_TExpRand: case FM_Decay2: n=3; break;
    case FM_Delay2: n=3; break;
    case FM_Rand: case FM_ExpRand: n=2; break;
    case FM_DelayN: case FM_DelayL: case FM_DelayC: n=3; break;
    case FM_CombN: case FM_CombL: case FM_CombC: case FM_AllpassN: case FM_AllpassL: case FM_AllpassC: n=4; break;
    case FM_Pulse: case FM_RandSeed: case FM_SinOsc: case FM_LFSaw: case FM_LFTri: case FM_Lag: case FM_LPF: case FM_HPF: case FM_LeakDC: n=2; break;
    case FM_LFPulse: case FM_LagUD: case FM_RLPF: case FM_BPF: case FM_Clip: case FM_Pan2: case FM_MulAdd: case FM_Sum3: case FM_Resonz: case FM_Ringz: n=3; break;
    case FM_Sum4: case FM_Line: case FM_XLine: n=4; break;
    case FM_BinaryOpUGen: n=2; break;
    case FM_WhiteNoise: case FM_PinkNoise: n=0; break;
    case FM_Select: n=u->inputs; if(n<2) BAD("Select needs at least one choice"); break;
    case FM_EnvGen: n=u->inputs; if(n<13 || (n-9)%4) BAD("Malformed EnvGen input layout"); break;
    case FM_Klang: case FM_Klank:
     j=u->type==FM_Klang?2:4; n=u->inputs;
     if(n< (unsigned)(j+3) || (n-j)%3) BAD("Malformed partial bank");
     u->partial=g->partials; u->partials=(n-j)/3; g->partials+=u->partials;
     if(g->partials>FM_PARTIALS) BAD("Too many total partials");
     if(u->rate!=2) BAD("Partial banks require audio rate");
     break;
    default: n=1;
   }
   if(u->inputs!=n) BAD("Wrong input count");
   if(u->type>=FM_Fold && u->type<=FM_MiMu) {
    int control_begin=-1;
    if(u->type==FM_Trig || u->type==FM_Trig1 || u->type==FM_PulseDivider || u->type==FM_LinExp || u->type==FM_Slew || u->type==FM_Lag2 || u->type==FM_Lag3)control_begin=1;
    if(u->type==FM_Stepper)control_begin=2;
    if(u->type==FM_XFade2 || u->type==FM_Balance2)control_begin=3;
    if(u->type==FM_MiMu){control_begin=2;if(u->rate!=2)BAD("MiMu requires audio rate");}
    if(control_begin>=0)for(j=control_begin;j<u->inputs;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate>1)BAD("Parameter requires initial/control rate");}
    if(u->rate==2 && (u->type==FM_MiMu || u->type==FM_XFade2 || u->type==FM_LinXFade2 || u->type==FM_Balance2 || u->type==FM_PulseDivider || u->type==FM_PulseCount || u->type==FM_SetResetFF || u->type==FM_Stepper)) {
     int signals=(u->type==FM_XFade2 || u->type==FM_LinXFade2 || u->type==FM_Balance2)?2:1;
     for(j=0;j<signals;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node<0 || g->node[ref->node].rate!=2)BAD("Audio input required; use K2A or DC");}
    }
   }
   if(u->type>=FM_RHPF && u->type<=FM_BHiShelf){
    int begin=-1;
    if(u->type==FM_RHPF || u->type==FM_BRF || u->type==FM_Amplitude || u->type>=FM_BPeakEQ)begin=1;
    if(u->type==FM_VarSaw)begin=2;
    if(begin>=0)for(j=begin;j<u->inputs;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate>1)BAD("Filter/envelope/duty parameters require initial/control rate");}
    if(u->type==FM_VarSaw || u->type==FM_LFCub || u->type==FM_LFPar){fm_ref *ref=&g->ref[u->input+1];if(ref->node>=0 && g->node[ref->node].rate!=0)BAD("Oscillator phase requires initial rate");}
    if(u->type>=FM_BPeakEQ && u->rate!=2)BAD("B equalizers require audio rate");
   }
   if(fm_is_reverb(u->type)) {
    if(u->rate!=2) BAD("Reverbs require audio rate");
    for(j=0;j<u->inputs;j++) {
     int audio=u->type==FM_MiVerb?j>=6:j<(u->type==FM_FreeVerb2?2:1);
     fm_ref *ref=&g->ref[u->input+j];unsigned rate=ref->node<0?0:g->node[ref->node].rate;
     if((audio && rate!=2)||(!audio && rate>1)) BAD("Reverbs require audio input and initial/control parameters");
    }
   }
   if(u->type==FM_Formant || u->type==FM_Formlet) {
    if(u->type==FM_Formant && u->rate!=2)BAD("Formant requires audio rate");
    for(j=u->type==FM_Formant?0:1;j<u->inputs;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate>1)BAD("Formant/Formlet parameters require initial/control rate");}
    if(u->type==FM_Formlet && u->rate==2){fm_ref *ref=&g->ref[u->input];if(ref->node<0 || g->node[ref->node].rate!=2)BAD("Formlet.ar requires audio input");}
   }
   if(fm_is_gendy(u->type)) {
    unsigned cp=u->type==FM_Gendy3?7:8;
    if(!literal(g,u,cp,&v) || v<1 || v>FM_GENDY_POINTS || v!=(int)v) BAD("Gendy initCPs must be an integer literal in 1..64");
    for(j=0;j<u->inputs;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate>1)BAD("Gendy parameters require initial/control rate");}
   }
   if(fm_is_delay(u->type)) {
    if(!literal(g,u,1,&v) || v<0) BAD("Delay maxdelaytime must be a nonnegative literal");
    if(u->inputs==4) {fm_ref *ref=&g->ref[u->input+3];if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Delay decaytime requires initial/control rate");}
   }
   if(u->type==FM_BinaryOpUGen && !((u->special>=0 && u->special<=13) || (u->special>=17 && u->special<=25) || (u->special>=30 && u->special<=46))) BAD("Unsupported binary operator selector");
   if(u->type==FM_UnaryOpUGen && !(u->special==0 || u->special==5 || (u->special>=8 && u->special<=36) || u->special==42 || u->special==43)) BAD("Unsupported unary operator selector");
   if(u->type==FM_EnvGen) {
    for(j=1;j<u->inputs;j++) {fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Only the EnvGen gate may be audio rate in this profile");}
    if(!u->rate || !literal(g,u,6,&v) || v<1 || v>FM_SEGMENTS || v!=(int)v || u->inputs!=9+4*(int)v) BAD("EnvGen needs 1..32 literal segments");
    for(j=7;j<=8;j++) if(!literal(g,u,j,&w) || (w < 0 && w != -99 && w != -1) || w>=v || w!=(int)w) BAD("Invalid envelope release/loop node");
    if(!literal(g,u,4,&v) || (v!=0 && v!=2)) BAD("Only literal doneAction 0 and 2 supported");
    for(j=11;j<u->inputs;j+=4) if(!literal(g,u,j,&v) || v<0 || v>8 || v!=(int)v) BAD("Envelope curve shape must be literal 0..8");
   }
   if(u->type>=FM_SampleRate && u->type<=FM_RadiansPerSample && u->rate!=0) BAD("Information UGen requires initial rate");
   if((u->type==FM_Delay1 || u->type==FM_Delay2)){for(j=1;j<u->inputs;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Delay initial values require initial/control rate");}}
   if(u->type>=FM_LFSaw && !(u->type>=FM_SampleRate && u->type<=FM_RadiansPerSample) && u->type!=FM_Select && u->type!=FM_RandSeed && u->type!=FM_Rand && u->type!=FM_ExpRand && !u->rate) BAD("UGen requires audio/control rate");
   if((u->type==FM_Rand || u->type==FM_ExpRand) && u->rate!=0) BAD("Rand/ExpRand require initial rate");
   if(u->type==FM_OnePole || u->type==FM_OneZero || u->type==FM_Decay || u->type==FM_Decay2){
    for(j=1;j<u->inputs;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Filter coefficients/times require initial/control rate");}
   }
   if(u->type==FM_Dust || u->type==FM_Dust2){fm_ref *ref=&g->ref[u->input];if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Dust density requires initial/control rate");}
   if((u->type==FM_TRand || u->type==FM_TExpRand) && u->rate==2){fm_ref *ref=&g->ref[u->input+2];if(ref->node<0 || g->node[ref->node].rate!=2) BAD("Audio triggered random requires audio trigger; use K2A or DC");}
   if(u->type==FM_TRand || u->type==FM_TExpRand){for(j=0;j<2;j++){fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Triggered random bounds require initial/control rate");}}
   if(u->type==FM_Impulse){fm_ref *ref=&g->ref[u->input+1];if(ref->node>=0 && g->node[ref->node].rate!=0) BAD("Impulse phase requires initial rate in this profile");}
   if(u->type==FM_RandSeed && (!literal(g,u,1,&v) || v < -16777216 || v > 16777216 || v!=(int)v)) BAD("RandSeed requires an exact literal integer seed in +/-2^24");
   if(u->type==FM_Saw || u->type==FM_Pulse || u->type==FM_LFNoise0 || u->type==FM_LFNoise1 || u->type==FM_LFNoise2) {
    for(j=0;j<u->inputs;j++) {fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Band-limited oscillator and LFNoise parameters require initial/control rate");}
   }
   if(u->type==FM_Select && u->rate==2) {
    for(j=1;j<u->inputs;j++) {fm_ref *ref=&g->ref[u->input+j];if(ref->node<0 || g->node[ref->node].rate!=2) BAD("Audio Select requires audio-rate choices");}
   }
   if(u->type==FM_LFSaw || u->type==FM_LFTri || u->type==FM_LFPulse) {
    fm_ref *ref=&g->ref[u->input+1];if(ref->node>=0 && g->node[ref->node].rate!=0) BAD("LFO initial phase must be initial rate");
   }
   if(u->type==FM_Lag || u->type==FM_LagUD || u->type==FM_LPF || u->type==FM_HPF || u->type==FM_RLPF || u->type==FM_BPF || u->type==FM_LeakDC || u->type==FM_Resonz || u->type==FM_Ringz || u->type==FM_LFPulse) {
    int begin=u->type==FM_LFPulse?2:1;
    for(j=begin;j<u->inputs;j++) {fm_ref *ref=&g->ref[u->input+j];if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Filter/lag/duty parameters require control or initial rate");}
   }
   if(u->type==FM_Line || u->type==FM_XLine) {
    if(!u->rate || !literal(g,u,3,&v) || (v!=0 && v!=2)) BAD("Line needs audio/control rate and doneAction 0 or 2");
   }
   if((u->type==FM_SinOsc || u->type==FM_Pan2 || u->type==FM_Resonz || u->type==FM_Ringz) && !u->rate) BAD("UGen requires audio/control rate");
   if(u->type==FM_Pan2) { fm_ref *ref=&g->ref[u->input+2]; if(ref->node>=0 && g->node[ref->node].rate==2) BAD("Pan2 level requires control or initial rate"); }
   if(u->type==FM_K2A && u->rate!=2) BAD("K2A requires audio rate");
   if(u->type==FM_K2A) { fm_ref *ref=&g->ref[u->input]; if(ref->node>=0 && g->node[ref->node].rate>1) BAD("K2A requires control or initial input"); }
   if(u->type==FM_A2K && u->rate!=1) BAD("A2K requires control rate");
   /* Fixed banks sample their parameters at initialization. */
   if(u->type==FM_Klang || u->type==FM_Klank) {
    int begin=u->type==FM_Klang?0:1;
    for(j=begin;j<u->inputs;j++) { fm_ref *ref=&g->ref[u->input+j]; if(ref->node>=0 && g->node[ref->node].rate!=0) BAD("Resonator/bank parameters currently require initial rate"); }
   }
   for(j=0;j<u->inputs;j++) {
    fm_ref *ref=&g->ref[u->input+j]; int rate=ref->node<0?0:g->node[ref->node].rate;
    if(u->type==FM_Rand || u->type==FM_ExpRand){if(rate>1) BAD("Rand/ExpRand bounds require initial/control rate");}
    else if(u->type!=FM_A2K && !(u->type==FM_Amplitude && j==0) && rate>u->rate) BAD("Input rate exceeds calculation rate");
   }
  }
 }
 /* LocalBuf values are private table references, not general server buffer IDs. */
 for(i=0;i<g->nodes;i++)for(j=0;j<g->node[i].inputs;j++){
  const fm_node *consumer=&g->node[i];const fm_ref *ref=&g->ref[consumer->input+j];
  if(ref->node>=0 && g->node[ref->node].type==FM_LocalBuf && !(j==0 && (consumer->type==FM_SetBuf || consumer->type==FM_DegreeToKey)))BAD("LocalBuf references may only feed SetBuf or DegreeToKey");
 }
 for(i=0;i<g->nodes;i++)if(g->node[i].type==FM_LocalBuf && g->node[i].partial==UINT16_MAX)BAD("LocalBuf requires a complete constant SetBuf initializer");
 if(rd(&r,2)!=0) BAD("SynthDef variants are not supported yet");
 if(r.failed || r.left) BAD("Truncated file or trailing bytes");
 g->valid=FM_VALID; return 1;
 #undef BAD
}
