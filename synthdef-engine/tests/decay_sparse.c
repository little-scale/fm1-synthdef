/* SPDX-License-Identifier: GPL-3.0-or-later
 * Long sparse decays compared with a double-precision recurrence, using the
 * same rounded coefficient so this isolates state-update error. */
#include "../src/fm_render.c"
#include <stdio.h>
int main(void){
 const float times[]={0,0.000001f,0.01f,0.1f,1,30,100000};
 const float rates[]={8000,44100,192000};unsigned t,r,i;double worst=0;
 for(r=0;r<3;r++)for(t=0;t<7;t++){
  float hi=0,lo=0,grow=decay_grow(times[t],rates[r]);double oracle=0;
  for(i=0;i<300000;i++){
   float x=i%17011==0?(i%34022==0?0.7f:-0.3f):0;
   double relative;oracle=(double)x+(1.0+(double)grow)*oracle;
   decay_tick(&hi,&lo,x,grow);relative=fabs((double)hi-oracle)/fmax(1.0,fabs(oracle));
   if(relative>worst)worst=relative;
   if(!isfinite(hi) || relative>0.000001){fprintf(stderr,"sparse decay mismatch rate=%g time=%g sample=%u error=%g\n",(double)rates[r],(double)times[t],i,relative);return 1;}
  }
 }
 printf("Sparse decay: 21 long runs, 6,300,000 samples; maximum normalized error %.9g\n",worst);return 0;
}
