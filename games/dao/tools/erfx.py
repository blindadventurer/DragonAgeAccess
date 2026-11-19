import struct,sys,zlib,os
p,out=sys.argv[1],sys.argv[2]
d=open(p,'rb').read()
n=struct.unpack_from('<I',d,16)[0]; off=32
for i in range(n):
    name=d[off:off+64].decode('utf-16le').rstrip('\0'); o,s=struct.unpack_from('<2I',d,off+64); off+=72
    if not name.endswith('.gfx'): continue
    b=d[o:o+s]
    if b[:3]==b'CFX': b=b'FWS'+b[3:8]+zlib.decompress(b[8:])
    open(os.path.join(out,name),'wb').write(b)
