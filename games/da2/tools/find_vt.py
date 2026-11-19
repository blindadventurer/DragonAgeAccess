import sys, struct, collections, pickle
sys.path.insert(0, '.')
from exe import *
strs={'getvar':b'Error: GetVariable failed','setvar':b'Error: SetVariable failed','invparsed':b"Error: InvokeParsed('%s','%s') - invalid char",'getraw':b'GetVariableRaw("%s") failed','nullpath':b'Error: NULL pathToVar'}
funcs={}
for k,s in strs.items():
  va=string_va(s); xs=xrefs_imm(va)
  fs=sorted(set(func_start(x) for x in xs)); funcs[k]=fs
  print(k,hex(va),[hex(x) for x in xs],[hex(f) for f in fs])
t=d[T[2]:T[2]+T[3]]
callsites=collections.defaultdict(list)
for i in range(len(t)-5):
  if t[i] in (0xE8,0xE9):
    tgt=(TEXT_VA+i+5+struct.unpack_from('<i',t,i+1)[0])&0xFFFFFFFF
    callsites[tgt].append(TEXT_VA+i)
pickle.dump(dict(callsites),open('../work/callsites.pkl','wb'))
for k,fs in funcs.items():
 for f in fs:
  lvl={f}; allf={f:0}
  for depth in (1,2,3):
    nxt=set()
    for g in lvl:
      for cs in callsites.get(g,[]):
        s=func_start(cs)
        if s and s not in allf: allf[s]=depth; nxt.add(s)
    lvl=nxt
  for g,dep in allf.items():
    for r in code_ptr_refs(g):
      vs=vtable_start(r)
      print(k,'func',hex(g),'depth',dep,'in vtable',hex(vs),'slot',(r-vs)//4)
