import re,sys
data=open(sys.argv[1],'rb').read()
pat=re.compile(sys.argv[2].encode())
for m in re.finditer(rb'[\x20-\x7e]{5,}',data):
    s=m.group()
    if pat.search(s): print(hex(m.start()), s.decode())
