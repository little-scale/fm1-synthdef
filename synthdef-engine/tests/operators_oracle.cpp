/* GPL-3.0-or-later. Pinned SC formulas plus standard-library transcendental oracle. */
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "SC_InlineUnaryOp.h"
#include "SC_InlineBinaryOp.h"
extern "C" float op_unary(int,float);
extern "C" float op_binary(int,float,float);
static float reference_unary(int op,float a){switch(op){case 23:return sc_octcps(a);case 24:return sc_cpsoct(a);case 32:return std::acos(a);case 33:return std::atan(a);case 34:return std::sinh(a);case 35:return std::cosh(a);case 36:return std::tanh(a);default:return NAN;}}
static float reference_binary(int op,float a,float b){switch(op){
case 3:return std::floor(a/b);case 17:return sc_lcm(a,b);case 18:return sc_gcd(a,b);case 22:return sc_atan2(a,b);case 23:return std::hypot(a,b);case 24:return sc_hypotx(a,b);case 25:return sc_pow(a,b);
case 30:return sc_ring1(a,b);case 31:return sc_ring2(a,b);case 32:return sc_ring3(a,b);case 33:return sc_ring4(a,b);case 34:return a*a-b*b;case 35:return a*a+b*b;case 36:return (a+b)*(a+b);case 37:return (a-b)*(a-b);case 38:return std::abs(a-b);case 39:return sc_thresh(a,b);case 40:return sc_amclip(a,b);
/* The UGen uses this conditional; SC_InlineBinaryOp's float sc_scaleneg has different semantics. */
case 41:return a>=0?a:a*b;case 43:return sc_excess(a,b);case 46:return a;default:return NAN;}}
int main(){
 unsigned checks=0,failures=0;double max_scaled=0;
 auto check=[&](float a,float b){double error=std::abs((double)a-b)/(1+std::abs((double)b));checks++;max_scaled=std::max(max_scaled,error);if(!std::isfinite(a)||!std::isfinite(b)||error>0.00002){if(failures<5)std::fprintf(stderr,"Mismatch %.9g %.9g scaled %.9g\n",a,b,error);failures++;}};
 for(int op:{23,24,32,33,34,35,36})for(int i=0;i<=65536;i++){
  float x=(float)i/65536,a=op==32?2*x-1:op==24?std::exp2(24*x-12):op==23?24*x-12:op==33?2000*x-1000:40*x-20;
  check(op_unary(op,a),reference_unary(op,a));
 }
 unsigned seed=0x12345678;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return (float)(seed&0xffffff)/16777216.0f;};
 for(int op:{3,17,18,22,23,24,25,30,31,32,33,34,35,36,37,38,39,40,41,43,46})for(int i=0;i<65536;i++){
  float a=random()*32-16,b=random()*8-4;if(b==0)b=0.125f;if(op==25 && std::abs(a)<.01f)a=.01f;if(op==43)b=std::abs(b);
  check(op_binary(op,a,b),reference_binary(op,a,b));
 }
 for(float a:{14930352.0f,9227465.0f,-14930352.0f,-9227465.0f,16777216.0f,0.0f})for(float b:{14930352.0f,9227465.0f,-14930352.0f,-9227465.0f,16777216.0f,0.0f})for(int op:{17,18})check(op_binary(op,a,b),reference_binary(op,a,b));
 for(float a:{-0.0f,0.0f,-1.0f,1.0f})for(float b:{-0.0f,0.0f,-1.0f,1.0f})check(op_binary(22,a,b),reference_binary(22,a,b));
 std::printf("{\"comparisons\":%u,\"failures\":%u,\"max_scaled_error\":%.9g,\"scaled_error_limit\":0.00002}\n",checks,failures,max_scaled);
 return failures?1:0;
}
