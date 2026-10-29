/* Test-only bridge; not included in firmware. */
#include "fm_synth.h"
typedef struct {fm_graph g;fm_voice v;fm_scratch b;float memory[4*FM_GENDY_POINTS+2];} gendy_probe;
#ifdef __cplusplus
extern "C" {
#endif
void gp_init(gendy_probe *,int type,unsigned cp,float sr,const float *params);
void gp_step(gendy_probe *,float sr,const float *params,unsigned count);
float gp_distribution(int,float,float);
#ifdef __cplusplus
}
#endif
