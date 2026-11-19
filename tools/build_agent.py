# Builds a game's Flash agent: a one-frame SWF 8 whose frame 1 DoAction is compiled
# by FFDec from core/agent/daoaccess.as (the same agent for both games).
# Usage: python tools/build_agent.py <output .gfx>   (build.ps1 passes
#        build/<game>/daoaccess.gfx or build/<game>/da2access.gfx)
import os, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FFDEC = os.path.join(ROOT, 'vendor', 'ffdec-26.5.0', 'ffdec-cli.jar')
SRC = os.path.join(ROOT, 'core', 'agent', 'daoaccess.as')
if len(sys.argv) != 2:
    sys.exit('usage: build_agent.py <output .gfx>')
OUT = os.path.abspath(sys.argv[1])
BUILD = os.path.dirname(OUT)
BASE = os.path.join(BUILD, 'agent_base.swf')


def tag(code, data):
    if len(data) < 0x3F:
        return struct.pack('<H', (code << 6) | len(data)) + data
    return struct.pack('<HI', (code << 6) | 0x3F, len(data)) + data


def base_swf():
    # RECT 0..1x1 px in twips, nbits = 6: 5 + 4*6 = 29 bits -> 4 bytes
    nb = 6
    bits = format(nb, '05b') + ''.join(format(v, f'0{nb}b') for v in (0, 20, 0, 20))
    bits += '0' * (-len(bits) % 8)
    rect = int(bits, 2).to_bytes(len(bits) // 8, 'big')
    body = rect + struct.pack('<HH', 24 << 8, 1)        # 24 fps, 1 frame
    body += tag(69, struct.pack('<I', 0))               # FileAttributes
    body += tag(9, b'\x00\x00\x00')                     # SetBackgroundColor
    body += tag(12, b'\x96\x02\x00\x02\x00\x17\x00')    # DoAction: push null, pop, end
    body += tag(1, b'')                                 # ShowFrame
    body += tag(0, b'')                                 # End
    return b'FWS' + bytes([8]) + struct.pack('<I', 8 + len(body)) + body


def main():
    os.makedirs(BUILD, exist_ok=True)
    with open(BASE, 'wb') as f:
        f.write(base_swf())
    cmd = ['java', '-jar', FFDEC, '-replace', BASE, OUT, '\\frame_1\\DoAction', SRC]
    r = subprocess.run(cmd, capture_output=True, text=True)
    sys.stdout.write(r.stdout)
    sys.stderr.write(r.stderr)
    if r.returncode != 0 or not os.path.exists(OUT):
        sys.exit('agent build failed')
    print('Built', OUT, os.path.getsize(OUT), 'bytes')


main()
