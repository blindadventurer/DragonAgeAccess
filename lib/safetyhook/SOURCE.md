# SafetyHook (third-party, unmodified)

- What: C++ hooking library for x86 and x64 (inline hooks, mid-function hooks with
  register access, vmt hooks), with the Zydis disassembler bundled.
- From: https://github.com/cursey/safetyhook/releases/tag/v0.7.0
  asset safetyhook-amalgamated-zydis.zip, sha256
  505d4c07ec1c5b94a17f3906ca86afbe1264e738d8becaa244866694c6200c2c
- Downloaded 2026-11-22. Files are copied from the zip as they are.
- Licenses: SafetyHook BSL-1.0 (LICENSE_safetyhook.txt), Zydis MIT
  (LICENSE_zydis.txt). Both allow shipping it inside the mod; keep the
  license files in the release.
- Needs C++23 (std::expected): build with cl /std:c++latest.
  Zydis.c is C: compile it as C (cl /TC), not C++.
