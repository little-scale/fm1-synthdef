#ifdef PLAT_HOST
#include_next <stdio.h>
#else
#pragma once
#include <stddef.h>
int snprintf(char *,size_t,const char *,...);

#endif
