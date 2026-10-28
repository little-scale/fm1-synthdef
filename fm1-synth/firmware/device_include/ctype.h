#ifdef PLAT_HOST
#include_next <ctype.h>
#else
#pragma once
static inline int isdigit(int x){return x>='0' && x<='9';}
static inline int isupper(int x){return x>='A' && x<='Z';}
static inline int isalpha(int x){return isupper(x)||(x>='a' && x<='z');}
static inline int isalnum(int x){return isalpha(x)||isdigit(x);}

#endif
