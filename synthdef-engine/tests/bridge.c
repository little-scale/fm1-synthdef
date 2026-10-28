/* Test-only C ABI: keep Python independent of struct layout. */
#include "fm_synth.h"
size_t test_graph_size(void) {return sizeof(fm_graph);}
size_t test_voice_size(void) {return sizeof(fm_voice);}
size_t test_scratch_size(void) {return sizeof(fm_scratch);}
int test_active(fm_voice *v) {return v->active;}
int test_fault(fm_voice *v) {return v->fault;}
