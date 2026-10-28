/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FMS_PROTOCOL_H
#define FMS_PROTOCOL_H
#include "slots.h"
#define FMS_CHUNK 192u
#define FMS_FRAME 256u
enum { FMS_INFO=0,FMS_READ=1,FMS_BEGIN=2,FMS_WRITE=3,FMS_COMMIT=4,FMS_ABORT=5 };
enum { FMS_OK=0,FMS_BAD=1,FMS_CONFLICT=2,FMS_IO=3,FMS_ORDER=4,FMS_INVALID=5 };
typedef struct {
 fms_flash flash;
 int (*validate)(unsigned slot,const void *,uint32_t); /* non-audio graph validation, 1 = valid */
 uint8_t stage[FMS_PAYLOAD],scratch[FMS_PAYLOAD];
 uint32_t transaction,expected,length,crc,received;
 unsigned slot;int active;
 uint8_t last_request[FMS_FRAME],last_reply[FMS_FRAME];
 uint32_t last_request_len,last_reply_len;
} fms_link;
/* Decoded packets: version, command, request ID (u16), length (u16), payload,
 * CRC32(all preceding bytes). Reply command has bit 7; first payload byte status.
 * Caller serializes access, supplies >=FMS_FRAME output and a valid flash adapter.
 * An exact immediate retry returns the cached reply, never repeats a commit. */
uint32_t fms_request(fms_link *,const uint8_t *,uint32_t,uint8_t *);
#endif
