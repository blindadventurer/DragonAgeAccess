// Exit watcher: closes programs that hang on after the game has exited.
// Started by the mod at game start (core\src\main.cpp StartExitWatcher) for a game
// whose game.h lists kCloseAfterExit; built by build.ps1 as <Mod>Watch.exe.
//
// Why (Dragon Age II on Steam, 2026-11-20): Steam starts the game through the EA app
// (EASteamLauncher creates DragonAge2.exe suspended and EADesktop lets it run). After
// the game exits, EADesktop stays alive and stops answering, so the next start ends in
// "ProxiedLaunchTimeout" 15 s later and the game never runs (EASteamLauncher.log,
// 13:48 and 20:42); closing EASteamLauncher alone was not enough, closing the EA app
// processes was (20:45, the game started).
//
// Command line: <game pid> <delay seconds> <log file> <game exe name> <name>...
// Waits for the game process to end, then the delay, and closes every process with
// one of the names, unless the game was started again meanwhile. Writes what it did
// to the mod's log (the game has closed it by then). No window, no other effect.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

static std::wstring g_log;

static void Log(const std::wstring& text) {
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t stamp[64];
    swprintf_s(stamp, L"%04d-%02d-%02d %02d:%02d:%02d.%03d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
               t.wSecond, t.wMilliseconds);
    std::wstring line = stamp + std::wstring(L"exit watch: ") + text + L"\r\n";
    int n = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), &utf8[0], n, nullptr, nullptr);
    HANDLE h = CreateFileW(g_log.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD put = 0;
    WriteFile(h, utf8.data(), (DWORD)utf8.size(), &put, nullptr);
    CloseHandle(h);
}

// Process ids running an exe of this name (case-insensitive).
static std::vector<DWORD> Find(const std::wstring& name) {
    std::vector<DWORD> ids;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return ids;
    PROCESSENTRY32W e = {sizeof e};
    for (BOOL ok = Process32FirstW(snap, &e); ok; ok = Process32NextW(snap, &e))
        if (_wcsicmp(e.szExeFile, name.c_str()) == 0) ids.push_back(e.th32ProcessID);
    CloseHandle(snap);
    return ids;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv || argc < 6) return 1;
    DWORD pid = wcstoul(argv[1], nullptr, 10), delay = wcstoul(argv[2], nullptr, 10);
    g_log = argv[3];
    std::wstring gameExe = argv[4];
    HANDLE game = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!game) {
        Log(L"cannot watch the game process " + std::to_wstring(pid) + L" (error " + std::to_wstring(GetLastError()) + L")");
        return 1;
    }
    WaitForSingleObject(game, INFINITE);
    CloseHandle(game);
    Log(L"the game has exited; in " + std::to_wstring(delay) + L" s programs left behind are closed");
    Sleep(delay * 1000);
    if (!Find(gameExe).empty()) {
        Log(gameExe + L" was started again meanwhile: nothing closed");
        return 0;
    }
    std::wstring closed, none;
    for (int i = 5; i < argc; ++i) {
        for (DWORD id : Find(argv[i])) {
            HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, id);
            bool ok = h && TerminateProcess(h, 0);
            DWORD err = ok ? 0 : GetLastError();
            if (h) CloseHandle(h);
            closed += std::wstring(L" ") + argv[i] + L"(" + std::to_wstring(id) + (ok ? L")" : L" FAILED " + std::to_wstring(err) + L")");
        }
    }
    Log(closed.empty() ? L"nothing was left running" : L"closed" + closed);
    LocalFree(argv);
    return 0;
}
