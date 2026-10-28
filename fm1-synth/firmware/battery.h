/* SPDX-License-Identifier: GPL-3.0-or-later
 * Stock ADC thresholds documented by fm1-base's plat_battery implementation:
 * https://github.com/charlesvestal/fm1-base/blob/main/device/plat_fm1.c
 * This adapter keeps the pinned platform unchanged. No percentage or charging
 * claim: the board exposes a voltage divider, not a fuel gauge/charger status.
 */
static int battery_raw=-1,battery_bars=-1,battery_usb;
static uint32_t battery_at,battery_samples;
static int battery_poll_enabled=1;
static void battery_poll(void){
 int sample=-1;uint32_t now=plat_ms();
#ifdef PLAT_HOST
 const char *raw=getenv("FMS_BATTERY_RAW"),*power=getenv("FMS_BATTERY_USB");
 battery_usb=power && atoi(power)!=0;
#else
 battery_usb=usb.config && !usb.suspended;
#endif
 if(!battery_poll_enabled || (battery_raw>=0 && now-battery_at<1000u))return;
#ifdef PLAT_HOST
 if(raw)sample=atoi(raw); /* explicit simulator fixture; absent means unknown */
#else
 /* The scan leaves spare ADC slots at the end of its probe frame. Claim only
  * an idle converter; never interrupt a key or MASTER conversion. Audio and
  * the digital key scan remain enabled during the bounded ADC read. */
 uint32_t flags=plat_lock();
 uint16_t scan=fm1_an.con;
 if(fm1_an.pend){plat_unlock(flags);return;}
 fm1_an.con=0;plat_unlock(flags);
 sample=fm1_adc_read(FM1_ADC_BATT);
 flags=plat_lock();fm1_an.con=scan;plat_unlock(flags);
#endif
 if(sample<=0 || sample>1023)return;
 battery_at=now;battery_samples++;
 battery_raw=battery_raw<0?sample:(3*battery_raw+sample+2)/4;
 {static const int thresholds[3]={531,561,591};
 if(battery_bars<0)battery_bars=(battery_raw>=531)+(battery_raw>=561)+(battery_raw>=591);
 while(battery_bars<3 && battery_raw>=thresholds[battery_bars]+4)battery_bars++;
 while(battery_bars>0 && battery_raw<thresholds[battery_bars-1]-4)battery_bars--;
 }
}
static void battery_draw(void){
 unsigned i;
 /* Dedicated top-right area. The plug means USB host power, not charging. */
 if(battery_usb){cv_rect(196,5,2,3,C_BLACK);cv_rect(201,5,2,3,C_BLACK);cv_rect(195,8,9,4,C_BLACK);cv_rect(199,12,2,4,C_BLACK);}
 cv_rect(210,4,24,12,C_BLACK);cv_rect(211,5,22,10,C_WHITE);cv_rect(234,8,2,4,C_BLACK);
 if(battery_bars<0)cv_text(218,4,&FONT_XS,"?",C_BLACK);
 else for(i=0;i<(unsigned)battery_bars;i++)cv_rect(213+(int)i*6,7,4,6,C_BLACK);
}
