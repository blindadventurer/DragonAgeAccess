"""Read a GFF V4.0 G2DA table (.gda). Usage: g2da.py <erf> <name.gda> [names...]
Column names are stored as CRC32 of the lowercase UTF-16LE name; candidate
names given on the command line (and a built-in list) are matched to them."""
import struct, sys, zlib

COMMON = ["ID", "Label", "Name", "TriggerChance", "Chance", "Repeatable", "Plot", "PlotFlag",
          "Flag", "Area", "AreaTag", "Tag", "Script", "Module", "Waypoint", "MinLevel", "Level",
          "Weight", "Enabled", "Once", "Repeat", "Type", "fDeathblow", "Color", "Area_Tag",
          "PlotName", "PlotGUID", "PlotCondition", "Condition", "Destination", "StartArea",
          "EndArea", "Resref", "AreaList", "Comment", "Description", "Unique", "Max", "Min",
          "Encounter", "EncounterArea", "Plot_Flag", "PlotFlagValue", "Value", "Priority"]

def erf_entry(path, name):
    d = open(path, 'rb').read()
    n = struct.unpack_from('<I', d, 16)[0]; off = 32
    for i in range(n):
        nm = d[off:off+64].decode('utf-16le').rstrip('\0'); o, s = struct.unpack_from('<2I', d, off+64); off += 72
        if nm.lower() == name.lower(): return d[o:o+s]
    raise SystemExit("not found")

def read(d, names=()):
    hashes = {}
    for nm in list(COMMON) + list(names):
        hashes[zlib.crc32(nm.lower().encode('utf-16le')) & 0xffffffff] = nm
    data = struct.unpack_from('<I', d, 0x18)[0]
    nstruct = struct.unpack_from('<I', d, 0x14)[0]
    structs = []
    off = 0x1C
    for i in range(nstruct):
        tag, fc, fo, size = struct.unpack_from('<4sIII', d, off); off += 16
        fields = [struct.unpack_from('<III', d, fo + 12*j) for j in range(fc)]
        structs.append((tag, fields, size))
    top = struct.unpack_from('<II', d, data)
    colo = data + top[0]; ncol = struct.unpack_from('<I', d, colo)[0]
    cols = []
    for i in range(ncol):
        h, t = struct.unpack_from('<IB', d, colo + 4 + 8*i)
        cols.append(hashes.get(h, "0x%08x" % h))
    rowo = data + top[1]; nrow = struct.unpack_from('<I', d, rowo)[0]
    rfields = structs[2][1]; rsize = structs[2][2]
    rows = []
    for r in range(nrow):
        base = rowo + 4 + r*rsize; row = []
        for (lab, typ, fo) in rfields:
            t = typ & 0xffff
            if t == 14:
                so = struct.unpack_from('<I', d, base+fo)[0]
                if so == 0xffffffff: row.append(None)
                else:
                    L = struct.unpack_from('<I', d, data+so)[0]
                    row.append(d[data+so+4:data+so+4+L*2-2].decode('utf-16le'))
            elif t == 8: row.append(round(struct.unpack_from('<f', d, base+fo)[0], 4))
            elif t in (4, 5): row.append(struct.unpack_from('<i', d, base+fo)[0])
            elif t in (0, 1): row.append(d[base+fo])
            else: row.append("t%d" % t)
        rows.append((base, row))
    return cols, rfields, rows

if __name__ == "__main__":
    d = erf_entry(sys.argv[1], sys.argv[2])
    cols, rf, rows = read(d, sys.argv[3:])
    print(" | ".join("%s(t%d)" % (c, f[1] & 0xffff) for c, f in zip(cols, rf)))
    for base, r in rows: print(r)
