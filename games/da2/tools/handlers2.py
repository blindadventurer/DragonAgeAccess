"""Owner's HandleMessage(Msg) target: handler vtable -> ctor -> registration site -> mov ecx, fn.
usage: python handlers2.py Owner Msg [Msg...]"""
import sys, pickle
sys.path.insert(0, '.')
from exe import *
from handlers import vtables_for
cs = pickle.load(open('../work/callsites.pkl', 'rb'))
def targets(owner, msg):
    pre = ('.?AV?$MemberFunctionMessageHandler@V%s@@V%s@@' % (owner, msg)).encode()
    res = []
    for vt in vtables_for(pre):
        for x in xrefs_imm(vt):
            ctor = func_start(x)
            for c in cs.get(ctor, []):
                for i in reversed(dis(c - 0x20, 0x20)):
                    if i.address >= c: continue
                    if i.mnemonic == 'mov' and i.op_str.startswith('ecx, 0x'):
                        res.append((hex(int(i.op_str[5:], 16)), 'reg@' + hex(c))); break
    return res
for m in sys.argv[2:]:
    print(m, targets(sys.argv[1], m))
