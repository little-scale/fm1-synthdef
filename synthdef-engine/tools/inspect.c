/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "fm_synth.h"
#include <stdio.h>
static fm_graph graph;
static unsigned char bytes[FM_FILE_BYTES+1];
int main(int argc,char **argv) {
 FILE *f;size_t n;char error[256];unsigned i;
 if(argc!=2){fprintf(stderr,"Usage: fm-inspect file.scsyndef\n");return 2;}
 f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 1;}n=fread(bytes,1,sizeof bytes,f);fclose(f);
 if(!fm_load(&graph,bytes,n,error,sizeof error)){fprintf(stderr,"Rejected: %s\n",error);return 1;}
 printf("%s: accepted by current engine profile\n%u nodes, %u signals, %u controls, %u partials\n",graph.name,graph.nodes,graph.outputs,graph.controls,graph.partials);
 for(i=0;i<graph.names;i++)printf("control %s = %.7g\n",graph.control_name[i].name,(double)graph.defaults[graph.control_name[i].index]);
 for(i=0;i<graph.nodes;i++){fm_node *u=&graph.node[i];printf("%2u  %-14s .%s  inputs=%u outputs=%u special=%d\n",i,fm_type_name(u->type),u->rate==2?"ar":u->rate==1?"kr":"ir",u->inputs,u->outputs,u->special);}
 printf("Memory: graph=%zu voice=%zu shared scratch=%zu bytes\n",sizeof(fm_graph),sizeof(fm_voice),sizeof(fm_scratch));return 0;
}
