#ifdef PLAT_HOST
#include_next <string.h>
#else
#pragma once
#include <stddef.h>
void *fs_memcpy(void *,const void *,unsigned);
#define memcpy fs_memcpy
void *fs_memset(void *,int,unsigned);
#define memset fs_memset
int memcmp(const void *,const void *,unsigned);
size_t strlen(const char *);
int strcmp(const char *,const char *);
char *strcpy(char *,const char *);

#endif
