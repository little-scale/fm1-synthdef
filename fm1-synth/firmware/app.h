/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "plat.h"
#define APP_NAME "SYNTHDEF"
#define APP_SPLASH_RGB RGB(255,255,255)
#define APP_BLOCK 256u
#define APP_CANVAS_PX (240u*24u)
#define APP_TOUCH
enum { OBJ_SETTINGS,OBJ_PANEL,OBJ_BLE,OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u,0xDC000u,0xDE000u}
void app_init(void);
void app_render(int32_t *,uint32_t,uint32_t);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *,const char *);
#define APP_HOST_EXPECT
int app_expect(const char *,int *);

#define APP_SYSEX
int app_sysex(const uint8_t *,uint32_t);
#define APP_OVERLOAD
void app_overload(void);
