/* C adaptation of Emilie Gillet's Rings (MIT); see reference/mi-rings/LICENSE.
 * State copied with memcpy, rings accessed as floats/bytes: no arena aliasing. */
typedef struct {float g,r,h,s1,s2;} rg_svf;
typedef struct {float lp,bp,f,damp;} rg_naive;
typedef struct {float coefficient,y1,y0;} rg_cos;
typedef struct {
 rg_svf excite,pluck;
 float note,history[4],coefficient,fast,slow,lag,dc_x,dc_y,peak,pluck_period,pluck_gain;
 uint32_t rng,pluck_write,pluck_remaining,previous_trigger;
} rg_common;
typedef struct {rg_svf filter[64];float position;} rg_modal;
typedef struct {
 rg_naive low,high;float attack[3],decay[3],detector[3],centroid;
 float carrier,modulator,brightness,feedback,amp_env,bright_env,gain,fm,previous;
 uint32_t carrier_phase,modulator_phase;
} rg_fm;
typedef struct {
 float frequency,delay,position,dispersion,compensation,dispersion_noise,phase,out[2],aux[2],bridge;
 float fir_x,fir_xx,fir_bright,fir_damp,dc_x,dc_y;
 rg_svf filter;rg_cos lfo;uint32_t write,stretch_write;
} rg_string;
typedef struct {rg_cos lfo[2];float lp[2];uint32_t write;} rg_reverb;
#define RG_REVERB_SAMPLES 21632u /* 21627 used slots, rounded to LFO tick period. */
#define RG_WORDS(T) ((uint32_t)((sizeof(T)+3u)/4u))
static unsigned rings_capacity(unsigned model,unsigned compact){return compact&&(model==2||model==5)?960u:2048u;}
static uint32_t rings_words(unsigned model,unsigned compact){
 uint32_t n=RG_WORDS(rg_common);
 if(model==1)return n+RG_WORDS(rg_modal);
 if(model==4)return n+RG_WORDS(rg_fm);
 unsigned strings=model==2||model==5?8:1;
 n+=256+strings*(RG_WORDS(rg_string)+rings_capacity(model,compact)+(strings==1?1024:0));
 if(model==6)n+=RG_WORDS(rg_reverb)+RG_REVERB_SAMPLES/2;
 return n;
}
