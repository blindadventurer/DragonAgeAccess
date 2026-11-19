// Click-to-move without the mouse (DragonAge2.exe 1.04.8524.0).
//
// Proven by disassembly (2026-11-19, see NOTEBOOK):
//   GameModeExplore handler of MoveCharacterToLocationMessage (0x46AD90) needs the
//   PreMove flag (byte 0xD4FA0E) and the picked point [this+0xFC] (valid while byte
//   [this+0x100] is set), takes the selected creature (0x5A7220 on the app) and calls
//   0x459C30(this, creatureId, &point16, -1)   thiscall, ret 0xC, which stores the
//   point (movaps: 16-byte aligned x, y, z, w), looks the creature up (0x5A6E40),
//   refuses a dead one (vtable 0x1D8) or one without a mover ([obj+0x79C]), then
//   moves it (0x5E6D70 on the mover, 0x4591B0).
// The mod calls 0x459C30 with the captured explore (or combat) mode instance.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "log.h"
#include "move.h"
#include "objects.h"

namespace {

using MoveFn = unsigned char(__thiscall*)(void* mode, unsigned id, const float* point, int flag);
constexpr DWORD kMove = 0x459C30;

bool CodeMatches() {
    static const unsigned char move[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x51, 0x8B, 0x45, 0x08};
    return memcmp((void*)kMove, move, sizeof move) == 0;
}

int RawIssue(void* mode, unsigned id, const float* point, int* ok) {
    __try {
        *ok = ((MoveFn)kMove)(mode, id, point, -1);
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

}  // namespace

bool MoveCreatureTo(unsigned id, double x, double y, double playerX, double playerY) {
    if (!CodeMatches()) {
        LogLine(L"move: code bytes differ from the proven build, refused");
        return false;
    }
    void* mode = ExploreMode() ? ExploreMode() : CombatMode();
    if (!mode) {
        LogLine(L"move: no explore mode captured yet, refused");
        return false;
    }
    CalibratePos(id, playerX, playerY);
    RawObject o;
    if (!ReadObject(id, o) || !o.posOk || PosOffset() < 0) {
        LogLine(L"move: creature " + std::to_wstring(id) + L" or its position not found, refused");
        return false;
    }
    // Same height as the creature: the pathfinder snaps to the floor (HYPOTHESIS,
    // as in Origins where it worked).
    __declspec(align(16)) float point[4] = {(float)x, (float)y, o.z, 0.0f};
    int ok = 0;
    int r = RawIssue(mode, id, point, &ok);
    wchar_t b[200];
    swprintf_s(b, L"move: id %u to %.2f %.2f %.2f -> call %d, said %d", id, x, y, o.z, r, ok);
    LogLine(b);
    return r == 1 && ok != 0;
}
