/* Device memory helper alignment and bounds checks, on a host with sanitizers. */
#include <stdint.h>
#include <assert.h>
extern void *fs_memset(void *,int,unsigned);
extern void *fs_memcpy(void *,const void *,unsigned);
static uint8_t data[40016],source[40016];
int main(void){
 unsigned n,a,b,i;
 for(i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*17);
 for(n=0;n<=40000;n=n<64?n+1:n+997)for(a=0;a<4;a++)for(b=0;b<4;b++){
  for(i=0;i<sizeof data;i++)data[i]=0xa5;
  fs_memset(data+4+a,0x37,n);
  for(i=0;i<sizeof data;i++)assert(data[i]==(i>=4+a && i<4+a+n?0x37:0xa5));
  fs_memcpy(data+4+a,source+4+b,n);
  for(i=0;i<sizeof data;i++)assert(data[i]==(i>=4+a && i<4+a+n?source[i-a+b]:0xa5));
 }
 return 0;
}
