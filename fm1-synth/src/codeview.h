/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FMS_CODEVIEW_H
#define FMS_CODEVIEW_H
#include <stdint.h>
enum { FMS_TEXT,FMS_KEYWORD,FMS_CLASS,FMS_NUMBER,FMS_STRING,FMS_COMMENT };
typedef void (*fms_glyph)(void *,unsigned row,unsigned column,char glyph,unsigned color);
unsigned fms_code(const char *,uint32_t,unsigned scroll,unsigned rows,unsigned columns,fms_glyph,void *);
#endif
