/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fm_synth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static fm_graph graph;
static fm_voice voice;
static fm_scratch scratch;
static float delay_memory[FM_DELAY_LIMIT];
static unsigned char bytes[FM_FILE_BYTES+1];
int main(int argc,char **argv) {
 FILE *f;size_t size;char err[256];unsigned i,blocks;
 if(argc<3){fprintf(stderr,"Usage: fm-render file.scsyndef output.f32 [blocks=345] [gate-event-block value]...\n");return 2;}
 f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 1;}
 size=fread(bytes,1,sizeof bytes,f);fclose(f);
 if(!fm_load(&graph,bytes,size,err,sizeof err)){fprintf(stderr,"%s\n",err);return 1;}
 if(!fm_start(&voice,&graph,44100))return 1;
 if(!fm_bind_delay(&voice,&graph,delay_memory,FM_DELAY_LIMIT)){fprintf(stderr,"Delay memory budget exceeded\n");return 1;}
 blocks=argc>3?(unsigned)strtoul(argv[3],NULL,10):345;
 if(blocks>1000000)return 2;
 f=fopen(argv[2],"wb");if(!f){perror(argv[2]);return 1;}
 for(i=0;i<blocks;i++) {
  float out[FM_BLOCK*2]={0};int j;
  for(j=4;j+1<argc;j+=2)if(i==(unsigned)strtoul(argv[j],NULL,10))fm_set(&voice,&graph,"gate",strtof(argv[j+1],NULL));
  fm_render(&graph,&voice,&scratch,out);
  if(fwrite(out,sizeof(float),FM_BLOCK*2,f)!=FM_BLOCK*2){fclose(f);return 1;}
 }
 if(fclose(f))return 1;
 printf("%s: %u nodes, %u controls, %u partials; graph %zu, voice %zu, scratch %zu bytes; fault=%u\n",graph.name,graph.nodes,graph.controls,graph.partials,sizeof graph,sizeof voice,sizeof scratch,voice.fault);
 return voice.fault?1:0;
}
