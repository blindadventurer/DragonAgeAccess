// Scaleform GFx 2.x access: Advance hook by vtable slot replacement, GetVariable, Invoke.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include "gfx.h"
#include "log.h"

namespace {

// Addresses for DAOrigins.exe 1.05.13263.0 (PE stamp 0x4eb19d53, image base 0x400000, no ASLR).
// Kept as RVAs and rebased on the running module in case the base ever differs.
constexpr DWORD kVtableRva      = 0xB4CA6C - 0x400000;  // GFxMovieRoot vtable
constexpr DWORD kGetVariableRva = 0x954AD0 - 0x400000;  // slot 12
constexpr DWORD kInvokeRva      = 0x956E80 - 0x400000;  // slot 18
constexpr DWORD kAdvanceRva     = 0x959590 - 0x400000;  // slot 30
constexpr int kSlotGetVariable = 12, kSlotInvoke = 18, kSlotAdvance = 30;

using GetVariableFn = bool(__thiscall*)(void* self, GFxValue* out, const char* path);
using InvokeFn = bool(__thiscall*)(void* self, const char* method, GFxValue* result, const GFxValue* args, unsigned n);
using AdvanceFn = float(__thiscall*)(void* self, float dt, unsigned catchUp);

GetVariableFn g_getVariable = nullptr;
InvokeFn g_invoke = nullptr;
AdvanceFn g_advance = nullptr;
volatile bool g_readerDisabled = false;

void LogCrash(DWORD code) {
    wchar_t m[128];
    swprintf_s(m, L"menu reader crashed (code 0x%08lx), disabled for this session", code);
    LogLineNow(m);
}

// SEH wrapper: no C++ objects with destructors may live in this function.
void SafeOnMovieAdvance(void* movie) {
    DWORD code = 0;
    __try {
        OnMovieAdvance(movie);
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        g_readerDisabled = true;
        LogCrash(code);
    }
}

// Timing (player report 2026-11-18: heavy lag of the whole game and the scanner):
// the game's own Advance and our work are timed apart, and gaps between Advance
// calls (frame stalls) are counted. Every 10 s with a stall or with our share
// above 30 ms a summary is logged ("perf: ...").
struct Perf {
    LARGE_INTEGER freq{};
    double modMs = 0, gameMs = 0, worstMod = 0, worstGap = 0;
    void* worstMovie = nullptr;
    int stalls = 0, calls = 0;
    LARGE_INTEGER last{}, windowStart{};
} g_perf;

double Ms(const LARGE_INTEGER& a, const LARGE_INTEGER& b) {
    return (double)(b.QuadPart - a.QuadPart) * 1000.0 / (double)g_perf.freq.QuadPart;
}

void PerfReport(const LARGE_INTEGER& now) {
    if (g_perf.stalls > 0 || g_perf.modMs > 30.0) {
        wchar_t b[300];
        swprintf_s(b, L"perf: %.0f s, mod %.0f ms (worst call %.0f ms in %s), game advance %.0f ms, %d calls, "
                      L"stalls over 150 ms: %d (worst %.0f ms)",
                   Ms(g_perf.windowStart, now) / 1000.0, g_perf.modMs, g_perf.worstMod,
                   MovieFileName(g_perf.worstMovie), g_perf.gameMs, g_perf.calls, g_perf.stalls, g_perf.worstGap);
        LogLine(b);
    }
    LARGE_INTEGER f = g_perf.freq;
    g_perf = Perf{};
    g_perf.freq = f;
    g_perf.windowStart = now;
    g_perf.last = now;
}

float __fastcall AdvanceHook(void* self, void* /*edx*/, float dt, unsigned catchUp) {
    if (!g_perf.freq.QuadPart) QueryPerformanceFrequency(&g_perf.freq);
    LARGE_INTEGER t0, t1, t2;
    QueryPerformanceCounter(&t0);
    if (g_perf.last.QuadPart) {
        double gap = Ms(g_perf.last, t0);
        if (gap > 150.0) { ++g_perf.stalls; if (gap > g_perf.worstGap) g_perf.worstGap = gap; }
    } else {
        g_perf.windowStart = t0;
    }
    float r = g_advance(self, dt, catchUp);
    QueryPerformanceCounter(&t1);
    if (!g_readerDisabled) SafeOnMovieAdvance(self);
    QueryPerformanceCounter(&t2);
    double mod = Ms(t1, t2);
    g_perf.gameMs += Ms(t0, t1);
    g_perf.modMs += mod;
    ++g_perf.calls;
    if (mod > g_perf.worstMod) { g_perf.worstMod = mod; g_perf.worstMovie = self; }
    g_perf.last = t2;
    if (Ms(g_perf.windowStart, t2) >= 10000.0) PerfReport(t2);
    return r;
}

}  // namespace

bool GfxInstall() {
    auto base = (BYTE*)GetModuleHandleW(nullptr);
    auto vt = (void**)(base + kVtableRva);
    void* expect[3] = {base + kGetVariableRva, base + kInvokeRva, base + kAdvanceRva};
    int slots[3] = {kSlotGetVariable, kSlotInvoke, kSlotAdvance};
    for (int i = 0; i < 3; ++i) {
        if (vt[slots[i]] != expect[i]) {
            wchar_t m[160];
            swprintf_s(m, L"gfx: vtable slot %d is %p, expected %p. Unknown game build, menu reader off.",
                       slots[i], vt[slots[i]], expect[i]);
            LogLine(m);
            return false;
        }
    }
    g_getVariable = (GetVariableFn)expect[0];
    g_invoke = (InvokeFn)expect[1];
    g_advance = (AdvanceFn)expect[2];
    DWORD old = 0;
    if (!VirtualProtect(&vt[kSlotAdvance], sizeof(void*), PAGE_READWRITE, &old)) {
        LogLine(L"gfx: VirtualProtect failed, menu reader off");
        return false;
    }
    vt[kSlotAdvance] = (void*)&AdvanceHook;
    VirtualProtect(&vt[kSlotAdvance], sizeof(void*), old, &old);
    LogLine(L"gfx: GFxMovieRoot vtable checked, Advance hook installed (slot 30)");
    return true;
}

static bool ValueToText(const GFxValue& v, std::wstring& out) {
    switch (v.type) {
    case GFX_STRING: out = Widen(v.v.s ? v.v.s : ""); return true;
    case GFX_STRINGW: out = v.v.ws ? v.v.ws : L""; return true;
    case GFX_BOOL: out = v.v.b ? L"true" : L"false"; return true;
    case GFX_NUMBER: {
        wchar_t b[64];
        swprintf_s(b, L"%g", v.v.n);
        out = b;
        return true;
    }
    default: return false;  // undefined, null
    }
}

bool GfxGet(void* movie, const char* path, std::wstring& out) {
    GFxValue v;
    if (!g_getVariable(movie, &v, path)) return false;
    return ValueToText(v, out);
}

bool GfxCall(void* movie, const char* method, std::initializer_list<GfxArg> args, std::wstring* result) {
    GFxValue argv[8];
    unsigned n = 0;
    for (const GfxArg& a : args) {
        if (n >= 8) break;
        if (a.isNumber) { argv[n].type = GFX_NUMBER; argv[n].v.n = a.n; }
        else { argv[n].type = GFX_STRING; argv[n].v.s = a.s.c_str(); }
        ++n;
    }
    GFxValue r;
    bool ok = g_invoke(movie, method, &r, n ? argv : nullptr, n);
    if (result) {
        result->clear();
        if (ok) ValueToText(r, *result);
    }
    return ok;
}

bool GfxInvoke(void* movie, const char* method) {
    GFxValue result;
    return g_invoke(movie, method, &result, nullptr, 0);
}
