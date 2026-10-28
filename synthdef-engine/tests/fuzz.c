/* Deterministic malformed-file exercise under ASan/UBSan; not exhaustive fuzzing. */
#include "fm_synth.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static fm_graph graph;
static fm_voice voice;
static fm_scratch scratch;
static float delay_memory[FM_DELAY_LIMIT];
static unsigned char original[FM_FILE_BYTES],data[FM_FILE_BYTES];
static uint32_t rng=0x391061a1;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(int argc,char **argv) {
 FILE *f;size_t n,i;int iteration;char error[256];
 assert(argc==2);f=fopen(argv[1],"rb");assert(f);n=fread(original,1,sizeof original,f);fclose(f);assert(n>0);
 for(iteration=0;iteration<20000;iteration++) {
  size_t length=n;unsigned changes=1+random32()%8;
  memcpy(data,original,n);
  if(iteration%3==0)length=random32()%n;
  for(i=0;i<changes;i++)data[random32()%n]=(unsigned char)random32();
  if(fm_load(&graph,data,length,error,sizeof error)) {
   unsigned b;assert(fm_start(&voice,&graph,44100));
   if(!fm_bind_delay(&voice,&graph,delay_memory,FM_DELAY_LIMIT))continue;
   for(b=0;b<4;b++) {float output[128]={0};fm_render(&graph,&voice,&scratch,output);for(i=0;i<128;i++)assert(isfinite(output[i]));}
  } else assert(!fm_start(&voice,&graph,44100));
 }
 puts("20,000 malformed/truncated graphs: sanitizer run passed");return 0;
}
