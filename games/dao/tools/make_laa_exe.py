"""Makes a 4 GB (Large Address Aware) DAOrigins.exe for the Steam copy of Origins.

The Steam exe is wrapped by SteamStub and must not be changed (it stops starting).
Steamless 3.1.0.5 removes the wrapper (DAOrigins.exe.unpacked.exe); this script then
  - clears the security (signature) directory when it points past the end of the
    file (Steamless keeps the Steam file's entry, which no longer exists),
  - sets the Large Address Aware flag (0x20), as games\\da2\\tools\\set_laa.py does,
  - writes the PE checksum again.
Usage: make_laa_exe.py <unpacked exe> <output exe>
"""
import os
import sys

import pefile

src, dst = sys.argv[1], sys.argv[2]
pe = pefile.PE(src)
size = os.path.getsize(src)
sec = pe.OPTIONAL_HEADER.DATA_DIRECTORY[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_SECURITY']]
if sec.VirtualAddress and sec.VirtualAddress + sec.Size > size:
    print('security directory %#x+%#x past the end (%#x): cleared' % (sec.VirtualAddress, sec.Size, size))
    sec.VirtualAddress = 0
    sec.Size = 0
before = pe.FILE_HEADER.Characteristics
pe.FILE_HEADER.Characteristics |= 0x20
pe.OPTIONAL_HEADER.CheckSum = 0
data = bytearray(pe.write())
pe.close()
pe2 = pefile.PE(data=bytes(data))
pe2.OPTIONAL_HEADER.CheckSum = pe2.generate_checksum()
out = pe2.write()
open(dst, 'wb').write(out)
print('Characteristics %#x -> %#x, checksum %#x, %d bytes' %
      (before, before | 0x20, pe2.OPTIONAL_HEADER.CheckSum, len(out)))
