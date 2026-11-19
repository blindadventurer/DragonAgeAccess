# Minimal reader for BioWare GFF V3.2 (Dragon Age: Origins .are files, header "ARE V3.2").
import struct, sys
class GFF:
    def __init__(self, b):
        self.b = b
        h = struct.unpack_from('<12I', b, 8)
        (self.so, self.sc, self.fo, self.fc, self.lo, self.lc, self.fdo, self.fdc, self.fio, self.fic, self.lio, self.lic) = h
        self.labels = [b[self.lo + 16*i:self.lo + 16*i + 16].split(b'\0')[0].decode() for i in range(self.lc)]
    def struct(self, i):
        t, dofs, cnt = struct.unpack_from('<3I', self.b, self.so + 12*i)
        if cnt == 1: idx = [dofs]
        else: idx = list(struct.unpack_from('<%dI' % cnt, self.b, self.fio + dofs))
        return {self.labels[l]: v for l, v in (self.field(f) for f in idx)}
    def field(self, f):
        t, l, v = struct.unpack_from('<3I', self.b, self.fo + 12*f)
        b = self.b
        if t in (0,): val = v & 0xFF
        elif t == 1: val = struct.unpack('<b', struct.pack('<I', v)[:1])[0]
        elif t == 2: val = v & 0xFFFF
        elif t == 3: val = struct.unpack('<h', struct.pack('<I', v)[:2])[0]
        elif t == 4: val = v
        elif t == 5: val = struct.unpack('<i', struct.pack('<I', v))[0]
        elif t == 8: val = struct.unpack('<f', struct.pack('<I', v))[0]
        elif t == 10:
            n = struct.unpack_from('<I', b, self.fdo + v)[0]; val = b[self.fdo+v+4:self.fdo+v+4+n].decode('latin-1')
        elif t == 11:
            n = b[self.fdo + v]; val = b[self.fdo+v+1:self.fdo+v+1+n].decode('latin-1')
        elif t == 14: val = self.struct(v)
        elif t == 15:
            n = struct.unpack_from('<I', b, self.lio + v)[0]
            val = [self.struct(x) for x in struct.unpack_from('<%dI' % n, b, self.lio + v + 4)]
        else: val = ('type%d' % t, v)
        return l, val
if __name__ == '__main__':
    g = GFF(open(sys.argv[1], 'rb').read())
    top = g.struct(0)
    print({k: (v if not isinstance(v, list) else len(v)) for k, v in top.items() if not isinstance(v, dict)})
    for c in top.get('CreatureList', []):
        print(c.get('Active'), c.get('Tag'), c.get('TemplateResRef'), round(c.get('XPosition', 0), 1), round(c.get('YPosition', 0), 1))
