"""List resource names of every ERF V2.0 / V3.0 archive under a folder.
usage: python listall.py <root> <out.txt>"""
import struct, sys, os

def names(p):
    with open(p, 'rb') as f:
        d = f.read()
    magic = d[:16].decode('utf-16le', 'replace')
    if magic == 'ERF V2.0':
        n = struct.unpack_from('<I', d, 16)[0]
        off = 32
        for i in range(n):
            yield d[off:off + 64].decode('utf-16le').rstrip('\0')
            off += 72
    elif magic == 'ERF V3.0':
        strsize, count = struct.unpack_from('<2I', d, 16)
        strings = d[0x30:0x30 + strsize]
        off = 0x30 + strsize
        for i in range(count):
            noff = struct.unpack_from('<I', d, off)[0]
            off += 28
            if noff != 0xFFFFFFFF:
                yield strings[noff:strings.index(b'\0', noff)].decode('ascii', 'replace')

root, out = sys.argv[1], sys.argv[2]
with open(out, 'w', encoding='utf-8') as o:
    for dp, dn, fn in os.walk(root):
        for f in fn:
            if f.lower().endswith(('.erf', '.rim')):
                p = os.path.join(dp, f)
                try:
                    for n in names(p):
                        o.write('%s\t%s\n' % (os.path.relpath(p, root), n))
                except Exception as e:
                    o.write('%s\tERROR %s\n' % (os.path.relpath(p, root), e))
