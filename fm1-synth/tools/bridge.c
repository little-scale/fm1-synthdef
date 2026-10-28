/* Host-only FFI, no structure-layout duplication in the Python tools. */
#include "performance.h"
size_t fms_graph_size(void){return sizeof(fm_graph);}
size_t fms_voice_size(void){return sizeof(fm_voice);}
size_t fms_performance_size(void){return sizeof(fms_performance);}
const char *fms_name(const fm_graph *g){return g->name;}
unsigned fms_controls(const fm_graph *g){return g->names;}
const char *fms_control_name(const fm_graph *g,unsigned i){return i<g->names?g->control_name[i].name:"";}
float fms_control_default(const fm_graph *g,unsigned i){return i<g->names?g->defaults[g->control_name[i].index]:0;}
unsigned fms_nodes(const fm_graph *g){return g->nodes;}
unsigned fms_faults(const fms_performance *p){return p->faults;}
