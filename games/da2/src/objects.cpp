// The engine's world objects, read directly (DragonAge2.exe 1.04.8524.0).
//
// Proven by disassembly (2026-11-19, see NOTEBOOK):
//   app = *(void**)0xD54494 (client app; every handler below loads it).
//   0x5A70C0  thiscall(app, id), ret 4: object lookup by id; returns the client
//             object (lookup 0x8A23C0, then vtable slot 6 AsClientObject) or 0.
//   0x459D20  cdecl(obj): the cursor action code (0 none, 3 talk, 4 attack,
//             7 pick up, 8 use / loot, -1 own party), read from:
//     byte +0x14 object type (6 creature, 7 item, 10 placeable);
//     vtable slot 4 (0x10) AsCreature, slot 9 (0x24) AsPlaceable;
//     creature: vtable slot 0x1D8 == 1 -> dead (then 0x5433B0(creature) = body
//       can be looted -> 8); byte +0x404 -> 4 (hostile); byte +0x791 & 4 -> no
//       action; byte +0x791 & 0x40 -> 3 (talk);
//     placeable: byte +0xEB & 8 = usable; vtable slot 0x104 (0) and (1) = the two
//       actions of its state (placeable_interactions ids) -> 8.
//   0x48D190  thiscall(mode, obj, action, 0, 0), ret 0x10: what
//             PerformDefaultActionOnTargetMessage (handler 0x4964E0) calls.
//   GameModeExplore vtable 0xBE13FC (36 slots, offset 0), GameModeCombat 0xBEEE44
//   (42 slots): their instances are captured by thunks (mov [g], ecx; jmp orig).
//   World position: found at run time (CalibratePos) by searching the leader's
//   object for the catalogue position.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "log.h"
#include "objects.h"

namespace {

using LookupFn = void*(__thiscall*)(void* app, unsigned id);
using SlotFn = void*(__thiscall*)(void* self);
using IntFn = int(__thiscall*)(void* self);
using ByteFn = unsigned char(__thiscall*)(void* self);
using PlcActionFn = unsigned short(__thiscall*)(void* self, int which);
using ActionFn = int(__cdecl*)(void* obj);
using DefaultActionFn = unsigned char(__thiscall*)(void* mode, void* obj, int action, int a, int b);

constexpr DWORD kAppPtr = 0xD54494;
constexpr DWORD kLookup = 0x5A70C0, kAction = 0x459D20, kDefaultAction = 0x48D190, kLootable = 0x5433B0;
constexpr DWORD kExploreVt = 0xBE13FC, kCombatVt = 0xBEEE44;
constexpr int kExploreSlots = 36, kCombatSlots = 42;
constexpr DWORD kExeLo = 0x400000, kExeHi = 0x400000 + 0xBD5000;

void* volatile g_explore = nullptr;
void* volatile g_combat = nullptr;
int g_posOff = -1;

bool InExe(const void* p) { return (DWORD)p >= kExeLo && (DWORD)p < kExeHi; }

void* App() { return *(void**)kAppPtr; }

void ClassName(void* obj, char* out, size_t cap) {
    out[0] = 0;
    __try {
        void** vt = *(void***)obj;
        if (!InExe(vt)) return;
        const DWORD* col = (const DWORD*)vt[-1];
        if (!InExe(col)) return;
        const char* td = (const char*)col[3];
        if (!InExe(td)) return;
        const char* name = td + 8;
        if (strncmp(name, ".?AV", 4) != 0) return;
        name += 4;
        size_t i = 0;
        while (name[i] && !(name[i] == '@' && name[i + 1] == '@') && i + 1 < cap) { out[i] = name[i]; ++i; }
        out[i] = 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out[0] = 0;
    }
}

void* Lookup(unsigned id) {
    __try {
        void* app = App();
        if (!app) return nullptr;
        return ((LookupFn)kLookup)(app, id);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool ReadOne(unsigned id, RawObject* r) {
    void* obj = Lookup(id);
    if (!obj) return false;
    __try {
        void** ovt = *(void***)obj;
        if (!InExe(ovt)) return false;
        const unsigned char* b = (const unsigned char*)obj;
        r->id = id;
        r->ptr = obj;
        r->type = b[0x14];
        if (g_posOff >= 0) {
            r->x = *(const float*)(b + g_posOff);
            r->y = *(const float*)(b + g_posOff + 4);
            r->z = *(const float*)(b + g_posOff + 8);
        } else {
            r->posOk = false;
        }
        r->usable = false;
        r->flag7f0 = 0;
        r->dead = false;
        r->lootable = false;
        r->talk = false;
        r->inPlay = true;
        r->sel23c = r->sel23d = 1;
        void* plc = ((SlotFn)ovt[9])(obj);
        if (plc) {
            const unsigned char* pb = (const unsigned char*)plc;
            r->usable = (pb[0xEB] & 8) != 0;
            void** pvt = *(void***)plc;
            r->act0 = ((PlcActionFn)pvt[0x104 / 4])(plc, 0);
            r->act1 = ((PlcActionFn)pvt[0x104 / 4])(plc, 1);
        }
        void* cre = ((SlotFn)ovt[4])(obj);
        if (cre) {
            const unsigned char* cb = (const unsigned char*)cre;
            void** cvt = *(void***)cre;
            r->flag7f0 = (cb[0x791] & 4) ? 1 : 0;
            r->talk = (cb[0x791] & 0x40) != 0;
            r->dead = ((IntFn)cvt[0x1D8 / 4])(cre) == 1;
            r->inPlay = (b[0x2E7] & 0x80) == 0;  // bit set = dormant (objects.h inPlay)
            if (r->dead) r->lootable = ((ByteFn)kLootable)(cre) != 0;
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int FindPos(const unsigned char* obj, double x, double y) {
    __try {
        for (int off = 0x18; off < 0x1000; off += 4) {
            float fx = *(const float*)(obj + off), fy = *(const float*)(obj + off + 4);
            if (std::fabs(fx - x) < 0.05 && std::fabs(fy - y) < 0.05) return off;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return -1;
}

// ---------------------------------------------------------------- mode capture

unsigned char* g_thunks = nullptr;
void* g_origExplore[kExploreSlots];
void* g_origCombat[kCombatSlots];

void WriteThunk(unsigned char*& p, void* volatile* target, void** orig) {
    *p++ = 0x89; *p++ = 0x0D; *(DWORD*)p = (DWORD)target; p += 4;
    *p++ = 0xFF; *p++ = 0x25; *(DWORD*)p = (DWORD)orig; p += 4;
}

bool PatchVtable(DWORD vtAddr, int slots, void** orig, void* volatile* target, unsigned char*& p) {
    void** vt = (void**)vtAddr;
    for (int i = 0; i < slots; ++i)
        if (!InExe(vt[i])) return false;
    DWORD old = 0;
    if (!VirtualProtect(vt, slots * sizeof(void*), PAGE_READWRITE, &old)) return false;
    for (int i = 0; i < slots; ++i) {
        orig[i] = vt[i];
        unsigned char* thunk = p;
        WriteThunk(p, target, &orig[i]);
        vt[i] = thunk;
    }
    VirtualProtect(vt, slots * sizeof(void*), old, &old);
    return true;
}

bool TryWString(const unsigned char* addr, wchar_t* out, size_t cap, bool* latinOnly) {
    __try {
        DWORD size = *(const DWORD*)(addr + 0x14), res = *(const DWORD*)(addr + 0x18);
        if (res < 7 || res > 4096 || size == 0 || size > res || size >= cap) return false;
        const wchar_t* chars = res < 8 ? (const wchar_t*)(addr + 4) : *(const wchar_t* const*)(addr + 4);
        if (!chars || IsBadReadPtr(chars, size * 2)) return false;
        *latinOnly = true;
        for (DWORD i = 0; i < size; ++i) {
            wchar_t c = chars[i];
            if (c < 0x20 || (c > 0x7E && c < 0x400) || c > 0x4FF) return false;
            if (c >= 0x400) *latinOnly = false;
            out[i] = c;
        }
        out[size] = 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// HYPOTHESIS (DA2 layout not proven): the first Latin string in the object's head is
// its tag, the first Cyrillic one its name. The scanner prefers catalogue names.
void FindStrings(void* obj, std::wstring& tag, std::wstring& name) {
    wchar_t buf[160];
    for (int off = 0x20; off + 0x1C <= 0x300 && (tag.empty() || name.empty()); off += 4) {
        bool latin = true;
        if (!TryWString((const unsigned char*)obj + off, buf, 160, &latin)) continue;
        if (latin && tag.empty()) tag = buf;
        else if (!latin && name.empty()) name = buf;
    }
}

}  // namespace

bool CalibratePos(unsigned id, double x, double y) {
    if (g_posOff >= 0) return true;
    void* obj = Lookup(id);
    if (!obj) return false;
    int off = FindPos((const unsigned char*)obj, x, y);
    wchar_t b[128];
    swprintf_s(b, L"objects: position of object %u (%.2f, %.2f) found at +0x%x", id, x, y, off);
    LogLine(b);
    if (off < 0) return false;
    g_posOff = off;
    return true;
}

int PosOffset() { return g_posOff; }

bool ReadObject(unsigned id, RawObject& out) {
    char cls[64];
    if (!ReadOne(id, &out)) return false;
    ClassName(out.ptr, cls, sizeof cls);
    out.cls = cls;
    return true;
}

std::wstring ObjectName(void* obj) { std::wstring t, n; FindStrings(obj, t, n); return n; }
std::wstring ObjectTag(void* obj) { std::wstring t, n; FindStrings(obj, t, n); return t; }

bool ObjectsInstall() {
    static const unsigned char lookup[] = {0x51, 0x8B, 0x54, 0x24, 0x08, 0x8B, 0x89, 0xFC, 0x00, 0x00};
    static const unsigned char action[] = {0x56, 0x8B, 0x74, 0x24, 0x08, 0x57, 0x85, 0xF6, 0x0F, 0x84};
    static const unsigned char defAction[] = {0x6A, 0xFF, 0x68, 0x21, 0x07, 0xB5, 0x00};
    static const unsigned char loot[] = {0x57, 0x8B, 0xF9, 0x8B, 0x07, 0x8B, 0x90, 0xD8, 0x01, 0x00};
    if (memcmp((void*)kLookup, lookup, sizeof lookup) || memcmp((void*)kAction, action, sizeof action) ||
        memcmp((void*)kDefaultAction, defAction, sizeof defAction) || memcmp((void*)kLootable, loot, sizeof loot)) {
        LogLine(L"objects: code bytes differ from the proven build, object access off");
        return false;
    }
    g_thunks = (unsigned char*)VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!g_thunks) return false;
    unsigned char* p = g_thunks;
    bool a = PatchVtable(kExploreVt, kExploreSlots, g_origExplore, &g_explore, p);
    bool b = PatchVtable(kCombatVt, kCombatSlots, g_origCombat, &g_combat, p);
    LogLine(std::wstring(L"objects: mode capture explore=") + (a ? L"on" : L"FAILED") + L" combat=" +
            (b ? L"on" : L"FAILED"));
    return a || b;
}

void* ExploreMode() { return g_explore; }
void* CombatMode() { return g_combat; }

// Party picker: handled in the menu reader by the cursor sweep (menu.cpp).
bool PartyCandidates(std::vector<PartyCandidate>& out, int& count) { out.clear(); count = 0; return false; }
std::string PartyToggle(unsigned) { return "error"; }

std::string ObjectClass(void* obj) {
    char buf[96];
    ClassName(obj, buf, sizeof buf);
    return buf;
}

size_t EnumObjects(std::vector<RawObject>& out, unsigned maxId) {
    static unsigned s_high = 0;
    static DWORD s_slowLogAt = 0;
    DWORD t0 = GetTickCount();
    out.clear();
    unsigned misses = 0;
    unsigned floorId = maxId > s_high ? maxId : s_high;
    for (unsigned id = 1; id <= 1000000; ++id) {
        if (id > floorId && misses > 30000) break;
        RawObject r;
        if (ReadOne(id, &r)) {
            if (id > s_high) s_high = id;
            r.cls = ObjectClass(r.ptr);
            FindStrings(r.ptr, r.tag, r.name);
            if (!std::isfinite(r.x) || !std::isfinite(r.y) || std::fabs(r.x) > 1e5 || std::fabs(r.y) > 1e5)
                r.posOk = false;
            out.push_back(r);
            misses = 0;
        } else {
            ++misses;
        }
    }
    DWORD ms = GetTickCount() - t0;
    if (ms > 40 && GetTickCount() - s_slowLogAt > 30000) {
        s_slowLogAt = GetTickCount();
        LogLine(L"objects: " + std::to_wstring(out.size()) + L" read in " + std::to_wstring(ms) +
                L" ms, highest id " + std::to_wstring(s_high));
    }
    return out.size();
}

bool ReadObjectBytes(unsigned id, unsigned char* buf, size_t n) {
    void* obj = Lookup(id);
    if (!obj) return false;
    __try {
        memcpy(buf, obj, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

namespace {
int RawInteract(void* mode, void* obj, int* action, int* result) {
    __try {
        *action = ((ActionFn)kAction)(obj);
        if (*action <= 0) return 2;
        // As the handler 0x4964E0 does for actions 2, 4 (attack) and 6 (its jump
        // table 0x4965E4): note the target id at [[app+0x100]+0xB0].
        if (*action == 2 || *action == 4 || *action == 6) {
            unsigned char* t = *(unsigned char**)((unsigned char*)App() + 0x100);
            if (t) *(unsigned*)(t + 0xB0) = *(const unsigned*)((const unsigned char*)obj + 0x10);
        }
        *result = ((DefaultActionFn)kDefaultAction)(mode, obj, *action, 0, 0);
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}
}  // namespace

int InteractWith(unsigned id, bool inCombat) {
    void* mode = inCombat ? (g_combat ? g_combat : g_explore) : (g_explore ? g_explore : g_combat);
    if (!mode) {
        LogLine(L"interact: no explore or combat mode captured yet");
        return -2;
    }
    void* obj = Lookup(id);
    if (!obj) {
        LogLine(L"interact: object " + std::to_wstring(id) + L" not found");
        return -1;
    }
    int action = -1, result = -1;
    int r = RawInteract(mode, obj, &action, &result);
    wchar_t b[160];
    swprintf_s(b, L"interact: id %u mode %p (%s) -> %d, action %d, result %d", id, mode,
               mode == g_combat ? L"combat" : L"explore", r, action, result);
    LogLine(b);
    return r == 1 ? action : (r == 2 ? 0 : -1);
}
