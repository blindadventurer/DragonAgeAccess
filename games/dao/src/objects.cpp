// The engine's world objects, read directly (DAOrigins.exe 1.05.13263.0).
//
// Proven by disassembly (2026-11-17, see NOTEBOOK):
//   0xA58190(id, &out)  __stdcall, ret 8: object lookup by id; returns al == 0 when
//                        found (the game compares with byte 0xB59409 = 0).
//   out->vtable[6]()     (slot 0x18) CCObject::AsClientObject: this for client objects,
//                        0 for others (0x5D5BE0 does exactly this).
//   obj->vtable[4]()     AsCreature, obj->vtable[9]() AsPlaceable (this or 0).
//   byte obj+0x14        object type: 6 creature, 7 item, 10 placeable (cursor code
//                        0x70CAD0); placeable byte +0x42C non-zero = usable.
//   obj+0x120            world x, y, z floats (found on the leader in test 02:42).
//   MSVC RTTI on the vtable gives the class: CCCreature, CCPlaceable,
//   CCTriggerAreaTransition, CCWayPoint, CCItem...
// Interaction (PerformDefaultActionOnTargetMessage handler 0x713350):
//   action = 0x70CAD0(obj) (cdecl), then GameModeExplore::0x712830(obj, action, 0)
//   (thiscall, ret 0xC). The explore (or combat) mode instance is captured by thunks
//   on its vtables (GameModeExplore 0xB16374, 32 slots; GameModeCombat 0xB1D89C,
//   32 slots), which record ecx and jump to the original. Slot counts read from the
//   decrypted Steam exe 2026-11-20: after the last function pointer comes data (a
//   UTF-16 string, floats) whose values happen to fall in the exe's range, so the
//   InExe check cannot catch a count that is too high.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "log.h"
#include "objects.h"

namespace {

using LookupFn = unsigned char(__stdcall*)(unsigned id, void** out);
using SlotFn = void*(__thiscall*)(void* self);
using IntFn = int(__thiscall*)(void* self);
using ByteFn = unsigned char(__thiscall*)(void* self);
using PlcActionFn = unsigned short(__thiscall*)(void* self, int which);
using ActionFn = int(__cdecl*)(void* obj);
using DefaultActionFn = unsigned char(__thiscall*)(void* mode, void* obj, int action, int flag);

constexpr DWORD kLookup = 0xA58190, kAction = 0x70CAD0, kDefaultAction = 0x712830, kLootable = 0x6AB100;
constexpr DWORD kExploreVt = 0xB16374, kCombatVt = 0xB1D89C;
constexpr int kExploreSlots = 32, kCombatSlots = 32;  // 39 before 2026-11-20 overwrote "Target Required"
// GameModePartyPicker (RTTI vtable, 30 slots). Its click handler
// SelectPartyMemberUnderCursorMessage 0x768AE0: skips while bytes +0xA0 / +0xA1
// are set; finds the entry under the mouse (0x7675B0) in the array +0x8C..+0x90
// (0x34 bytes each: +0 creature id, +4 creature object, +8 state, +0x10 locked
// byte, +0x24 slot); state 2 -> 0x766040(entry) adds (state 1, word +0xA2 = count,
// refused at 4), state 1 -> 0x7661B0(entry) removes; then 0x7658E0 on the entry.
constexpr DWORD kPickerVt = 0xB1D31C, kPickerAdd = 0x766040, kPickerRemove = 0x7661B0, kPickerAfter = 0x7658E0;
constexpr int kPickerSlots = 30;  // 39 before 2026-11-20 overwrote "LockedPartyMembers"
constexpr DWORD kExeLo = 0x400000, kExeHi = 0x400000 + 0x9EC0A2;

void* volatile g_explore = nullptr;
void* volatile g_combat = nullptr;
void* volatile g_picker = nullptr;

bool InExe(const void* p) { return (DWORD)p >= kExeLo && (DWORD)p < kExeHi; }

// RTTI class name of an object (".?AVCCCreature@@" -> "CCCreature"), or "".
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

// One object by id; false when the id is free or anything faults.
bool ReadOne(unsigned id, RawObject* r) {
    __try {
        void* out = nullptr;
        if (((LookupFn)kLookup)(id, &out) != 0 || !out) return false;
        void** vt = *(void***)out;
        if (!InExe(vt)) return false;
        void* obj = ((SlotFn)vt[6])(out);
        if (!obj) return false;
        const unsigned char* b = (const unsigned char*)obj;
        r->id = id;
        r->ptr = obj;
        r->type = b[0x14];
        r->x = *(const float*)(b + 0x120);
        r->y = *(const float*)(b + 0x124);
        r->z = *(const float*)(b + 0x128);
        r->usable = false;
        r->flag7f0 = 0;
        r->dead = false;
        r->lootable = false;
        r->sel23c = b[0x23C];
        r->sel23d = b[0x23D];
        r->inPlay = b[0x2C5] != 0;  // CCObject "active" byte (objects.h inPlay)
        void** ovt = *(void***)obj;
        void* plc = ((SlotFn)ovt[9])(obj);
        if (plc) {  // placeable
            r->usable = b[0x42C] != 0;
            // The placeable's two actions in its current state (CCPlaceable vtable
            // slot 0x100 = 0x74C720 with 0 and 1, as the cursor code 0x70CBFB asks):
            // placeable_interactions.gda ids, 1 Open, 3 Unlock, 5 Use, 10 Dialog,
            // 11 Loot, 15 Area Transition.
            void** pvt = *(void***)plc;
            r->act0 = ((PlcActionFn)pvt[0x100 / 4])(plc, 0);
            r->act1 = ((PlcActionFn)pvt[0x100 / 4])(plc, 1);
        }
        if (((SlotFn)ovt[4])(obj)) {  // creature
            // Cursor code 0x70CAD0: +0x7F0 non-zero = no action at all; else hostile
            // (0x838210 on +0x4E8) = attack; else byte +0x860 non-zero = talk.
            r->flag7f0 = *(const int*)(b + 0x7F0);
            r->talk = b[0x860] != 0;
            // CCCreature vtable slot 0x1D0 (0x6A3660): dead = current health <= 0;
            // 0x6AB100: the body can be looted (cursor code 0x70CB99).
            r->dead = ((IntFn)ovt[0x1D0 / 4])(obj) != 0;
            if (r->dead) r->lootable = ((ByteFn)kLootable)(obj) != 0;
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// ---------------------------------------------------------------- mode capture

unsigned char* g_thunks = nullptr;
void* g_origExplore[kExploreSlots];
void* g_origCombat[kCombatSlots];
void* g_origPicker[kPickerSlots];

// Thunk per slot: mov [target], ecx ; jmp dword ptr [orig]
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

// A Visual C++ 2008 std::wstring at addr (alloc +0, buffer or pointer +4, size
// +0x14, capacity +0x18; inline while capacity < 8), copied into out.
bool TryWString(const unsigned char* addr, wchar_t* out, size_t cap) {
    __try {
        DWORD size = *(const DWORD*)(addr + 0x14), res = *(const DWORD*)(addr + 0x18);
        if (res < 7 || res > 4096 || size == 0 || size > res || size >= cap) return false;
        const wchar_t* chars = res < 8 ? (const wchar_t*)(addr + 4) : *(const wchar_t* const*)(addr + 4);
        if (!chars || IsBadReadPtr(chars, size * 2)) return false;
        for (DWORD i = 0; i < size; ++i) {
            wchar_t c = chars[i];
            if (c < 0x20 || (c > 0x7E && c < 0x400) || c > 0x4FF) return false;  // Latin, Cyrillic only
            out[i] = c;
        }
        out[size] = 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// First plausible string inside [from, to) of the object (catalogue attribute
// storage: Tag at +0x78, Name at +0xD4, registered by the CCObject constructor 0x699670).
std::wstring FindWString(void* obj, int from, int to) {
    wchar_t buf[160];
    for (int off = from; off + 0x1C <= to; off += 4)
        if (TryWString((const unsigned char*)obj + off, buf, 160)) return buf;
    return L"";
}

}  // namespace

bool ReadObject(unsigned id, RawObject& out) {
    char cls[64];
    if (!ReadOne(id, &out)) return false;
    ClassName(out.ptr, cls, sizeof cls);
    out.cls = cls;
    return true;
}

std::wstring ObjectName(void* obj) { return FindWString(obj, 0xD4, 0x108 + 0x1C); }
std::wstring ObjectTag(void* obj) { return FindWString(obj, 0x78, 0xD4 + 0x1C); }

bool ObjectsInstall() {
    // Code bytes of the functions we call, on the proven build.
    static const unsigned char lookup[] = {0x8B, 0x44, 0x24, 0x04, 0x56, 0x8B, 0x74, 0x24};
    static const unsigned char action[] = {0x53, 0x56, 0x8B, 0x74, 0x24, 0x0C, 0x85, 0xF6};
    static const unsigned char defAction[] = {0x6A, 0xFF, 0x68, 0xF6, 0xB4, 0xAA, 0x00};
    if (memcmp((void*)kLookup, lookup, sizeof lookup) || memcmp((void*)kAction, action, sizeof action) ||
        memcmp((void*)kDefaultAction, defAction, sizeof defAction)) {
        LogLine(L"objects: code bytes differ from the proven build, object access off");
        return false;
    }
    g_thunks = (unsigned char*)VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!g_thunks) return false;
    unsigned char* p = g_thunks;
    bool a = PatchVtable(kExploreVt, kExploreSlots, g_origExplore, &g_explore, p);
    bool b = PatchVtable(kCombatVt, kCombatSlots, g_origCombat, &g_combat, p);
    static const unsigned char add[] = {0x51, 0x53, 0x8B, 0xD9, 0x66, 0x83, 0xBB, 0xA2};
    static const unsigned char rem[] = {0x53, 0x8B, 0xD9, 0x66, 0x83, 0xBB, 0xA2, 0x00};
    bool c = !memcmp((void*)kPickerAdd, add, sizeof add) && !memcmp((void*)kPickerRemove, rem, sizeof rem) &&
             PatchVtable(kPickerVt, kPickerSlots, g_origPicker, &g_picker, p);
    LogLine(std::wstring(L"objects: mode capture explore=") + (a ? L"on" : L"FAILED") + L" combat=" +
            (b ? L"on" : L"FAILED") + L" partypicker=" + (c ? L"on" : L"FAILED"));
    return a || b;
}

void* ExploreMode() { return g_explore; }

namespace {
using EntryFn = unsigned char(__thiscall*)(void* mode, void* entry);
using AfterFn = void(__thiscall*)(void* entry);

int RawCandidates(void* mode, PartyCandidate* out, int cap, int* count) {
    __try {
        const unsigned char* m = (const unsigned char*)mode;
        const unsigned char* b = *(const unsigned char* const*)(m + 0x8C);
        const unsigned char* e = *(const unsigned char* const*)(m + 0x90);
        *count = *(const short*)(m + 0xA2);
        if (!b || e < b) return 0;
        int n = (int)((e - b) / 0x34);
        if (n > cap) n = cap;
        for (int i = 0; i < n; ++i) {
            const unsigned char* en = b + i * 0x34;
            out[i].id = *(const unsigned*)(en + 0);
            out[i].state = *(const int*)(en + 8);
            out[i].locked = en[0x10] != 0;
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// 1 added, 2 removed, 3 full, 4 locked, 5 busy, 6 missing, -1 fault
int RawToggle(void* mode, unsigned id) {
    __try {
        unsigned char* m = (unsigned char*)mode;
        if (m[0xA0] || m[0xA1]) return 5;
        unsigned char* b = *(unsigned char**)(m + 0x8C);
        unsigned char* e = *(unsigned char**)(m + 0x90);
        if (!b || e < b) return 6;
        for (unsigned char* en = b; en + 0x34 <= e; en += 0x34) {
            if (*(unsigned*)en != id) continue;
            if (en[0x10]) return 4;
            int st = *(int*)(en + 8);
            if (st == 2) {
                if (*(short*)(m + 0xA2) >= 4) return 3;
                ((EntryFn)kPickerAdd)(mode, en);
                ((AfterFn)kPickerAfter)(en);
                return 1;
            }
            if (st == 1) {
                ((EntryFn)kPickerRemove)(mode, en);
                ((AfterFn)kPickerAfter)(en);
                return 2;
            }
            return 5;
        }
        return 6;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}
}  // namespace

bool PartyCandidates(std::vector<PartyCandidate>& out, int& count) {
    out.clear();
    count = 0;
    if (!g_picker) return false;
    PartyCandidate buf[32];
    int n = RawCandidates(g_picker, buf, 32, &count);
    if (n < 0) return false;
    for (int i = 0; i < n; ++i) {
        RawObject r;
        if (ReadObject(buf[i].id, r)) buf[i].name = ObjectName(r.ptr);
        out.push_back(buf[i]);
    }
    return true;
}

std::string PartyToggle(unsigned id) {
    if (!g_picker) return "missing";
    int r = RawToggle(g_picker, id);
    wchar_t b[96];
    swprintf_s(b, L"party toggle id %u -> %d", id, r);
    LogLine(b);
    switch (r) {
        case 1: return "added";
        case 2: return "removed";
        case 3: return "full";
        case 4: return "locked";
        case 5: return "busy";
        case 6: return "missing";
    }
    return "error";
}
void* CombatMode() { return g_combat; }

std::string ObjectClass(void* obj) {
    char buf[96];
    ClassName(obj, buf, sizeof buf);
    return buf;
}

// Object ids only grow during a session (test 00:58, the battle of Ostagar: a
// party mage was 8644 and every darkspawn was above the old fixed limit of 8000,
// so the scanner saw no enemy at all). The walk goes at least to maxId and to the
// highest id ever found, then on until 30000 ids in a row are free. 4000 was too
// few: a save loaded inside the Lothering chantry (test 19:24) had the party at
// 130..7330 and the chantry's people at 16927..17018, so the walk stopped before
// them and the scanner listed only the party.
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
            r.name = ObjectName(r.ptr);
            r.tag = ObjectTag(r.ptr);
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
    __try {
        void* out = nullptr;
        if (((LookupFn)kLookup)(id, &out) != 0 || !out) return false;
        void* obj = ((SlotFn)(*(void***)out)[6])(out);
        if (!obj) return false;
        memcpy(buf, obj, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

namespace {
int RawInteract(void* mode, unsigned id, int* action, int* result) {
    __try {
        void* outObj = nullptr;
        if (((LookupFn)kLookup)(id, &outObj) != 0 || !outObj) return 0;
        void* obj = ((SlotFn)(*(void***)outObj)[6])(outObj);
        if (!obj) return 0;
        *action = ((ActionFn)kAction)(obj);
        if (*action == 0) return 2;
        *result = ((DefaultActionFn)kDefaultAction)(mode, obj, *action, 0);
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
    int action = -1, result = -1;
    int r = RawInteract(mode, id, &action, &result);
    wchar_t b[160];
    swprintf_s(b, L"interact: id %u mode %p -> %d, action %d, result %d", id, mode, r, action, result);
    LogLine(b);
    return r == 1 ? action : (r == 2 ? 0 : -1);
}

// Origins keeps the world position at a known offset (obj+0x120, see the top of this
// file), so the shared scanner never needs to calibrate (objects.h).
bool CalibratePos(unsigned, double, double) { return true; }
int PosOffset() { return 0x120; }
