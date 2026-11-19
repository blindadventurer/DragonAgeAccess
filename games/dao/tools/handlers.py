# Finds the member function registered for MemberFunctionMessageHandler<Owner, Message>.
import sys
exec(open(__file__.replace('handlers.py','rtti.py')).read())
def handler(owner, msg):
    name=('.?AV?$MemberFunctionMessageHandler@V%s@@V%s@@' % (owner,msg)).encode()
    o=d.find(name)
    if o<0:
        name=('.?AV?$MemberFunctionMessageHandler@V%s@@V%s@' % (owner,msg)).encode(); o=d.find(name)
    if o<0: return None
    td=off2va(o-8); res=[]
    for c in find_dword(td):
        if c>=12 and dw(c-12)==0:
            col=off2va(c-12)
            for r in find_dword(col):
                vt=off2va(r+4)
                for ref in [x for x in find_dword(vt) if text[2]<=x<text[2]+text[3]]:
                    va=off2va(ref)
                    # look back for 'mov ecx, imm32' (B9 xx xx xx xx) within 16 bytes
                    for back in range(4,24):
                        if d[ref-back]==0xB9:
                            res.append(dw(ref-back+1)); break
    return res
if __name__=='__main__':
    for m in sys.argv[2:]:
        print(m, [hex(x) for x in (handler(sys.argv[1], m) or [])])
