import sys; sys.path.insert(0,'.')
import exe
if len(sys.argv)>3:
    import importlib, types
    exe.EXE=sys.argv[3]
from exe import *
va=int(sys.argv[1],16); n=int(sys.argv[2])
for i,x in enumerate(func_insns(va)):
  if i>=n: break
  print(hex(x.address),x.mnemonic,x.op_str)
