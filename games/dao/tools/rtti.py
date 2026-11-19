import pefile, struct, sys, re
from capstone import *
EXE="F:/games/Dragon Age - Origins/bin_ship/DAOrigins.exe"
pe=pefile.PE(EXE,fast_load=True); IB=pe.OPTIONAL_HEADER.ImageBase
d=pe.__data__ if hasattr(pe,'__data__') else open(EXE,'rb').read()
d=bytes(d)
secs=[(s.Name.rstrip(b'\0').decode(),s.VirtualAddress,s.PointerToRawData,s.SizeOfRawData,s.Misc_VirtualSize) for s in pe.sections]
def off2va(o):
    for n,va,raw,rs,vs in secs:
        if raw<=o<raw+rs: return IB+va+o-raw
def va2off(v):
    r=v-IB
    for n,va,raw,rs,vs in secs:
        if va<=r<va+rs: return raw+r-va
text=[s for s in secs if s[0]=='.text'][0]
def is_code(v): r=v-IB; return text[1]<=r<text[1]+text[4]
def dw(o): return struct.unpack_from('<I',d,o)[0]
def find_dword(val, start=0):
    b=struct.pack('<I',val); res=[]; i=d.find(b,start)
    while i>=0: res.append(i); i=d.find(b,i+1)
    return res
def vtables(clsname):
    nm=('.?AV'+clsname+'@@').encode()+b'\0'
    o=d.find(nm); td=off2va(o-8); out=[]
    for c in find_dword(td):
        if c>=12 and dw(c-12)==0:  # COL sig
            col=off2va(c-12); offs=dw(c-8)
            for r in find_dword(col):
                vt=off2va(r+4); out.append((offs,vt))
    return out
def vtentries(vt):
    o=va2off(vt); e=[]
    while True:
        v=dw(o)
        if not is_code(v): break
        e.append(v); o+=4
        if len(e)>400: break
    return e
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=False
def disfunc(va,maxb=4000):
    o=va2off(va); code=d[o:o+maxb]; out=[]
    for ins in md.disasm(code,va):
        out.append(ins)
        if ins.mnemonic=='ret' or ins.mnemonic=='int3': break
    return out
