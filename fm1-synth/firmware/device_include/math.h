/* SPDX-License-Identifier: MPL-2.0
 * Polynomial kernels Copyright (C) 2026 Charles Vestal.
 * Freestanding float backend; polynomial kernels from fm1-base fastmath.h (MPL-2.0).
 * Invalid domains return NaN for the engine's per-voice fault policy. No doubles.
 */
#pragma once
#include "fastmath.h"
static inline float fs_value(uint32_t u){fm_bits_t b;b.u=u;return b.f;}
static inline int fs_finite(float x){fm_bits_t b;b.f=x;return (b.u&0x7f800000u)!=0x7f800000u;}
#define isfinite(x) fs_finite(x)
#define NAN (fs_value(0x7fc00000u))
#define INFINITY (fs_value(0x7f800000u))
static inline float fabsf(float x){return fm_fabsf(x);}
static inline float fminf(float a,float b){return a!=a?b:b!=b?a:a<b?a:b;}
static inline float fmaxf(float a,float b){return a!=a?b:b!=b?a:a>b?a:b;}
static inline float floorf(float x){if(!isfinite(x)||fabsf(x)>=8388608)return x;return fm_floorf(x);}
static inline float ceilf(float x){return -floorf(-x);}
static inline float fmodf(float x,float y){
 float a=fabsf(x),b=fabsf(y),t;unsigned steps=0;
 if(!isfinite(x)||y!=y||!b)return NAN;if(!isfinite(y)||a<b)return x;
 t=b;while(t<=a*0.5f && steps<255){t*=2;steps++;}
 for(;;){if(a>=t)a-=t;if(!steps)break;steps--;t*=0.5f;}
 return x<0?-a:a;
}
static inline float exp2f(float x){
 float n,f,p;fm_bits_t b;
 if(x!=x)return x;if(x < -126)return 0;if(x>127)return INFINITY;
 n=floorf(x+0.5f);f=x-n;
 p=1.534580891e-4f;p=p*f+1.339993158e-3f;p=p*f+9.618488972e-3f;
 p=p*f+5.550328776e-2f;p=p*f+2.402264689e-1f;p=p*f+6.931472057e-1f;p=p*f+1;
 b.f=p;b.u+=(uint32_t)(int32_t)n*0x800000u;return b.f;
}
static inline float expf(float x){
 float n,r;
 if(x!=x)return x;if(x < -87.3f)return 0;if(x>88.7f)return INFINITY;
 n=floorf(x*FM_LOG2E+0.5f);r=(x-n*0.693145752f)-n*1.42860677e-6f;
 return exp2f(n)*exp2f(r*FM_LOG2E);
}
static inline float expm1f(float x){
 if(fabsf(x)<0.25f)return x*(1+x*(0.5f+x*(0.166666667f+x*(0.0416666667f+x*(0.00833333333f+x*(0.00138888889f+x*0.000198412698f))))));
 return expf(x)-1;
}
static inline float log2f(float x){if(x!=x||x<0)return NAN;if(x==0)return -INFINITY;if(!isfinite(x))return x;if(x<1.17549435e-38f)return fm_log2f(x*16777216)-24;return fm_log2f(x);}
static inline float logf(float x){return log2f(x)*FM_LN2;}
static inline float log10f(float x){return log2f(x)*0.301029995664f;}
static inline float powf(float x,float y){if(y==0)return 1;if(x==0)return y>0?0:INFINITY;if(x<0)return NAN;return exp2f(y*log2f(x));}
static inline float sqrtf(float x){if(x!=x||x<0)return NAN;if(x==0||!isfinite(x))return x;if(x<1.17549435e-38f)return fm_sqrtf(x*16777216)*0.000244140625f;return fm_sqrtf(x);}
static inline float cbrtf(float x){float y,a=fabsf(x);unsigned i;if(!a||!isfinite(x))return x;y=exp2f(log2f(a)/3);for(i=0;i<2;i++)y=(2*y+a/(y*y))/3;return x<0?-y:y;}
static inline float fs_angle(float x){if(!isfinite(x))return NAN;return fabsf(x)>100?fmodf(x,FM_TWO_PI):x;}
static inline float sinf(float x){x=fs_angle(x);return isfinite(x)?fm_sinf(x):NAN;}
static inline float cosf(float x){
 float r,z,p,sign=1;x=fs_angle(x);if(!isfinite(x))return NAN;
 r=fabsf(fm_wrap_pi(x));if(r>1.57079632679f){r=(3.140625f-r)+0.000967653589793f;sign=-1;}
 z=r*r;p=-2.75573143e-7f;p=p*z+2.48015873e-5f;p=p*z-0.00138888889f;p=p*z+0.0416666667f;
 return sign*(1-(0.5f*z-z*z*p));
}
static inline float tanf(float x){return sinf(x)/cosf(x);}
static inline float asinf(float x){
 float a=fabsf(x),z,p,r;if(a>1||!isfinite(x))return NAN;
 /* asin(x)=x+x^3 P(x^2); half-angle reduces the polynomial to |x|<=0.5. */
 z=a>0.5f?sqrtf((1-a)*0.5f):a;
 p=0.0303819444f;p=p*z*z+0.0446428571f;p=p*z*z+0.075f;p=p*z*z+0.166666667f;
 r=z+z*z*z*p;if(a>0.5f)r=1.57079632679f-2*r;return x<0?-r:r;
}
/* Reduced-domain odd series; no double arithmetic or libm dependency. */
static inline float atanf(float x){
 float a=fabsf(x),offset=0.0f;int inverse=a>1.0f;
 if(inverse)a=1.0f/a;
 if(a>0.414213562373095f){a=(a-1.0f)/(a+1.0f);offset=0.7853981633974483f;}
 float q=a*a,p=1.0f/17.0f;
 p=-1.0f/15.0f+q*p;p=1.0f/13.0f+q*p;p=-1.0f/11.0f+q*p;
 p=1.0f/9.0f+q*p;p=-1.0f/7.0f+q*p;p=1.0f/5.0f+q*p;p=-1.0f/3.0f+q*p;
 float y=offset+a*(1.0f+q*p);if(inverse)y=1.5707963267948966f-y;
 return x<0.0f?-y:y;
}
static inline float atan2f(float y,float x){
 union{float f;uint32_t u;} sy={y},sx={x};
 if(x!=x||y!=y)return NAN;
 if(y==0.0f)return sx.u>>31?(sy.u>>31?-3.1415926535897932f:3.1415926535897932f):y;
 if(x==0.0f)return y<0.0f?-1.5707963267948966f:1.5707963267948966f;
 float a=atanf(fabsf(y/x));if(x<0.0f)a=3.1415926535897932f-a;
 return y<0.0f?-a:a;
}
static inline float acosf(float x){if(fabsf(x)>1.0f||x!=x)return NAN;return atan2f(sqrtf((1.0f-x)*(1.0f+x)),x);}
static inline float sinhf(float x){float a=expm1f(fabsf(x));float y=0.5f*(a+a/(a+1.0f));return x<0.0f?-y:y;}
static inline float coshf(float x){float a=expf(fabsf(x));return 0.5f*(a+1.0f/a);}
static inline float tanhf(float x){float a=fabsf(x);if(a>10.0f)return x<0.0f?-1.0f:1.0f;float e=expm1f(-2.0f*a),y=-e/(2.0f+e);return x<0.0f?-y:y;}
static inline float hypotf(float x,float y){float a=fmaxf(fabsf(x),fabsf(y)),b=fminf(fabsf(x),fabsf(y));if(a==0.0f)return 0.0f;float q=b/a;return a*sqrtf(1.0f+q*q);}
