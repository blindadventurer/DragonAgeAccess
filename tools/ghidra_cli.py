# Headless Ghidra (no MCP server needed) on this project's own Ghidra project
# C:\tools\ghidra-projects\DragonAgeAccess.gpr, program DAOrigins.unpacked.exe-531cd1.
# Run with the pyghidra venv of the MCP server:
#   C:\tools\mcp\ghidra\Scripts\python.exe tools\ghidra_cli.py <cmd> <args...>
#   str <substring>...        strings containing these, with the functions using them
#   dec <hexaddr>...          decompile the functions at these addresses
#   callers <hexaddr>         who calls this address
#   rename <hexaddr> <name>.. give functions readable names (saved in the project)
# Fails while another process (a session's Ghidra MCP server) holds the project.
import sys, os
os.environ.setdefault('GHIDRA_INSTALL_DIR', r'C:\tools\ghidra\ghidra_12.2_PUBLIC')
import pyghidra
pyghidra.start()
from ghidra.base.project import GhidraProject
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

proj = GhidraProject.openProject(r'C:\tools\ghidra-projects', 'DragonAgeAccess', True)
try:
    prog = proj.openProgram('/', 'DAOrigins.unpacked.exe-531cd1', sys.argv[1] != 'rename')
    fm = prog.getFunctionManager()
    listing = prog.getListing()
    refm = prog.getReferenceManager()
    af = prog.getAddressFactory()
    out = []
    cmd = sys.argv[1]
    if cmd == 'str':
        subs = sys.argv[2:]
        for d in listing.getDefinedData(True):
            v = d.getValue()
            if v is None: continue
            s = str(v)
            if not any(x in s for x in subs): continue
            users = []
            for r in refm.getReferencesTo(d.getAddress()):
                f = fm.getFunctionContaining(r.getFromAddress())
                users.append('%s@%s' % (f.getName() if f else '?', r.getFromAddress()))
            out.append('%s %r -> %s' % (d.getAddress(), s[:80], ', '.join(users[:8])))
    elif cmd == 'dec':
        di = DecompInterface(); di.openProgram(prog)
        for a in sys.argv[2:]:
            addr = af.getAddress(a)
            f = fm.getFunctionContaining(addr)
            if f is None:
                out.append('no function at ' + a); continue
            r = di.decompileFunction(f, 120, ConsoleTaskMonitor())
            out.append('// %s @ %s\n%s' % (f.getName(), f.getEntryPoint(), r.getDecompiledFunction().getC() if r.decompileCompleted() else 'FAILED'))
    elif cmd == 'callers':
        addr = af.getAddress(sys.argv[2])
        for r in refm.getReferencesTo(addr):
            f = fm.getFunctionContaining(r.getFromAddress())
            out.append('%s %s %s' % (r.getFromAddress(), r.getReferenceType(), f.getName() + '@' + str(f.getEntryPoint()) if f else '?'))
    elif cmd == 'rename':
        tx = prog.startTransaction('rename')
        from ghidra.program.model.symbol import SourceType
        a = sys.argv[2:]
        for i in range(0, len(a), 2):
            f = fm.getFunctionAt(af.getAddress(a[i]))
            if f: f.setName(a[i + 1], SourceType.USER_DEFINED)
            else: out.append('no function at ' + a[i])
        prog.endTransaction(tx, True)
        proj.save(prog)
        out.append('renamed')
    sys.stdout.reconfigure(encoding='utf-8')
    print('\n'.join(out))
finally:
    proj.close()
