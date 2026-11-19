"""Finds GameMode message handlers: MemberFunctionMessageHandler<Owner, Msg> vtable
and the owner's HandleMessage(const Msg&) it calls.
usage: python handlers.py Owner Msg [Msg...]"""
import sys, struct
sys.path.insert(0, '.')
from exe import *
def find_dw_all(val):
    return find_all(struct.pack('<I', val))
def vtables_for(prefix):
    out = []
    o = d.find(prefix)
    while o >= 0:
        td = off2va(o - 8)
        for c in find_dw_all(td):
            if c >= 12 and dw(c - 12) == 0:
                col = off2va(c - 12)
                for r in find_dw_all(col):
                    out.append(off2va(r + 4))
        o = d.find(prefix, o + 1)
    return out
if __name__ == '__main__':
    owner = sys.argv[1]
    for m in sys.argv[2:]:
        pre = ('.?AV?$MemberFunctionMessageHandler@V%s@@V%s@@' % (owner, m)).encode()
        for vt in vtables_for(pre):
            ents = vtable_at(vt, 8)
            print(m, 'vtable', hex(vt), [hex(x) for x in ents])
            for f in ents:
                for x in func_insns(f, 200)[:12]:
                    if x.mnemonic in ('call', 'jmp'): print('   ', hex(f), x.mnemonic, x.op_str)
