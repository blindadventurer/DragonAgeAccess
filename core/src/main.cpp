// Dragon Age Access: accessibility mod for Dragon Age: Origins and Dragon Age II
// (Eclipse engine, 32-bit). One build per game (games\<game>\game.h names it).
// Loaded by Ultimate ASI Loader (dinput8.dll proxy) from bin_ship\scripts\<MOD_NAME>.asi.
// DllMain only starts a worker thread; everything else runs there.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <psapi.h>
#include <string>
#include <cstdio>
#include "log.h"
#include "speech.h"
#include "gfx.h"
#include "objects.h"
#include "game.h"

#pragma comment(lib, "version.lib")
#pragma comment(lib, "shell32.lib")

static std::wstring g_modDir;  // bin_ship\scripts\<MOD_NAME> (prism.dll lives here)

static std::wstring ModulePath(HMODULE m) {
    wchar_t p[MAX_PATH] = L"";
    GetModuleFileNameW(m, p, MAX_PATH);
    return p;
}

static std::wstring FileVersion(const std::wstring& path) {
    DWORD dummy = 0, size = GetFileVersionInfoSizeW(path.c_str(), &dummy);
    if (!size) return L"none";
    std::string buf(size, '\0');
    if (!GetFileVersionInfoW(path.c_str(), 0, size, &buf[0])) return L"none";
    VS_FIXEDFILEINFO* fi = nullptr; UINT len = 0;
    if (!VerQueryValueW(&buf[0], L"\\", (void**)&fi, &len) || !fi) return L"none";
    wchar_t v[64];
    swprintf_s(v, L"%u.%u.%u.%u", HIWORD(fi->dwFileVersionMS), LOWORD(fi->dwFileVersionMS),
               HIWORD(fi->dwFileVersionLS), LOWORD(fi->dwFileVersionLS));
    return v;
}

static std::wstring LoadedModuleInfo(const wchar_t* name) {
    HMODULE m = GetModuleHandleW(name);
    if (!m) return std::wstring(name) + L"=not loaded";
    std::wstring p = ModulePath(m);
    return std::wstring(name) + L"=" + p + L" (" + FileVersion(p) + L")";
}

// PE TimeDateStamp and SizeOfImage of the running exe: the build identity.
static std::wstring ExeStamp() {
    auto base = (const BYTE*)GetModuleHandleW(nullptr);
    auto dos = (const IMAGE_DOS_HEADER*)base;
    auto nt = (const IMAGE_NT_HEADERS32*)(base + dos->e_lfanew);
    wchar_t s[64];
    swprintf_s(s, L"ts=0x%08x size=0x%x", nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage);
    return s;
}

static BOOL CALLBACK FindVisibleWindow(HWND h, LPARAM found) {
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid == GetCurrentProcessId() && IsWindowVisible(h)) {
        *(HWND*)found = h;
        return FALSE;
    }
    return TRUE;
}

static HMODULE g_self = nullptr;
static LPTOP_LEVEL_EXCEPTION_FILTER g_prevFilter = nullptr;

// Address space in use, the largest free block (fragmentation) and private bytes.
static std::wstring MemoryLine() {
    MEMORYSTATUSEX ms = {sizeof(ms)};
    GlobalMemoryStatusEx(&ms);
    SIZE_T largest = 0;
    MEMORY_BASIC_INFORMATION mbi;
    for (char* p = nullptr; VirtualQuery(p, &mbi, sizeof(mbi)) == sizeof(mbi);) {
        if (mbi.State == MEM_FREE && mbi.RegionSize > largest) largest = mbi.RegionSize;
        char* next = (char*)mbi.BaseAddress + mbi.RegionSize;
        if (next <= p) break;
        p = next;
    }
    PROCESS_MEMORY_COUNTERS_EX pmc = {};
    pmc.cb = sizeof(pmc);
    K32GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    const ULONGLONG mb = 1024 * 1024;
    return L"memory: used " + std::to_wstring((ms.ullTotalVirtual - ms.ullAvailVirtual) / mb) +
           L" of " + std::to_wstring(ms.ullTotalVirtual / mb) + L" MB address space, largest free " +
           std::to_wstring(largest / mb) + L" MB, private " + std::to_wstring(pmc.PrivateUsage / mb) + L" MB";
}

static std::wstring Hex(ULONG_PTR v) {
    wchar_t b[16];
    swprintf(b, 16, L"%08IX", v);
    return b;
}

// Stack words pointing into the exe or our module (a rough call stack). No C++
// objects here, so __try is allowed.
static int ScanStack(ULONG_PTR esp, const MODULEINFO& exe, const MODULEINFO& mine,
                     ULONG_PTR* out, bool* isMod, int max) {
    int n = 0;
    __try {
        const ULONG_PTR* sp = (const ULONG_PTR*)esp;
        for (int i = 0; i < 2048 && n < max; ++i) {
            ULONG_PTR v = sp[i];
            bool inMe = v >= (ULONG_PTR)mine.lpBaseOfDll && v < (ULONG_PTR)mine.lpBaseOfDll + mine.SizeOfImage;
            bool inExe = v >= (ULONG_PTR)exe.lpBaseOfDll && v < (ULONG_PTR)exe.lpBaseOfDll + exe.SizeOfImage;
            if (inMe || inExe) { out[n] = v; isMod[n] = inMe; ++n; }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return n;
}

// Logs where the game crashed and whether our module is on the stack, then lets
// the previous handler (Windows error reporting) run as before.
static LONG WINAPI CrashFilter(EXCEPTION_POINTERS* ep) {
    static volatile LONG once = 0;
    if (InterlockedExchange(&once, 1) == 0) {
        const EXCEPTION_RECORD* er = ep->ExceptionRecord;
        const CONTEXT* c = ep->ContextRecord;
        std::wstring s = L"CRASH code " + Hex(er->ExceptionCode) + L" at " + Hex((ULONG_PTR)er->ExceptionAddress);
        if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2)
            s += (er->ExceptionInformation[0] ? L" writing " : L" reading ") + Hex(er->ExceptionInformation[1]);
        s += L" | eax " + Hex(c->Eax) + L" ebx " + Hex(c->Ebx) + L" ecx " + Hex(c->Ecx) + L" edx " + Hex(c->Edx) +
             L" esi " + Hex(c->Esi) + L" edi " + Hex(c->Edi) + L" ebp " + Hex(c->Ebp) + L" esp " + Hex(c->Esp);
        LogLineNow(s);
        // Stack words that point into the exe or our module (rough call stack).
        MODULEINFO mine = {}, exe = {};
        K32GetModuleInformation(GetCurrentProcess(), g_self, &mine, sizeof(mine));
        K32GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(nullptr), &exe, sizeof(exe));
        ULONG_PTR words[40];
        bool mod[40];
        int shown = ScanStack(c->Esp, exe, mine, words, mod, 40);
        std::wstring st = L"CRASH stack:";
        for (int i = 0; i < shown; ++i)
            st += mod[i] ? L" MOD+" + Hex(words[i] - (ULONG_PTR)mine.lpBaseOfDll) : L" " + Hex(words[i]);
        LogLineNow(st);
        LogLineNow(MemoryLine());
    }
    return g_prevFilter ? g_prevFilter(ep) : EXCEPTION_CONTINUE_SEARCH;
}

static bool ReadFileBytes(const std::wstring& path, std::string& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(h, nullptr), got = 0;
    bool ok = size != INVALID_FILE_SIZE && size < (1u << 24);
    if (ok) {
        out.resize(size);
        ok = size == 0 || (ReadFile(h, &out[0], size, &got, nullptr) && got == size);
    }
    CloseHandle(h);
    return ok;
}

// The installer asked the player whether to free Page Up / Page Down from the camera
// (the scanner's keys) before the game had ever written its key settings; a yes is
// install.txt "freekeys=1". Here, once the game has made KeyBindings.ini, the two
// default zoom lines (ZoomIn_1=Keyboard::Button_Next, ZoomOut_1=Keyboard::Button_Prior,
// proven in both games) become (UNASSIGNED), with the backup
// KeyBindings.ini.before-access. The backup marks it done: never twice, so a later
// choice of the player in the game's options stays. Runs first thing at start,
// HYPOTHESIS: before the game reads the file; else it counts from the next start.
static void FreeZoomKeysOnce() {
    std::string rec;
    if (!ReadFileBytes(g_modDir + L"\\install.txt", rec) || rec.find("freekeys=1") == std::string::npos) return;
    wchar_t docs[MAX_PATH] = L"";
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docs))) return;
    std::wstring ini = std::wstring(docs) + L"\\" DOCS_DIR L"\\Settings\\KeyBindings.ini";
    std::wstring backup = ini + L".before-access";
    if (GetFileAttributesW(backup.c_str()) != INVALID_FILE_ATTRIBUTES) return;  // done before
    std::string data;
    if (!ReadFileBytes(ini, data)) { LogLine(L"keys: no KeyBindings.ini yet, Page Up / Page Down freed later"); return; }
    const char* from[2] = {"ZoomIn_1=Keyboard::Button_Next", "ZoomOut_1=Keyboard::Button_Prior"};
    const char* to[2] = {"ZoomIn_1=(UNASSIGNED)", "ZoomOut_1=(UNASSIGNED)"};
    std::string out;
    int changed = 0;
    for (size_t pos = 0; pos < data.size();) {
        size_t end = data.find('\n', pos);
        size_t next = end == std::string::npos ? data.size() : end + 1;
        std::string core = data.substr(pos, next - pos), tail;
        while (!core.empty() && (core.back() == '\n' || core.back() == '\r')) { tail.insert(tail.begin(), core.back()); core.pop_back(); }
        for (int i = 0; i < 2; ++i)
            if (core == from[i]) { core = to[i]; ++changed; }
        out += core + tail;
        pos = next;
    }
    if (changed == 0) { LogLine(L"keys: Page Up / Page Down are not on the camera, nothing to free"); return; }
    if (!CopyFileW(ini.c_str(), backup.c_str(), TRUE)) { LogLine(L"keys: backup of KeyBindings.ini FAILED"); return; }
    HANDLE h = CreateFileW(ini.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD put = 0;
    bool ok = h != INVALID_HANDLE_VALUE && WriteFile(h, out.data(), (DWORD)out.size(), &put, nullptr) && put == out.size();
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    LogLine(std::wstring(L"keys: freed Page Up / Page Down from the camera (") + std::to_wstring(changed) +
            L" lines) in " + ini + (ok ? L"" : L" -- write FAILED"));
}

#ifdef CLOSE_AFTER_EXIT_DELAY
// Starts <Mod>Watch.exe (core\watcher), which closes the programs of kCloseAfterExit
// (game.h) CLOSE_AFTER_EXIT_DELAY seconds after this game process ends. It runs
// outside the game, so it breaks away from a job the launcher may have put the game in.
static void StartExitWatcher() {
    std::wstring exe = g_modDir + L"\\" MOD_NAME L"Watch.exe";
    if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES) { LogLine(L"exit watch: " + exe + L" missing"); return; }
    std::wstring names, cmd = L"\"" + exe + L"\" " + std::to_wstring(GetCurrentProcessId()) + L" " +
                              std::to_wstring(CLOSE_AFTER_EXIT_DELAY) + L" \"" + LogPath() + L"\" " GAME_EXE;
    for (const wchar_t* n : kCloseAfterExit) { cmd += std::wstring(L" ") + n; names += std::wstring(L" ") + n; }
    STARTUPINFOW si = {sizeof si};
    PROCESS_INFORMATION pi = {};
    DWORD flags = CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB;
    BOOL ok = CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, flags, nullptr, g_modDir.c_str(), &si, &pi);
    if (!ok) ok = CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, g_modDir.c_str(), &si, &pi);
    if (!ok) { LogLine(L"exit watch: start FAILED, error " + std::to_wstring(GetLastError())); return; }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    LogLine(L"exit watch: started (pid " + std::to_wstring(pi.dwProcessId) + L"), closes" + names + L" " +
            std::to_wstring(CLOSE_AFTER_EXIT_DELAY) + L" s after the game exits");
}
#endif

static DWORD WINAPI Worker(LPVOID) {
    FreeZoomKeysOnce();  // first: the game reads its key settings early
    std::wstring exe = ModulePath(nullptr);
    LogLine(std::wstring(MOD_NAME) + L" probe loaded | bits=" + std::to_wstring(sizeof(void*) * 8) +
            L" | build=" + std::wstring(_CRT_WIDE(__DATE__) L" " _CRT_WIDE(__TIME__)) +
            L" | pid=" + std::to_wstring(GetCurrentProcessId()) +
            L" | exe=" + exe + L" (" + FileVersion(exe) + L", " + ExeStamp() + L")");
    LogLine(L"cmdline: " + std::wstring(GetCommandLineW()));
    LogLine(L"loader: " + LoadedModuleInfo(L"dinput8.dll"));
#ifdef CLOSE_AFTER_EXIT_DELAY
    StartExitWatcher();
#endif

    SpeechStart(g_modDir);
    Speak(START_PHRASE L".", true);
    GfxInstall();  // menu reader: Advance hook on the Flash movies
    ObjectsInstall();  // world objects and the explore/combat mode capture

    // Readiness signal for later stages: the game's first visible window.
    HWND wnd = nullptr;
    for (int i = 0; i < 1200 && !wnd; ++i) {  // up to 2 minutes
        EnumWindows(FindVisibleWindow, (LPARAM)&wnd);
        if (!wnd) Sleep(100);
    }
    if (wnd) {
        wchar_t title[256] = L"";
        GetWindowTextW(wnd, title, 256);
        LogLine(std::wstring(L"window visible: \"") + title + L"\" | " + LoadedModuleInfo(L"d3d9.dll") + L" | " + LoadedModuleInfo(L"d3d11.dll"));
    } else {
        LogLine(L"no visible window after 2 minutes");
    }
    // Crash and memory watch: the game crashed on null pointers in its own code
    // (2026-11-17, Ostagar and save loading); the exe has no Large Address Aware
    // flag, so it has 2 GB of address space. Log usage every 30 s, and the crash
    // itself.
    g_prevFilter = SetUnhandledExceptionFilter(CrashFilter);
    for (;;) {
        LogLine(MemoryLine());
        Sleep(30000);
    }
    return 0;
}

extern "C" __declspec(dllexport) void InitializeASI() {}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        g_self = module;
        std::wstring self = ModulePath(module);           // ...\scripts\<MOD_NAME>.asi
        std::wstring base = self.substr(0, self.rfind(L'.'));
        LogInit(base + L".log");
        g_modDir = base;                                    // ...\scripts\<MOD_NAME>
        HANDLE t = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
