"""Helpers over DragonAge2.exe: address conversion, xrefs, call graph, vtables."""
import pefile, struct, re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
# The Steam exe (EA wrapper) has its .text encrypted on disk: only .rdata / .data
# (vtables, strings) read correctly from it; code needs a decrypted copy (none yet).
EXE = "F:/SteamLibrary/steamapps/common/Dragon Age II/bin_ship/DragonAge2.exe"
pe = pefile.PE(EXE, fast_load=True); IB = pe.OPTIONAL_HEADER.ImageBase
d = open(EXE, 'rb').read()
secs = [(s.Name.rstrip(b'\0').decode(), s.VirtualAddress, s.PointerToRawData, s.SizeOfRawData, s.Misc_VirtualSize) for s in pe.sections]
def off2va(o):
    for n, va, raw, rs, vs in secs:
        if raw <= o < raw + rs: return IB + va + o - raw
def va2off(v):
    r = v - IB
    for n, va, raw, rs, vs in secs:
        if va <= r < va + rs: return raw + r - va
T = [s for s in secs if s[0] == '.text'][0]
RD = [s for s in secs if s[0] == '.rdata'][0]
def is_code(v): r = v - IB; return T[1] <= r < T[1] + T[4]
def dw(o): return struct.unpack_from('<I', d, o)[0]
def find_all(b, lo=0, hi=None):
    res = []; i = d.find(b, lo)
    while i >= 0 and (hi is None or i < hi): res.append(i); i = d.find(b, i + 1)
    return res
md = Cs(CS_ARCH_X86, CS_MODE_32)
_text = d[T[2]:T[2] + T[3]]
TEXT_VA = IB + T[1]
def xrefs_imm(va):
    """offsets in .text where va appears as a 32-bit immediate"""
    b = struct.pack('<I', va)
    out = []; i = _text.find(b)
    while i >= 0: out.append(TEXT_VA + i); i = _text.find(b, i + 1)
    return out
def func_start(va):
    """walk back to the byte after int3/nop padding (MSVC aligns functions to 16)"""
    o = va2off(va)
    while o > T[2]:
        if d[o - 1] == 0xCC and (off2va(o) & 0xF) == 0: return off2va(o)
        if d[o-1] == 0xC3 and (off2va(o) & 0xF) == 0 and d[o] in (0x55,0x83,0x56,0x53,0x8B,0x6A,0x51,0x81,0x57,0x64,0xA1,0x33,0xB8,0xD9): return off2va(o)
        o -= 1
def dis(va, n=4000):
    o = va2off(va); return list(md.disasm(d[o:o + n], va))
def func_insns(va, maxlen=20000):
    out = []
    for ins in md.disasm(d[va2off(va):va2off(va) + maxlen], va):
        out.append(ins)
        if ins.mnemonic == 'int3': break
    return out
def calls(va):
    r = []
    for ins in func_insns(va):
        if ins.mnemonic in ('call', 'jmp') and ins.op_str.startswith('0x'):
            r.append(int(ins.op_str, 16))
    return r
def code_ptr_refs(va):
    """offsets in .rdata holding the dword va"""
    return [off2va(o) for o in find_all(struct.pack('<I', va), RD[2], RD[2] + RD[3])]
def vtable_at(vt, maxn=400):
    o = va2off(vt); e = []
    while len(e) < maxn:
        v = dw(o)
        if not is_code(v): break
        e.append(v); o += 4
    return e
def vtable_start(va):
    """walk back from an .rdata slot to the first entry of the run of code pointers"""
    o = va2off(va)
    while is_code(dw(o - 4)): o -= 4
    return off2va(o)
def string_va(s):
    return off2va(d.find(s))
