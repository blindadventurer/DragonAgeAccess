import sys; sys.path.insert(0,'.')
from exe import *
vt=int(sys.argv[1],16); lo=int(sys.argv[2]); hi=int(sys.argv[3])
e=vtable_at(vt)
print('vtable entries',len(e))
for i in range(lo,min(hi,len(e))):
  f=e[i]; ins=func_insns(f,6000)
  rets=sorted(set(x.op_str or '0' for x in ins if x.mnemonic=='ret'))
  first=' ; '.join(x.mnemonic+' '+x.op_str for x in ins[:6])
  print(i,hex(f),'len',len(ins),'ret',rets,'|',first[:150])
