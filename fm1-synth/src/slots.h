/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FMS_SLOTS_H
#define FMS_SLOTS_H
#include <stdint.h>
#define FMS_SYNTH_SLOTS 8u
#define FMS_FX_SLOT 8u /* first FX storage index; displayed as FX1 */
#define FMS_FX_PRESETS 4u
#define FMS_SLOTS (FMS_SYNTH_SLOTS+FMS_FX_PRESETS)
#define FMS_FX_BASE 0xD7000u
#define FMS_FX_BANK 8192u
#define FMS_FX_PAYLOAD (FMS_FX_BANK-256u)
#define FMS_FLASH_BASE 0x97000u
#define FMS_BANK 16384u
#define FMS_PAYLOAD (FMS_BANK-256u)
#define FMS_FLASH_END (FMS_FX_BASE+2u*FMS_FX_BANK)
static inline uint32_t fms_slot_capacity(unsigned slot){return slot>=FMS_FX_SLOT?FMS_FX_PAYLOAD:FMS_PAYLOAD;}
typedef struct {
 void *context;
 int (*read)(void *,uint32_t,void *,uint32_t);
 int (*erase)(void *,uint32_t); /* one aligned 4096-byte sector */
 int (*program)(void *,uint32_t,const void *,uint32_t); /* <=256 bytes, no page crossing */
} fms_flash;
/* 0 on success, -1 on I/O/invalid, -2 conflict, -3 shared FX capacity exceeded. Empty slot is length 0.
 * Caller serializes all operations and supplies scratch >=FMS_PAYLOAD.
 * Generation 0 means never saved; UINT32_MAX is reserved, never wraps. */
int fms_slot_read(const fms_flash *,unsigned,void *,uint32_t *,uint32_t *generation);
int fms_slot_write(const fms_flash *,unsigned,const void *,uint32_t,uint32_t expected,void *scratch);
uint32_t fms_crc(const void *,uint32_t);
#endif
