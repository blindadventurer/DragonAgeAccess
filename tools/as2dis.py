# Minimal SWF + ActionScript 2 disassembler (for reading the game's .gfx menus).
import struct, sys

OPS = {0x04:'NextFrame',0x05:'PrevFrame',0x06:'Play',0x07:'Stop',0x08:'ToggleQuality',0x09:'StopSounds',
0x0A:'Add',0x0B:'Subtract',0x0C:'Multiply',0x0D:'Divide',0x0E:'Equals',0x0F:'Less',0x10:'And',0x11:'Or',0x12:'Not',
0x13:'StringEquals',0x14:'StringLength',0x15:'StringExtract',0x17:'Pop',0x18:'ToInteger',0x1C:'GetVariable',
0x1D:'SetVariable',0x20:'SetTarget2',0x21:'StringAdd',0x22:'GetProperty',0x23:'SetProperty',0x24:'CloneSprite',
0x25:'RemoveSprite',0x26:'Trace',0x27:'StartDrag',0x28:'EndDrag',0x29:'StringLess',0x2A:'Throw',0x2B:'CastOp',
0x2C:'ImplementsOp',0x30:'RandomNumber',0x31:'MBStringLength',0x32:'CharToAscii',0x33:'AsciiToChar',0x34:'GetTime',
0x35:'MBStringExtract',0x36:'MBCharToAscii',0x37:'MBAsciiToChar',0x3A:'Delete',0x3B:'Delete2',0x3C:'DefineLocal',
0x3D:'CallFunction',0x3E:'Return',0x3F:'Modulo',0x40:'NewObject',0x41:'DefineLocal2',0x42:'InitArray',0x43:'InitObject',
0x44:'TypeOf',0x45:'TargetPath',0x46:'Enumerate',0x47:'Add2',0x48:'Less2',0x49:'Equals2',0x4A:'ToNumber',0x4B:'ToString',
0x4C:'PushDuplicate',0x4D:'StackSwap',0x4E:'GetMember',0x4F:'SetMember',0x50:'Increment',0x51:'Decrement',
0x52:'CallMethod',0x53:'NewMethod',0x54:'InstanceOf',0x55:'Enumerate2',0x60:'BitAnd',0x61:'BitOr',0x62:'BitXor',
0x63:'BitLShift',0x64:'BitRShift',0x65:'BitURShift',0x66:'StrictEquals',0x67:'Greater',0x68:'StringGreater',0x69:'Extends',
0x81:'GotoFrame',0x83:'GetURL',0x87:'StoreRegister',0x88:'ConstantPool',0x8A:'WaitForFrame',0x8B:'SetTarget',0x8C:'GoToLabel',
0x8D:'WaitForFrame2',0x8E:'DefineFunction2',0x8F:'Try',0x94:'With',0x96:'Push',0x99:'Jump',0x9A:'GetURL2',0x9B:'DefineFunction',
0x9D:'If',0x9E:'Call',0x9F:'GotoFrame2'}

def cstr(b, i):
    j = b.find(0, i)
    if j < 0: j = len(b)
    return b[i:j].decode('utf-8', 'replace'), j + 1

def dis(b, out, indent='  ', pool=None):
    i = 0; pool = pool if pool is not None else []
    while i < len(b):
        op = b[i]; at = i; i += 1
        if op == 0: out.append(f'{indent}{at:5d} End'); continue
        ln = 0
        if op >= 0x80:
            ln = struct.unpack_from('<H', b, i)[0]; i += 2
        data = b[i:i+ln]; i += ln
        name = OPS.get(op, f'op{op:02x}')
        arg = ''
        if op == 0x88:
            n = struct.unpack_from('<H', data, 0)[0]; k = 2; pool.clear()
            for _ in range(n):
                s, k = cstr(data, k); pool.append(s)
            arg = f'{n} entries'
        elif op == 0x96:
            k = 0; items = []
            while k < len(data):
                t = data[k]; k += 1
                if t == 0: s, k = cstr(data, k); items.append(repr(s))
                elif t == 1: items.append(str(struct.unpack_from('<f', data, k)[0])); k += 4
                elif t == 2: items.append('null')
                elif t == 3: items.append('undefined')
                elif t == 4: items.append(f'r{data[k]}'); k += 1
                elif t == 5: items.append('true' if data[k] else 'false'); k += 1
                elif t == 6: items.append(str(struct.unpack_from('<d', data[k+4:k+8]+data[k:k+4])[0])); k += 8
                elif t == 7: items.append(str(struct.unpack_from('<i', data, k)[0])); k += 4
                elif t == 8:
                    c = data[k]; k += 1; items.append(repr(pool[c]) if c < len(pool) else f'pool{c}')
                elif t == 9:
                    c = struct.unpack_from('<H', data, k)[0]; k += 2; items.append(repr(pool[c]) if c < len(pool) else f'pool{c}')
                else: items.append(f'?t{t}'); break
            arg = ', '.join(items)
        elif op in (0x9B, 0x8E):
            fn, k = cstr(data, 0)
            n = struct.unpack_from('<H', data, k)[0]; k += 2
            params = []
            if op == 0x8E:
                k += 1; k += 2  # regcount, flags
                for _ in range(n):
                    r = data[k]; k += 1; p, k = cstr(data, k); params.append(f'{p}' + (f'@r{r}' if r else ''))
            else:
                for _ in range(n):
                    p, k = cstr(data, k); params.append(p)
            size = struct.unpack_from('<H', data, k)[0]
            out.append(f'{indent}{at:5d} function {fn}({", ".join(params)}) {{')
            body = b[i:i+size]; i += size
            dis(body, out, indent + '    ', pool)
            out.append(f'{indent}      }}'); continue
        elif op in (0x99, 0x9D):
            arg = f'-> {at + 5 + struct.unpack_from("<h", data, 0)[0]}'
        elif op == 0x83:
            u, k = cstr(data, 0); t, k = cstr(data, k); arg = f'{u!r} {t!r}'
        elif op in (0x8B, 0x8C):
            arg = repr(cstr(data, 0)[0])
        elif op == 0x87: arg = f'r{data[0]}'
        elif op == 0x94: arg = f'size {struct.unpack_from("<H", data, 0)[0]}'
        elif op == 0x8F: arg = data.hex()
        elif ln: arg = data.hex()
        out.append(f'{indent}{at:5d} {name} {arg}')

TAGS = {0:'End',1:'ShowFrame',2:'DefineShape',4:'PlaceObject',5:'RemoveObject',9:'SetBackgroundColor',10:'DefineFont',
11:'DefineText',12:'DoAction',13:'DefineFontInfo',14:'DefineSound',22:'DefineShape2',26:'PlaceObject2',28:'RemoveObject2',
32:'DefineShape3',33:'DefineText2',34:'DefineButton2',36:'DefineBitsLossless2',37:'DefineEditText',39:'DefineSprite',
43:'FrameLabel',48:'DefineFont2',56:'ExportAssets',57:'ImportAssets',59:'DoInitAction',62:'DefineFontInfo2',
70:'PlaceObject3',73:'DefineFontAlignZones',74:'CSMTextSettings',75:'DefineFont3',77:'Metadata',83:'DefineShape4',
86:'DefineSceneAndFrameLabelData',88:'DefineFontName',1000:'GFx_ExporterInfo',1001:'GFx_DefineExternalImage',
1002:'GFx_FontTextureInfo',1003:'GFx_DefineExternalGradient',1004:'GFx_DefineGradientMap',1005:'GFx_DefineCompactedFont',
1006:'GFx_DefineExternalSound',1007:'GFx_DefineExternalStreamSound',1008:'GFx_DefineSubImage',1009:'GFx_DefineExternalImage2'}

def rect_len(b, i):
    nb = b[i] >> 3; return (5 + 4 * nb + 7) // 8

def tags(b, i, end, out, depth, exports):
    pad = '  ' * depth
    while i + 2 <= end:
        hdr = struct.unpack_from('<H', b, i)[0]; i += 2
        code, ln = hdr >> 6, hdr & 0x3F
        if ln == 0x3F: ln = struct.unpack_from('<I', b, i)[0]; i += 4
        data = b[i:i+ln]; i += ln
        tn = TAGS.get(code, f'tag{code}')
        try:
            one(code, tn, data, out, pad, depth, exports)
        except Exception as e:
            out.append(f'{pad}!! {tn} parse error {e}')

def one(code, tn, data, out, pad, depth, exports):
        if code == 39:
            sid = struct.unpack_from('<H', data, 0)[0]
            out.append(f'{pad}DefineSprite id={sid} {exports.get(sid, "")}')
            tags(data, 4, len(data), out, depth + 1, exports)
        elif code == 12:
            out.append(f'{pad}DoAction'); dis(data, out, pad + '  ')
        elif code == 59:
            sid = struct.unpack_from('<H', data, 0)[0]
            out.append(f'{pad}DoInitAction sprite={sid} {exports.get(sid, "")}'); dis(data[2:], out, pad + '  ')
        elif code == 56:
            n = struct.unpack_from('<H', data, 0)[0]; k = 2
            for _ in range(n):
                cid = struct.unpack_from('<H', data, k)[0]; s, k = cstr(data, k + 2); exports[cid] = s
                out.append(f'{pad}Export {cid} = {s}')
        elif code == 43:
            out.append(f'{pad}FrameLabel {cstr(data, 0)[0]!r}')
        elif code in (26, 70):
            fl = data[0]; k = 1
            fl2 = 0
            if code == 70: fl2 = data[1]; k = 2
            depth_ = struct.unpack_from('<H', data, k)[0]; k += 2
            s = f'{pad}{tn} depth={depth_}'
            if code == 70 and (fl2 & 8 or (fl2 & 16 and fl & 2)):
                c, k = cstr(data, k); s += f' class={c!r}'
            if fl & 2: s += f' char={struct.unpack_from("<H", data, k)[0]}'; k += 2
            # skip matrix/cxform/ratio, then name: parse roughly
            if fl & 4:
                bits = ''.join(f'{x:08b}' for x in data[k:k+30]); p = 0
                if bits[p] == '1': p += 1; nb = int(bits[p:p+5], 2); p += 5 + 2 * nb
                else: p += 1
                if bits[p] == '1': p += 1; nb = int(bits[p:p+5], 2); p += 5 + 2 * nb
                else: p += 1
                nb = int(bits[p:p+5], 2); p += 5 + 2 * nb
                k += (p + 7) // 8
            if fl & 8:
                bits = ''.join(f'{x:08b}' for x in data[k:k+30]); p = 0
                ha, hm = bits[0] == '1', bits[1] == '1'; nb = int(bits[2:6], 2); p = 6 + nb * ((4 if ha else 0) + (4 if hm else 0))
                k += (p + 7) // 8
            if fl & 16: k += 2
            if fl & 32: s += f' name={cstr(data, k)[0]!r}'
            out.append(s)
        elif code == 37:
            cid = struct.unpack_from('<H', data, 0)[0]; k = 2 + rect_len(data, 2)
            f1, f2 = data[k], data[k+1]; k += 2
            if f1 & 1: k += 4
            if f2 & 0x80: k += 2  # HasFontClass? (bit 7 of flags2 = HasFontClass in later SWF)
            if f1 & 4: k += 4
            if f1 & 2: k += 2
            if f2 & 0x20: k += 9
            var, k = cstr(data, k)
            txt = cstr(data, k)[0] if f1 & 0x80 else ''
            out.append(f'{pad}DefineEditText id={cid} var={var!r} text={txt[:80]!r}')
        elif code == 34:
            cid = struct.unpack_from('<H', data, 0)[0]
            out.append(f'{pad}DefineButton2 id={cid}')
        elif code not in (1,):
            if depth == 0 and code in (0,): pass
        if code == 1: out.append(f'{pad}ShowFrame')
        if code == 1000 or code >= 1001: out.append(f'{pad}{tn} len={len(data)}')

def main(path):
    b = open(path, 'rb').read()
    i = 8 + rect_len(b, 8) + 4
    out = []; exports = {}
    tags(b, i, len(b), out, 0, exports)
    print('\n'.join(out))

main(sys.argv[1])
