"""Sets the Large Address Aware flag (0x20) on a 32-bit exe and updates its checksum."""
import pefile, sys
p = sys.argv[1]
pe = pefile.PE(p)
before = pe.FILE_HEADER.Characteristics
pe.FILE_HEADER.Characteristics |= 0x20
pe.OPTIONAL_HEADER.CheckSum = pe.generate_checksum()
data = pe.write()
pe.close()
open(p, 'wb').write(data)
print('Characteristics %#x -> %#x' % (before, before | 0x20))
