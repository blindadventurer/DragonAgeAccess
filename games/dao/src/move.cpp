// Click-to-move without the mouse: the same calls the game makes when the left
// button is released over the ground in explore mode.
//
// Proven by disassembly of DAOrigins.exe 1.05.13263.0 (2026-11-17, see NOTEBOOK):
//   GameModeExplore handler of MoveCharacterToLocationMessage (0x70C9A0) needs the
//   PreMove flag (byte 0xC35E77) and the picked point [this+0xD0], then calls
//   0x70BE80(this, creatureId, &point16, -1), which does:
//     obj = 0x5D58C0(app = *(void**)0xC327EC, id)       thiscall, ret 4
//     if (obj->vtable[0x1D0/4]() != 0) stop
//     mc = *(void**)(obj + 0x764); if (!mc) stop
//     ok = 0x764020(mc, &point16, -1)                    thiscall, ret 8
//     0x7F6370(obj, -1)                                  cdecl
// The point is a 16-byte aligned x, y, z, w float vector (movaps).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "log.h"
#include "move.h"

namespace {

using GetObjFn = void*(__thiscall*)(void* app, unsigned id);
using BusyFn = int(__thiscall*)(void* obj);
using MoveFn = unsigned char(__thiscall*)(void* mc, const float* point, int flag);
using CmdFn = void(__cdecl*)(void* obj, int flag);

constexpr DWORD kAppPtr = 0xC327EC;
constexpr DWORD kGetObj = 0x5D58C0, kMove = 0x764020, kCmd = 0x7F6370;
constexpr DWORD kMoverOffset = 0x764, kBusySlot = 0x1D0 / 4;

// First bytes of each function on the proven build; refuse to call otherwise.
bool CodeMatches() {
    static const unsigned char getObj[] = {0x51, 0x8B, 0x54, 0x24, 0x08};
    static const unsigned char move[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0};
    static const unsigned char cmd[] = {0x6A, 0xFF, 0x68, 0x7B, 0xDC, 0xAA, 0x00};
    return memcmp((void*)kGetObj, getObj, sizeof getObj) == 0 && memcmp((void*)kMove, move, sizeof move) == 0 &&
           memcmp((void*)kCmd, cmd, sizeof cmd) == 0;
}

int g_posOffset = -1;  // where the creature keeps its world x, y, z (found at run time)

bool Near(float a, double b) { return std::fabs(a - b) < 0.05; }

// Inside SEH: no C++ objects with destructors in these functions.
int FindPosOffset(const unsigned char* obj, double x, double y) {
    __try {
        for (int off = 0x10; off < 0x800; off += 4) {
            float fx = *(const float*)(obj + off), fy = *(const float*)(obj + off + 4);
            if (Near(fx, x) && Near(fy, y)) return off;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return -1;
}

int RawMove(unsigned id, const float* point, void** objOut, float* zOut, double x, double y, int* stage) {
    __try {
        *stage = 1;
        void* app = *(void**)kAppPtr;
        if (!app) return 0;
        *stage = 2;
        void* obj = ((GetObjFn)kGetObj)(app, id);
        *objOut = obj;
        if (!obj) return 0;
        *stage = 3;
        if (g_posOffset < 0) g_posOffset = FindPosOffset((const unsigned char*)obj, x, y);
        if (g_posOffset >= 0) *zOut = *(const float*)((const unsigned char*)obj + g_posOffset + 8);
        *stage = 4;
        void** vt = *(void***)obj;
        if (((BusyFn)vt[kBusySlot])(obj) != 0) return 0;
        *stage = 5;
        void* mc = *(void**)((unsigned char*)obj + kMoverOffset);
        if (!mc) return 0;
        *stage = 6;
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int RawIssue(void* obj, const float* point, int* ok) {
    __try {
        void* mc = *(void**)((unsigned char*)obj + kMoverOffset);
        *ok = ((MoveFn)kMove)(mc, point, -1);
        ((CmdFn)kCmd)(obj, -1);
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
    __declspec(align(16)) float point[4] = {(float)x, (float)y, 0.0f, 0.0f};
    void* obj = nullptr;
    float z = 0.0f;
    int stage = 0;
    int r = RawMove(id, point, &obj, &z, playerX, playerY, &stage);
    wchar_t b[200];
    swprintf_s(b, L"move: id %u stage %d result %d posOffset 0x%x z %.3f", id, stage, r, g_posOffset, z);
    LogLine(b);
    if (r != 1) return false;
    if (g_posOffset < 0) {
        LogLine(L"move: player position not found in the object, refused (no height)");
        return false;
    }
    point[2] = z;  // same height as the player: the pathfinder snaps to the floor (HYPOTHESIS)
    int ok = 0;
    r = RawIssue(obj, point, &ok);
    swprintf_s(b, L"move: issued to %.2f %.2f %.2f -> call %d, mover said %d", x, y, z, r, ok);
    LogLine(b);
    return r == 1;
}
