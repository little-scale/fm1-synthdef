/* GPL-3.0-or-later: test adapter for the pinned, unmodified upstream MiMu. */
#include "../reference/mi-ugens/projects/MiMu/MiMu.cpp"
extern "C" float oracle_mu(float input,float gain){
 float x=SoftClip(input*gain);int16_t pcm=x*32767.0;
 return MuLaw2Lin(Lin2MuLaw(pcm))*(1.0f/32767.0f);
}
