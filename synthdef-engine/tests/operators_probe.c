/* GPL-3.0-or-later. Test adapter for the exact kernels used by rendering. */
#include "../src/fm_render.c"
float op_unary(int op,float a){return unary(op,a);}
float op_binary(int op,float a,float b){return binary(op,a,b);}
