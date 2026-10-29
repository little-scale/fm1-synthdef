"""Declared distribution/roughness checks for deliberately float chaotic paths."""
import math
TYPES=('Crackle','Logistic','Henon','Cusp','Latoocarfian','Lorenz')
def comparison(name,a,b):
 if not name.startswith('m14_') or not any(name.startswith('m14_'+t) for t in TYPES) or name.endswith('_stable'):return {}
 # Remove stereo padding and constructor transient; kr outputs are held for 64 frames.
 stride=128 if '_kr_' in name else 2
 x=list(a)[::stride];y=list(b)[::stride];skip=max(4,len(x)//10);x=x[skip:];y=y[skip:]
 def stats(z):
  m=sum(z)/len(z);sd=math.sqrt(sum((v-m)**2 for v in z)/len(z));r=math.sqrt(sum(v*v for v in z)/len(z));d=math.sqrt(sum((v-w)**2 for v,w in zip(z[1:],z))/max(1,len(z)-1));return m,sd,r,d
 ma,sa,ra,da=stats(x);mb,sb,rb,db=stats(y)
 ratio=lambda x,y:x/y if y else (1 if not x else float('inf'))
 rms_ratio=ratio(ra,rb);sd_ratio=ratio(sa,sb);roughness_ratio=ratio(da,db)
 mean_error=abs(ma-mb)/max(ra,rb,1e-12)
 # These envelopes intentionally check acoustic character, not sample identity.
 passed=all(math.isfinite(v) for v in x+y) and .5<rms_ratio<2 and .5<sd_ratio<2 and .4<roughness_ratio<2.5 and mean_error<.4
 return dict(comparison='single-precision chaotic statistics; independent transition oracle required',rms_ratio=rms_ratio,sd_ratio=sd_ratio,roughness_ratio=roughness_ratio,normalized_mean_error=mean_error,stochastic_passed=passed)
