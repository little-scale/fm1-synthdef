/* SPDX-License-Identifier: GPL-3.0-or-later
 * Exercise the device-side ADC ownership contract and indicator thresholds. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static struct {int config,suspended;} usb;
static struct {uint16_t con;int pend;} fm1_an;
static uint32_t now;static int locked,reads,reading;
static uint32_t plat_ms(void){return now;}
static uint32_t plat_lock(void){assert(!locked);locked=1;return 0;}
static void plat_unlock(uint32_t f){(void)f;assert(locked);locked=0;}
#define FM1_ADC_BATT 3
static int fm1_adc_read(int channel){assert(channel==3 && !fm1_an.con && !fm1_an.pend && !locked);reads++;return reading;}
#define C_BLACK 0
#define C_WHITE 65535
static int FONT_XS;
static void cv_rect(int x,int y,int w,int h,int c){(void)x;(void)y;(void)w;(void)h;(void)c;}
static void cv_text(int x,int y,const int *font,const char *s,int c){(void)x;(void)y;(void)font;(void)s;(void)c;}
#include "../firmware/battery.h"
int main(void){
 fm1_an.con=123;fm1_an.pend=1;reading=610;
 battery_poll();assert(reads==0 && fm1_an.con==123 && battery_bars==-1);
 fm1_an.pend=0;battery_poll();assert(reads==1 && fm1_an.con==123 && battery_bars==3 && !locked);
 now=999;battery_poll();assert(reads==1);
 now=1000;reading=500;battery_poll();assert(reads==2 && battery_raw==583 && battery_bars==2);
 usb.config=1;battery_poll();assert(battery_usb && reads==2);
 usb.suspended=1;battery_poll();assert(!battery_usb && reads==2);
 for(int raw=500;raw<=620;raw++){
  now+=1000;reading=raw;battery_raw=-1;battery_bars=-1;
  battery_poll();assert(battery_bars==(raw>=531)+(raw>=561)+(raw>=591));
  battery_draw();
 }
 /* Averaged readings inside the hysteresis band must not chatter. */
 battery_raw=591;battery_bars=2;reading=591;now+=1000;battery_poll();assert(battery_bars==2);
 battery_bars=3;reading=590;now+=1000;battery_poll();assert(battery_bars==3);
 puts("Battery: ADC ownership, scan restoration, polling, USB state, thresholds and hysteresis passed");
}
