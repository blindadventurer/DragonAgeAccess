"""List or extract files from a Dragon Age 2 ERF V3.0 archive.
usage: python erf3.py <archive> [outdir] [suffix-filter]"""
import struct, sys, zlib, os

def entries(d):
    magic = d[:16].decode('utf-16le')
    assert magic == 'ERF V3.0', magic
    strsize, count, flags, module = struct.unpack_from('<4I', d, 16)
    strings = d[0x30:0x30 + strsize]
    comp = flags >> 29
    enc = (flags >> 4) & 0xF
    off = 0x30 + strsize
    for i in range(count):
        noff, nhash, thash, o, packed, unpacked = struct.unpack_from('<IQIIII', d, off)
        off += 28
        name = None
        if noff != 0xFFFFFFFF:
            end = strings.index(b'\0', noff)
            name = strings[noff:end].decode('ascii')
        yield name or '%016x' % nhash, o, packed, unpacked, comp, enc

def unpack(b, unpacked, comp):
    if comp == 0 or len(b) == unpacked:
        return b
    for skip in (0, 1):
        for wbits in (15, -15):
            try:
                r = zlib.decompress(b[skip:], wbits)
                if len(r) == unpacked:
                    return r
            except zlib.error:
                pass
    raise ValueError('cannot unpack (comp %d)' % comp)

def main():
    p = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else None
    flt = sys.argv[3] if len(sys.argv) > 3 else ''
    d = open(p, 'rb').read()
    for name, o, packed, unpacked, comp, enc in entries(d):
        if flt and not name.endswith(flt):
            continue
        if not out:
            print(name, packed, unpacked, 'comp', comp, 'enc', enc)
            continue
        if enc:
            print('encrypted, skipped', name); continue
        b = unpack(d[o:o + packed], unpacked, comp)
        if b[:3] == b'CFX':  # Scaleform compressed movie -> plain SWF
            b = b'FWS' + b[3:8] + zlib.decompress(b[8:])
        open(os.path.join(out, name), 'wb').write(b)

main()
