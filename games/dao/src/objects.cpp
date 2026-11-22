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
#include <algorithm>
#include <cmath>
#include <iterator>
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

namespace {
bool InstallItemHook();  // received items, below
}

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
    bool items = InstallItemHook();
    LogLine(std::wstring(L"objects: mode capture explore=") + (a ? L"on" : L"FAILED") + L" combat=" +
            (b ? L"on" : L"FAILED") + L" partypicker=" + (c ? L"on" : L"FAILED") + L" items=" +
            (items ? L"on" : L"FAILED"));
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

namespace {
// The engine's object manager (static study of the decrypted Steam exe, 2026-11-22):
//   0x4C5380 returns the manager, a pointer kept at 0xC2BE24; the id lookup 0xA58190
//   asks it through 0x4D5EB0 -> 0x8170B0 -> 0x6FE370.
//   The manager starts with a stdext::hash_map<id, object> (Visual C++ 2008): list
//   +4, its head node pointer +8, size +0xC, bucket vector +0x14..+0x18 (8 bytes a
//   bucket), mask +0x20. List node: +0 next, +4 prev, +8 key = object id (compared
//   unsigned by 0x6FE370), +0xC value. 0x4D5F00 walks exactly this list (head [m+8],
//   n = [n] until the head again) and collects every value, so the list holds every
//   object the lookup can find. We read only the ids and go through the proven
//   lookup for each, which takes and releases its own reference.
constexpr DWORD kObjMgrPtr = 0xC2BE24, kObjMgrGet = 0x4C5380, kObjMgrWalk = 0x4D5F00;

void ReadFull(RawObject& r) {
    r.cls = ObjectClass(r.ptr);
    r.name = ObjectName(r.ptr);
    r.tag = ObjectTag(r.ptr);
    if (!std::isfinite(r.x) || !std::isfinite(r.y) || std::fabs(r.x) > 1e5 || std::fabs(r.y) > 1e5)
        r.posOk = false;
}

// Ids of the manager's hash map; -1 when the list cannot be read (then the old walk).
int ListIds(unsigned* ids, int cap) {
    __try {
        const unsigned char* m = *(const unsigned char* const*)kObjMgrPtr;
        if (!m) return -1;
        unsigned size = *(const unsigned*)(m + 0xC);
        const unsigned char* head = *(const unsigned char* const*)(m + 8);
        if (!head || size > 1000000) return -1;
        int n = 0;
        unsigned steps = 0;
        for (const unsigned char* node = *(const unsigned char* const*)head; node != head;
             node = *(const unsigned char* const*)node) {
            if (!node || ++steps > size + 1) return -1;  // broken or changing list
            if (n < cap) ids[n++] = *(const unsigned*)(node + 8);
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool ListUsable() {
    // mov eax,[0xC2BE24] ; ret  /  sub esp,8 ; cmp dword [ecx+0xC],0 ; je
    static const unsigned char get[] = {0xA1, 0x24, 0xBE, 0xC2, 0x00, 0xC3};
    static const unsigned char walk[] = {0x83, 0xEC, 0x08, 0x83, 0x79, 0x0C, 0x00, 0x0F, 0x84};
    static int ok = -1;
    if (ok < 0) {
        ok = !memcmp((void*)kObjMgrGet, get, sizeof get) && !memcmp((void*)kObjMgrWalk, walk, sizeof walk);
        LogLine(ok ? L"objects: engine object list on" : L"objects: engine object list code differs, id walk used");
    }
    return ok == 1;
}

// The old way, kept as the fallback and for the comparison below. Object ids only
// grow during a session (test 00:58, the battle of Ostagar: every darkspawn was
// above the old fixed limit of 8000). The walk goes at least to maxId and to the
// highest id ever found, then on until 30000 ids in a row are free (4000 missed the
// Lothering chantry's people at 16927..17018, test 19:24).
void WalkIds(std::vector<RawObject>& out, unsigned maxId) {
    static unsigned s_high = 0;
    unsigned misses = 0;
    unsigned floorId = maxId > s_high ? maxId : s_high;
    for (unsigned id = 1; id <= 1000000; ++id) {
        if (id > floorId && misses > 30000) break;
        RawObject r;
        if (ReadOne(id, &r)) {
            if (id > s_high) s_high = id;
            out.push_back(r);
            misses = 0;
        } else {
            ++misses;
        }
    }
}

std::wstring IdList(const std::vector<unsigned>& v) {
    std::wstring s;
    for (size_t i = 0; i < v.size() && i < 40; ++i) s += L" " + std::to_wstring(v[i]);
    if (v.size() > 40) s += L" ...";
    return s;
}
}  // namespace

// Every object of the engine's object manager (its id list, see above), read through
// the proven lookup; the id walk only when the list's code is not the proven one.
// Until the list is proven in a game, once a minute both are run and compared (log
// "objects: list N ... walk M ...").
size_t EnumObjects(std::vector<RawObject>& out, unsigned maxId) {
    static DWORD s_slowLogAt = 0, s_compareAt = 0;
    static std::vector<unsigned> ids(65536);
    DWORD t0 = GetTickCount();
    out.clear();
    int n = ListUsable() ? ListIds(ids.data(), (int)ids.size()) : -1;
    if (n >= (int)ids.size()) {
        ids.resize(ids.size() * 4);
        n = ListIds(ids.data(), (int)ids.size());
    }
    if (n < 0) {
        static DWORD s_failLogAt = 0;
        if (ListUsable() && GetTickCount() - s_failLogAt > 30000) {
            s_failLogAt = GetTickCount();
            LogLine(L"objects: engine object list unreadable, id walk used");
        }
        WalkIds(out, maxId);
    } else {
        std::sort(ids.begin(), ids.begin() + n);
        for (int i = 0; i < n; ++i) {
            RawObject r;
            if (ReadOne(ids[i], &r)) out.push_back(r);
        }
    }
    for (RawObject& r : out) ReadFull(r);
    DWORD ms = GetTickCount() - t0;
    if (n >= 0 && GetTickCount() - s_compareAt > 60000) {
        s_compareAt = GetTickCount();
        DWORD t1 = GetTickCount();
        std::vector<RawObject> walked;
        WalkIds(walked, maxId);
        DWORD wms = GetTickCount() - t1;
        std::vector<unsigned> a, b, onlyList, onlyWalk;
        for (const RawObject& r : out) a.push_back(r.id);
        for (const RawObject& r : walked) b.push_back(r.id);
        std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(onlyList));
        std::set_difference(b.begin(), b.end(), a.begin(), a.end(), std::back_inserter(onlyWalk));
        LogLine(L"objects: list " + std::to_wstring(n) + L" ids, " + std::to_wstring(out.size()) + L" read in " +
                std::to_wstring(ms) + L" ms; walk " + std::to_wstring(walked.size()) + L" in " +
                std::to_wstring(wms) + L" ms; only in list:" + IdList(onlyList) + L" | only in walk:" +
                IdList(onlyWalk));
    } else if (ms > 40 && GetTickCount() - s_slowLogAt > 30000) {
        s_slowLogAt = GetTickCount();
        LogLine(L"objects: " + std::to_wstring(out.size()) + L" read in " + std::to_wstring(ms) + L" ms" +
                (n < 0 ? L" (id walk)" : L" (engine list)"));
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

// Ability targeting (static study of the decrypted Steam exe, 2026-11-22, Ghidra
// headless and capstone; tools\handlers.py found the GameModeExplore handlers):
//   PerformAbilityMessage handler 0x711B50 casts at once when the ability needs no
//   pick (self, or Lightning-like abilities with column 0x1C7CA631 = 1 on the current
//   attack target); otherwise 0x70BA30 enters the targeting state, a struct at
//   0xC35F60: ability id +0x40 (0xC35FA0), item +0x48 (0xC35FA8), TargetType mask
//   +0x4C (0xC35FAC), caster +0x50 (0xC35FB0). Shock (ABI_base 14000: TargetType
//   126, AOE_Type 2 cone) waits there for a mouse click, so the mod's Enter (the
//   default action, an attack) never cast it (test 2026-11-22 02:24-02:28).
//   PerformAbilityOnSelectedTargetMessage handler 0x711A00 is the click on a target:
//   mask = 0x70ABE0(ecx = 0xC35F60) (+0x4C while an ability is pending, else +0x54);
//   mode->0x70D550(obj, mask) (thiscall, ret 8) says whether the object is a valid
//   target; then mode->0x711680(ability, caster, item, targetId, &vec) (thiscall, ret
//   0x14) casts (0x7F6690) and leaves the targeting state (0x7113F0). The vector is
//   zero for a creature target; the ground click (0x7116C0) passes target -1 and the
//   picked point instead.
// Test 2026-11-22 16:19-16:25: every call of the old route (mask, 0x70D550, then
// 0x711680 with the raw id) faulted (-1) in combat mode, the ability stayed pending.
// The handler takes the target as [obj+0x10] (0x53CC40 on obj+4), not the id. Now
// the creature case calls the handler 0x711A00 itself, with the selected-target
// global 0xC06794 set to the id, exactly as the game's click does; a fault logs its
// code, address, step and the exe return addresses on the stack.
namespace {
constexpr DWORD kTargeting = 0xC35F60, kTgtMaskFn = 0x70ABE0, kTgtValid = 0x70D550, kTgtCast = 0x711680,
                kTgtClick = 0x711A00, kSelTarget = 0xC06794;
using MaskFn = unsigned(__thiscall*)(void* state);
using ValidFn = unsigned char(__thiscall*)(void* mode, void* obj, unsigned mask);
using CastFn = void(__thiscall*)(void* mode, unsigned ability, unsigned caster, unsigned item, unsigned target,
                                 float* vec);
using ClickFn = unsigned char(__thiscall*)(void* mode, void* msg);

bool TargetingCodeOk() {
    static int ok = -1;
    if (ok < 0) {
        static const unsigned char mask[] = {0x83, 0x79, 0x40, 0x00, 0x74, 0x04, 0x8B, 0x41, 0x4C, 0xC3};
        static const unsigned char valid[] = {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85, 0xF6};
        static const unsigned char cast[] = {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54, 0x24, 0x0C, 0x56};
        static const unsigned char click[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0xA1, 0x94, 0x67, 0xC0, 0x00};
        ok = !memcmp((void*)kTgtMaskFn, mask, sizeof mask) && !memcmp((void*)kTgtValid, valid, sizeof valid) &&
             !memcmp((void*)kTgtCast, cast, sizeof cast) && !memcmp((void*)kTgtClick, click, sizeof click);
        if (!ok) LogLine(L"objects: ability targeting code differs, pending abilities not cast by the mod");
    }
    return ok == 1;
}

// What the last fault was, for the log line (CastPendingOn).
struct CastFault {
    DWORD code = 0;
    void* addr = nullptr;
    DWORD ret[6] = {};
    int nret = 0;
};
CastFault g_fault;

int FaultFilter(EXCEPTION_POINTERS* p) {
    g_fault = CastFault();
    g_fault.code = p->ExceptionRecord->ExceptionCode;
    g_fault.addr = p->ExceptionRecord->ExceptionAddress;
    // Return addresses inside the exe's code (0x401000..0xAB0000) near the stack top.
    DWORD* sp = (DWORD*)p->ContextRecord->Esp;
    for (int i = 0; i < 96 && g_fault.nret < 6; ++i) {
        DWORD v = 0;
        if (!ReadProcessMemory(GetCurrentProcess(), sp + i, &v, sizeof v, nullptr)) break;
        if (v >= 0x401000 && v < 0xAB0000) g_fault.ret[g_fault.nret++] = v;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

// 1 cast, 3 cast at the target's place, -3 not a valid target, -4 the game kept the
// ability waiting, -1 fault (step in *step).
int RawCast(void* mode, unsigned id, bool atPlace, unsigned* ability, int* step) {
    unsigned* sel = (unsigned*)kSelTarget;
    unsigned oldSel = 0;
    bool selSet = false;
    __try {
        *step = 1;
        unsigned char* st = (unsigned char*)kTargeting;
        *ability = *(unsigned*)(st + 0x40);
        if (!*ability) return 0;
        void* outObj = nullptr;
        if (((LookupFn)kLookup)(id, &outObj) != 0 || !outObj) return -3;
        *step = 2;
        void* obj = ((SlotFn)(*(void***)outObj)[6])(outObj);
        if (!obj) return -3;
        unsigned caster = *(unsigned*)(st + 0x50), item = *(unsigned*)(st + 0x48);
        if (!atPlace) {
            *step = 3;
            unsigned mask = ((MaskFn)kTgtMaskFn)(st);
            *step = 4;
            if (!((ValidFn)kTgtValid)(mode, obj, mask)) return -3;
            *step = 5;
            oldSel = *sel;
            *sel = id;
            selSet = true;
            unsigned char msg[64] = {};
            ((ClickFn)kTgtClick)(mode, msg);
            *sel = oldSel;
            selSet = false;
            *step = 6;
            return *(unsigned*)(st + 0x40) == *ability ? -4 : 1;
        }
        *step = 7;
        float at[4];
        memcpy(at, (unsigned char*)obj + 0x120, sizeof at);
        ((CastFn)kTgtCast)(mode, *ability, caster, item, 0xFFFFFFFF, at);
        *step = 8;
        return 3;
    } __except (FaultFilter(GetExceptionInformation())) {
        if (selSet) *sel = oldSel;
        return -1;
    }
}
}  // namespace

unsigned PendingAbility() {
    if (!TargetingCodeOk()) return 0;
    __try {
        return *(unsigned*)(kTargeting + 0x40);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

int CastPendingOn(unsigned id, bool inCombat, unsigned* ability) {
    *ability = 0;
    if (!TargetingCodeOk()) return 0;
    void* mode = inCombat ? (g_combat ? g_combat : g_explore) : (g_explore ? g_explore : g_combat);
    if (!mode) return 0;
    int step = 0;
    int r = RawCast(mode, id, false, ability, &step);
    // Not a valid creature target (an ability aimed at the ground): its place.
    if (r == -3 && *ability) r = RawCast(mode, id, true, ability, &step);
    wchar_t b[320];
    swprintf_s(b, L"cast pending: ability %u on id %u mode %p (%s) -> %d step %d", *ability, id, mode,
               mode == g_combat ? L"combat" : L"explore", r, step);
    if (r == -1) {
        wchar_t f[200];
        swprintf_s(f, L"; fault %08X at %p, stack", g_fault.code, g_fault.addr);
        wcscat_s(b, f);
        for (int i = 0; i < g_fault.nret; ++i) {
            swprintf_s(f, L" %06X", g_fault.ret[i]);
            wcscat_s(b, f);
        }
    }
    LogLine(b);
    return r;
}

// Received items (static study 2026-11-22, Ghidra and capstone on the unpacked exe):
// the "Предметы получены" alert (type 3) is made by 0x629620, called only from
// 0x6313A0, slot 7 of CCParty's secondary vtable 0xB05DBC (thiscall, ret 4, one
// argument: the item object just added). That handler marks the item's
// NewlyAcquired (+0x620) and raises the alert. Item attributes (registration
// 0x7546F0): StackSize at +0x5F0 (value +0xC, as 0x53CC40 reads), NewlyAcquired
// +0x620, AbilityID +0x660; Name is the CCObject one at +0xD4 (0x699670), read by
// ObjectName. The hook reads name and stack size before the original runs (a stack
// may merge into another item and go away).
namespace {
constexpr DWORD kPartyItemVt = 0xB05DBC, kItemAdded = 0x6313A0;
constexpr int kItemAddedSlot = 7;
using ItemAddedFn = void(__thiscall*)(void* party, void* item);
ItemAddedFn g_origItemAdded = nullptr;

struct QueuedItem {
    ReceivedItem item;
    ULONGLONG at;
};
std::vector<QueuedItem> g_received;
CRITICAL_SECTION g_receivedLock;

int ReadStackSize(void* item) {
    __try {
        int n = *(const int*)((const unsigned char*)item + 0x5F0 + 0xC);
        return n > 0 && n < 10000 ? n : 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 1;
    }
}

void __fastcall ItemAddedHook(void* party, void*, void* item) {
    if (item) {
        QueuedItem q{{ObjectName(item), ReadStackSize(item)}, GetTickCount64()};
        LogLine(L"item added: \"" + q.item.name + L"\" x" + std::to_wstring(q.item.count));
        EnterCriticalSection(&g_receivedLock);
        if (g_received.size() >= 64) g_received.erase(g_received.begin());
        g_received.push_back(std::move(q));
        LeaveCriticalSection(&g_receivedLock);
    }
    g_origItemAdded(party, item);
}

bool InstallItemHook() {
    static const unsigned char head[] = {0x55, 0x8B, 0x6C, 0x24, 0x08, 0x56, 0x57, 0x55, 0x8B, 0xF9};
    void** slot = (void**)kPartyItemVt + kItemAddedSlot;
    if (memcmp((void*)kItemAdded, head, sizeof head) || *slot != (void*)kItemAdded) return false;
    InitializeCriticalSection(&g_receivedLock);
    DWORD old = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old)) return false;
    g_origItemAdded = (ItemAddedFn)*slot;
    *slot = (void*)&ItemAddedHook;
    VirtualProtect(slot, sizeof(void*), old, &old);
    return true;
}
}  // namespace

std::vector<ReceivedItem> TakeReceivedItems(unsigned maxAgeMs) {
    std::vector<ReceivedItem> out;
    if (!g_origItemAdded) return out;
    ULONGLONG now = GetTickCount64();
    EnterCriticalSection(&g_receivedLock);
    for (auto& q : g_received)
        if (now - q.at <= maxAgeMs) out.push_back(q.item);
    g_received.clear();
    LeaveCriticalSection(&g_receivedLock);
    return out;
}

// Origins keeps the world position at a known offset (obj+0x120, see the top of this
// file), so the shared scanner never needs to calibrate (objects.h).
bool CalibratePos(unsigned, double, double) { return true; }
int PosOffset() { return 0x120; }
