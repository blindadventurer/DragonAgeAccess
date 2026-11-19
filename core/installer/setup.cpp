// Installer of the access mod. One exe with every mod file inside (resources):
// finds the game, copies the files into bin_ship and into the game's override folder,
// offers the 4 GB memory flag (Large Address Aware) for the game exe and frees
// Page Up / Page Down from the camera. Also updates or removes the mod.
// Every question and result is a standard Windows message box, which screen readers
// read at once.
//
// Shared by both games; each build includes games\<game>\game.h (names, proven builds).
//
// Command line (for tests): /find only logs the game folders it finds; /auto answers
// every question with its default, shows no window and never opens the folder
// picker; /uninstall removes the mod;
// /game:<folder> and /docs:<Documents folder> replace the found folders.
// A log is written to %TEMP%\<MOD_NAME>-setup.log (and bin_ship\scripts).
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <imagehlp.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include "game.h"

static bool g_ru = true, g_auto = false, g_uninstall = false;
static std::wstring g_gameArg, g_docsArg;
static std::vector<std::wstring> g_log;

static void Log(const std::wstring& s) { g_log.push_back(s); }
static std::wstring T(const wchar_t* ru, const wchar_t* en) { return g_ru ? ru : en; }
static std::wstring Caption() { return T(L"Установка ", L"Setup: ") + MOD_TITLE; }

static int Ask(const std::wstring& text, UINT type, int autoAnswer) {
    Log(L"ask: " + text);
    int r = autoAnswer;
    if (!g_auto) r = MessageBoxW(nullptr, text.c_str(), Caption().c_str(), type | MB_SETFOREGROUND | MB_TOPMOST);
    Log(L"answer: " + std::to_wstring(r) + (g_auto ? L" (auto)" : L""));
    return r;
}

static void Tell(const std::wstring& text, UINT icon = MB_ICONINFORMATION) {
    Log(L"tell: " + text);
    if (!g_auto) MessageBoxW(nullptr, text.c_str(), Caption().c_str(), MB_OK | icon | MB_SETFOREGROUND | MB_TOPMOST);
}

// ---------------------------------------------------------------- files

static bool FileExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static bool DirExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring Parent(const std::wstring& p) {
    size_t i = p.find_last_of(L"\\/");
    return i == std::wstring::npos ? L"" : p.substr(0, i);
}

static std::wstring FileName(const std::wstring& p) {
    size_t i = p.find_last_of(L"\\/");
    return i == std::wstring::npos ? p : p.substr(i + 1);
}

static std::wstring Lower(std::wstring s) {
    CharLowerBuffW(&s[0], (DWORD)s.size());
    return s;
}

static std::wstring ErrText(DWORD e) {
    wchar_t* buf = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, e, 0, (LPWSTR)&buf, 0, nullptr);
    std::wstring s = buf ? buf : L"";
    if (buf) LocalFree(buf);
    while (!s.empty() && (s.back() == L'\n' || s.back() == L'\r' || s.back() == L' ')) s.pop_back();
    return s + L" (" + std::to_wstring(e) + L")";
}

static bool ReadAll(const std::wstring& p, std::string& out) {
    HANDLE h = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size;
    bool ok = GetFileSizeEx(h, &size) && size.QuadPart < 0x40000000;
    if (ok) {
        out.resize((size_t)size.QuadPart);
        DWORD got = 0;
        ok = out.empty() || (ReadFile(h, &out[0], (DWORD)out.size(), &got, nullptr) && got == out.size());
    }
    CloseHandle(h);
    return ok;
}

// Writes through a temporary file next to the target, then replaces the target.
static bool WriteAll(const std::wstring& p, const void* data, size_t n, std::wstring& err) {
    std::wstring tmp = p + L".setup-tmp";
    HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { err = ErrText(GetLastError()); return false; }
    DWORD put = 0;
    bool ok = n == 0 || (WriteFile(h, data, (DWORD)n, &put, nullptr) && put == n);
    DWORD e = GetLastError();
    CloseHandle(h);
    if (ok) {
        SetFileAttributesW(p.c_str(), FILE_ATTRIBUTE_NORMAL);  // a read-only target cannot be replaced
        ok = MoveFileExW(tmp.c_str(), p.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
        e = GetLastError();
    }
    if (!ok) { DeleteFileW(tmp.c_str()); err = ErrText(e); }
    return ok;
}

static void MakeDirs(const std::wstring& dir) { SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr); }

// Copies a file to a backup name unless that backup already exists (the first backup
// is the original and must not be overwritten by a later run).
static bool Backup(const std::wstring& p, const std::wstring& backup) {
    if (FileExists(backup)) { Log(L"backup kept: " + backup); return true; }
    bool ok = CopyFileW(p.c_str(), backup.c_str(), TRUE) != 0;
    Log((ok ? L"backup made: " : L"backup FAILED: ") + backup);
    return ok;
}

static void DeleteTree(const std::wstring& dir) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::wstring n = fd.cFileName;
            if (n == L"." || n == L"..") continue;
            std::wstring p = dir + L"\\" + n;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) DeleteTree(p);
            else { SetFileAttributesW(p.c_str(), FILE_ATTRIBUTE_NORMAL); DeleteFileW(p.c_str()); }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryW(dir.c_str());
}

// ---------------------------------------------------------------- embedded files

struct Item { int id; std::wstring where; std::wstring rel; };  // where: "bin" or "ovr"

static bool Resource(const wchar_t* name, const char*& data, DWORD& size) {
    HRSRC r = FindResourceW(nullptr, name, MAKEINTRESOURCEW(10));  // RT_RCDATA
    if (!r) return false;
    HGLOBAL g = LoadResource(nullptr, r);
    if (!g) return false;
    data = (const char*)LockResource(g);
    size = SizeofResource(nullptr, r);
    return data != nullptr;
}

static std::wstring Widen(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}

static std::string Narrow(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

// Manifest "FILES": one line per file, "<resource id>\t<bin|ovr>\t<relative path>".
static std::vector<Item> LoadManifest() {
    std::vector<Item> items;
    const char* data; DWORD size;
    if (!Resource(L"FILES", data, size)) return items;
    std::wstring text = Widen(std::string(data, size));
    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find(L'\n', pos);
        if (end == std::wstring::npos) end = text.size();
        std::wstring line = text.substr(pos, end - pos);
        pos = end + 1;
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        size_t t1 = line.find(L'\t'), t2 = line.find(L'\t', t1 + 1);
        if (t1 == std::wstring::npos || t2 == std::wstring::npos) continue;
        items.push_back({_wtoi(line.substr(0, t1).c_str()), line.substr(t1 + 1, t2 - t1 - 1), line.substr(t2 + 1)});
    }
    return items;
}

static bool ItemData(const Item& it, const char*& data, DWORD& size) {
    return Resource(MAKEINTRESOURCEW(it.id), data, size);
}

// ---------------------------------------------------------------- game exe

struct PeInfo { bool ok = false; DWORD ts = 0, size = 0; WORD chars = 0; };

static PeInfo ReadPe(const std::wstring& path) {
    PeInfo pe;
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return pe;
    char buf[4096];
    DWORD got = 0;
    ReadFile(h, buf, sizeof buf, &got, nullptr);
    CloseHandle(h);
    if (got < 0x40 || buf[0] != 'M' || buf[1] != 'Z') return pe;
    DWORD off = *(DWORD*)(buf + 0x3C);
    if (off + sizeof(IMAGE_NT_HEADERS32) > got) return pe;
    auto* nt = (IMAGE_NT_HEADERS32*)(buf + off);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return pe;
    pe.ok = true;
    pe.ts = nt->FileHeader.TimeDateStamp;
    pe.size = nt->OptionalHeader.SizeOfImage;
    pe.chars = nt->FileHeader.Characteristics;
    return pe;
}

static const ProvenBuild* Proven(const PeInfo& pe) {
    for (const ProvenBuild& b : kProvenBuilds)
        if (pe.ok && pe.ts == b.ts && pe.size == b.size) return &b;
    return nullptr;
}

static std::wstring Hex(DWORD v) {
    wchar_t s[16];
    swprintf_s(s, L"0x%08x", v);
    return s;
}

// Sets the Large Address Aware flag and updates the PE checksum (what 4GB Patch and
// tools\set_laa.py do). The original is kept as <exe>.before-laa.
static bool SetLaa(const std::wstring& exe, std::wstring& err) {
    std::string data;
    if (!ReadAll(exe, data)) { err = ErrText(GetLastError()); return false; }
    DWORD off = *(DWORD*)(data.data() + 0x3C);
    auto* nt = (IMAGE_NT_HEADERS32*)(&data[0] + off);
    nt->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
    nt->OptionalHeader.CheckSum = 0;
    DWORD headerSum = 0, sum = 0;
    if (!CheckSumMappedFile(&data[0], (DWORD)data.size(), &headerSum, &sum)) {
        err = L"CheckSumMappedFile " + ErrText(GetLastError());
        return false;
    }
    nt->OptionalHeader.CheckSum = sum;
    if (!Backup(exe, exe + L".before-laa")) { err = T(L"не удалось сделать копию exe", L"could not back up the exe"); return false; }
    if (!WriteAll(exe, data.data(), data.size(), err)) return false;
    Log(L"LAA set, checksum " + Hex(sum));
    return true;
}

// ---------------------------------------------------------------- key bindings

// The game zooms the camera with Page Down / Page Up as second bindings
// (KeyBindings.ini, proven in both games): ZoomIn_1=Keyboard::Button_Next,
// ZoomOut_1=Keyboard::Button_Prior. Freeing them sets (UNASSIGNED); putting them back
// restores those defaults. Only these exact lines are touched.
static int ZoomKeys(const std::wstring& ini, bool free, bool write, std::wstring& err) {
    std::string data;
    if (!ReadAll(ini, data)) { err = ErrText(GetLastError()); return -1; }
    const char* from[2] = {"ZoomIn_1=Keyboard::Button_Next", "ZoomOut_1=Keyboard::Button_Prior"};
    const char* to[2] = {"ZoomIn_1=(UNASSIGNED)", "ZoomOut_1=(UNASSIGNED)"};
    std::string out;
    int changed = 0;
    size_t pos = 0;
    while (pos < data.size()) {
        size_t end = data.find('\n', pos);
        size_t next = end == std::string::npos ? data.size() : end + 1;
        std::string line = data.substr(pos, next - pos);
        std::string core = line, tail;
        while (!core.empty() && (core.back() == '\n' || core.back() == '\r')) { tail.insert(tail.begin(), core.back()); core.pop_back(); }
        for (int i = 0; i < 2; ++i) {
            const char* a = free ? from[i] : to[i];
            const char* b = free ? to[i] : from[i];
            if (core == a) { core = b; ++changed; }
        }
        out += core + tail;
        pos = next;
    }
    if (write && changed > 0) {
        if (free && !Backup(ini, ini + L".before-access")) { err = T(L"не удалось сделать копию файла", L"could not back up the file"); return -1; }
        if (!WriteAll(ini, out.data(), out.size(), err)) return -1;
    }
    return changed;
}

// ---------------------------------------------------------------- finding the game

static bool GameRoot(std::wstring dir, std::wstring& root) {
    while (!dir.empty() && (dir.back() == L'\\' || dir.back() == L'/' || dir.back() == L'"')) dir.pop_back();
    if (!dir.empty() && dir[0] == L'"') dir.erase(0, 1);
    if (dir.empty()) return false;
    if (FileExists(dir + L"\\bin_ship\\" GAME_EXE)) { root = dir; return true; }
    if (Lower(FileName(dir)) == L"bin_ship" && FileExists(dir + L"\\" GAME_EXE)) { root = Parent(dir); return true; }
    return false;
}

static void AddCandidate(std::vector<std::wstring>& list, const std::wstring& dir, const wchar_t* source) {
    std::wstring root;
    if (!GameRoot(dir, root)) return;
    wchar_t full[MAX_PATH * 2];
    if (GetFullPathNameW(root.c_str(), MAX_PATH * 2, full, nullptr)) root = full;
    for (const std::wstring& r : list)
        if (Lower(r) == Lower(root)) return;
    Log(L"found (" + std::wstring(source) + L"): " + root);
    list.push_back(root);
}

static void ScanSubdirs(std::vector<std::wstring>& list, const std::wstring& base, const wchar_t* source) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((base + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        std::wstring n = fd.cFileName;
        if (n != L"." && n != L".." && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            AddCandidate(list, base + L"\\" + n, source);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static std::wstring RegString(HKEY root, const std::wstring& key, const wchar_t* value, REGSAM view) {
    HKEY k;
    if (RegOpenKeyExW(root, key.c_str(), 0, KEY_READ | view, &k) != ERROR_SUCCESS) return L"";
    wchar_t buf[1024];
    DWORD size = sizeof buf, type = 0;
    std::wstring s;
    if (RegQueryValueExW(k, value, nullptr, &type, (BYTE*)buf, &size) == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
        buf[min(size / sizeof(wchar_t), (DWORD)1023)] = 0;
        s = buf;
    }
    RegCloseKey(k);
    return s;
}

static std::wstring Env(const wchar_t* name) {
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetEnvironmentVariableW(name, buf, MAX_PATH * 2);
    return n && n < MAX_PATH * 2 ? buf : L"";
}

static std::vector<std::wstring> FindGames() {
    std::vector<std::wstring> list;
    if (!g_gameArg.empty()) { AddCandidate(list, g_gameArg, L"command line"); return list; }

    // The installer's own folder and the folders above it.
    wchar_t self[MAX_PATH * 2];
    GetModuleFileNameW(nullptr, self, MAX_PATH * 2);
    std::wstring d = Parent(self);
    for (int i = 0; i < 3 && !d.empty(); ++i, d = Parent(d)) AddCandidate(list, d, L"installer folder");

    for (const RegPath& r : kRegPaths)
        AddCandidate(list, RegString(HKEY_LOCAL_MACHINE, r.key, r.value, KEY_WOW64_32KEY), L"registry");

    // GOG Galaxy and GOG installers: HKLM\SOFTWARE\GOG.com\Games\<id>, value path.
    for (REGSAM view : {KEY_WOW64_32KEY, KEY_WOW64_64KEY}) {
        HKEY k;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\GOG.com\\Games", 0, KEY_READ | view, &k) != ERROR_SUCCESS) continue;
        wchar_t name[256];
        for (DWORD i = 0;; ++i) {
            DWORD len = 256;
            if (RegEnumKeyExW(k, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
            AddCandidate(list, RegString(HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\GOG.com\\Games\\") + name, L"path", view), L"GOG");
        }
        RegCloseKey(k);
    }

    // Steam: HKCU\Software\Valve\Steam SteamPath plus the libraries listed in
    // steamapps\libraryfolders.vdf ("path" entries); every folder in steamapps\common.
    std::wstring steam = RegString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", 0);
    for (wchar_t& c : steam) if (c == L'/') c = L'\\';
    if (!steam.empty()) {
        std::vector<std::wstring> libs = {steam};
        std::string vdf;
        if (ReadAll(steam + L"\\steamapps\\libraryfolders.vdf", vdf)) {
            size_t pos = 0;
            while ((pos = vdf.find("\"path\"", pos)) != std::string::npos) {
                size_t a = vdf.find('"', pos + 6);
                size_t b = a == std::string::npos ? a : vdf.find('"', a + 1);
                if (b == std::string::npos) break;
                std::string p = vdf.substr(a + 1, b - a - 1), u;
                for (size_t i = 0; i < p.size(); ++i) {
                    if (p[i] == '\\' && i + 1 < p.size() && p[i + 1] == '\\') ++i;
                    u += p[i];
                }
                libs.push_back(Widen(u));
                pos = b + 1;
            }
        }
        for (const std::wstring& lib : libs) ScanSubdirs(list, lib + L"\\steamapps\\common", L"Steam");
    }

    // Usual install folders of the EA app, Origin and GOG (folders that do not exist
    // are skipped).
    std::wstring pf64 = Env(L"ProgramW6432"), pf86 = Env(L"ProgramFiles(x86)");
    if (pf64.empty()) pf64 = Env(L"ProgramFiles");
    if (pf86.empty()) pf86 = Env(L"ProgramFiles");
    ScanSubdirs(list, pf64 + L"\\EA Games", L"EA folder");
    ScanSubdirs(list, pf86 + L"\\EA Games", L"EA folder");
    ScanSubdirs(list, pf86 + L"\\Origin Games", L"Origin folder");
    ScanSubdirs(list, pf86 + L"\\GOG Galaxy\\Games", L"GOG folder");
    ScanSubdirs(list, L"C:\\GOG Games", L"GOG folder");
    return list;
}

static bool PickFolder(std::wstring& out) {
    IFileOpenDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg)))) return false;
    DWORD opt = 0;
    dlg->GetOptions(&opt);
    dlg->SetOptions(opt | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    std::wstring title = T(L"Выберите папку игры ", L"Choose the folder of ") + GAME_TITLE;
    dlg->SetTitle(title.c_str());
    bool ok = false;
    if (dlg->Show(nullptr) == S_OK) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR p = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &p))) { out = p; CoTaskMemFree(p); ok = true; }
            item->Release();
        }
    }
    dlg->Release();
    return ok;
}

// Returns the game folder, or empty when the player cancelled.
static std::wstring ChooseGame() {
    std::vector<std::wstring> found = FindGames();
    for (const std::wstring& g : found) {
        int r = Ask(T(L"Найдена игра ", L"Found ") + GAME_TITLE + T(L" в папке:\n", L" in the folder:\n") + g +
                        T(L"\n\nДа: установить сюда.\nНет: выбрать другую папку.\nОтмена: выйти.",
                          L"\n\nYes: use this folder.\nNo: choose another folder.\nCancel: quit."),
                    MB_YESNOCANCEL | MB_ICONQUESTION, IDYES);
        if (r == IDYES) return g;
        if (r == IDCANCEL) return L"";
    }
    if (g_auto) { Log(L"no game found (auto mode: no folder picker)"); return L""; }
    std::wstring intro = found.empty()
        ? T(L"Игра не найдена автоматически.\n\n", L"The game was not found automatically.\n\n")
        : L"";
    Tell(intro + T(L"Сейчас откроется окно выбора папки. Выберите папку, в которую установлена игра: в ней есть папка bin_ship.",
                   L"A folder window opens now. Choose the folder the game is installed in: it holds a bin_ship folder."));
    for (;;) {
        std::wstring pick, root;
        if (!PickFolder(pick)) return L"";
        Log(L"picked: " + pick);
        if (GameRoot(pick, root)) return root;
        int r = Ask(T(L"В этой папке нет игры:\n", L"This folder holds no game:\n") + pick +
                        T(L"\n\nНужна папка, внутри которой есть bin_ship\\", L"\n\nThe right folder holds bin_ship\\") + GAME_EXE +
                        T(L". Выбрать ещё раз?", L". Choose again?"),
                    MB_RETRYCANCEL | MB_ICONWARNING, IDCANCEL);
        if (r != IDRETRY) return L"";
    }
}

static std::wstring RunningGame() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return L"";
    PROCESSENTRY32W pe = {sizeof pe};
    std::wstring found;
    for (BOOL ok = Process32FirstW(snap, &pe); ok && found.empty(); ok = Process32NextW(snap, &pe))
        for (const wchar_t* n : kProcesses)
            if (_wcsicmp(pe.szExeFile, n) == 0) found = n;
    CloseHandle(snap);
    return found;
}

static std::wstring DocsFolder() {
    if (!g_docsArg.empty()) return g_docsArg;
    PWSTR p = nullptr;
    std::wstring s;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &p))) s = p;
    CoTaskMemFree(p);
    return s;
}

// ---------------------------------------------------------------- install / remove

struct Paths { std::wstring game, bin, scripts, exe, gameDocs, override_, keys; };

static Paths MakePaths(const std::wstring& game) {
    Paths p;
    p.game = game;
    p.bin = game + L"\\bin_ship";
    p.scripts = p.bin + L"\\scripts";
    p.exe = p.bin + L"\\" GAME_EXE;
    p.gameDocs = DocsFolder() + L"\\" DOCS_DIR;
    p.override_ = p.gameDocs + L"\\packages\\core\\override";
    p.keys = p.gameDocs + L"\\Settings\\KeyBindings.ini";
    return p;
}

static std::wstring Dest(const Paths& p, const Item& it) {
    return (it.where == L"ovr" ? p.override_ : p.bin) + L"\\" + it.rel;
}

static bool SameAsEmbedded(const std::wstring& file, const Item& it) {
    std::string have;
    const char* data; DWORD size;
    return ReadAll(file, have) && ItemData(it, data, size) && have.size() == size && memcmp(have.data(), data, size) == 0;
}

static int Install(const Paths& p) {
    std::vector<Item> items = LoadManifest();
    if (items.empty()) { Tell(T(L"Установщик повреждён: в нём нет файлов мода.", L"The installer is damaged: it holds no mod files."), MB_ICONERROR); return 1; }

    PeInfo pe = ReadPe(p.exe);
    Log(L"exe " + p.exe + L" ts=" + Hex(pe.ts) + L" size=" + Hex(pe.size) + L" chars=" + Hex(pe.chars));
    const ProvenBuild* proven = Proven(pe);
    if (!proven) {
        std::wstring names;
        for (const ProvenBuild& b : kProvenBuilds) names += std::wstring(names.empty() ? L"" : L", ") + b.name;
        int r = Ask(T(L"Эта версия игры ещё не проверялась с модом. Мод проверен только на ", L"This game version has not been tested with the mod. The mod is proven only on ") +
                        names + T(L".\n\nНа другой версии мод может молчать или работать с ошибками. Если всё же попробуете, пришлите нам журнал мода: bin_ship\\scripts\\",
                                  L".\n\nOn another version the mod may stay silent or misbehave. If you try anyway, please send us the mod's log: bin_ship\\scripts\\") +
                        MOD_NAME + T(L".log.\n\nВсё равно установить?", L".log.\n\nInstall anyway?"),
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, IDYES);
        if (r != IDYES) { Tell(T(L"Установка отменена. Ничего не изменено.", L"Setup cancelled. Nothing was changed.")); return 1; }
    }

    // A foreign dinput8.dll (another mod's loader) is replaced only with consent.
    std::wstring dinput = p.bin + L"\\dinput8.dll";
    for (const Item& it : items) {
        if (it.where != L"bin" || Lower(it.rel) != L"dinput8.dll" || !FileExists(dinput) || SameAsEmbedded(dinput, it)) continue;
        int r = Ask(T(L"В папке игры уже есть другой файл dinput8.dll. Возможно, он от другого мода. Мод без своего dinput8.dll не загрузится.\n\nЗаменить? Старый файл сохранится как dinput8.dll.before-access и вернётся при удалении мода.",
                      L"The game folder already has another dinput8.dll, maybe from another mod. The mod cannot load without its own dinput8.dll.\n\nReplace it? The old file is kept as dinput8.dll.before-access and comes back when the mod is removed."),
                    MB_YESNO | MB_ICONWARNING, IDYES);
        if (r != IDYES) { Tell(T(L"Установка отменена. Ничего не изменено.", L"Setup cancelled. Nothing was changed.")); return 1; }
        if (!Backup(dinput, dinput + L".before-access")) {
            Tell(T(L"Не удалось сохранить копию dinput8.dll. Установка отменена.", L"Could not back up dinput8.dll. Setup cancelled."), MB_ICONERROR);
            return 1;
        }
    }

    std::wstring failed;
    for (const Item& it : items) {
        const char* data; DWORD size;
        std::wstring dest = Dest(p, it), err;
        MakeDirs(Parent(dest));
        if (!ItemData(it, data, size)) err = L"resource missing";
        else WriteAll(dest, data, size, err);
        Log((err.empty() ? L"copied " : L"FAILED " + err + L": ") + dest);
        if (!err.empty()) failed += L"\n" + dest + L": " + err;
    }
    if (!failed.empty()) {
        Tell(T(L"Не удалось скопировать файлы мода:", L"Could not copy the mod files:") + failed, MB_ICONERROR);
        return 1;
    }
    std::wstring report = T(L"Мод ", L"") + MOD_TITLE + T(L" установлен в папку игры:\n", L" is installed in the game folder:\n") + p.game +
                          T(L"\n\n- Файл для чтения меню положен в папку override в Документах.",
                            L"\n\n- The menu reader file is in the override folder in Documents.");

    // 4 GB memory flag.
    if (pe.ok && (pe.chars & IMAGE_FILE_LARGE_ADDRESS_AWARE)) {
        report += T(L"\n- Доступ к 4 ГБ памяти у игры уже был включён.", L"\n- The game already had access to 4 GB of memory.");
    } else if (pe.ok) {
        int r = Ask(T(L"Включить игре доступ к 4 ГБ памяти?\n\nБез этого игра работает у предела в 2 ГБ и может вылетать в больших локациях. Меняется один флаг в файле ",
                      L"Give the game access to 4 GB of memory?\n\nWithout it the game runs at its 2 GB limit and may crash in large areas. One flag changes in ") +
                        std::wstring(GAME_EXE) + T(L", как делает программа 4GB Patch. Копия исходного файла сохранится рядом как ",
                                                   L", as the 4GB Patch tool does. The original is kept next to it as ") +
                        GAME_EXE L".before-laa.",
                    MB_YESNO | MB_ICONQUESTION, IDYES);
        if (r == IDYES) {
            std::wstring err;
            if (SetLaa(p.exe, err)) report += T(L"\n- Доступ к 4 ГБ памяти включён.", L"\n- Access to 4 GB of memory is on.");
            else {
                Log(L"LAA FAILED: " + err);
                report += T(L"\n- Не удалось включить доступ к 4 ГБ памяти: ", L"\n- Could not turn on 4 GB of memory: ") + err;
            }
        } else report += T(L"\n- Доступ к 4 ГБ памяти не включён.", L"\n- 4 GB of memory left off.");
    }

    // Page Up / Page Down.
    std::wstring err;
    if (!FileExists(p.keys)) {
        Log(L"no key bindings file: " + p.keys);
        report += T(L"\n- Файла настроек клавиш игры ещё нет, он появится после первого запуска игры. Тогда запустите установщик ещё раз, чтобы освободить клавиши Page Up и Page Down для мода.",
                    L"\n- The game's key settings file does not exist yet; it appears after the first start of the game. Then run this installer again to free Page Up and Page Down for the mod.");
    } else if (ZoomKeys(p.keys, true, false, err) > 0) {
        int r = Ask(T(L"Снять с камеры клавиши Page Up и Page Down?\n\nМод использует их для сканера, а игра по умолчанию приближает ими камеру. Колесо мыши продолжит управлять камерой. Копия файла настроек сохранится как KeyBindings.ini.before-access.",
                      L"Free Page Up and Page Down from the camera?\n\nThe mod uses them for the scanner, while the game zooms the camera with them by default. The mouse wheel still zooms. The settings file is backed up as KeyBindings.ini.before-access."),
                    MB_YESNO | MB_ICONQUESTION, IDYES);
        if (r == IDYES) {
            int n = ZoomKeys(p.keys, true, true, err);
            Log(L"zoom keys freed: " + std::to_wstring(n) + L" " + err);
            report += n > 0 ? T(L"\n- Клавиши Page Up и Page Down сняты с камеры.", L"\n- Page Up and Page Down are freed from the camera.")
                            : T(L"\n- Не удалось изменить настройки клавиш: ", L"\n- Could not change the key settings: ") + err;
        } else report += T(L"\n- Клавиши Page Up и Page Down оставлены камере.", L"\n- Page Up and Page Down left on the camera.");
    } else {
        Log(L"zoom keys: nothing to free " + err);
        report += T(L"\n- Клавиши Page Up и Page Down уже свободны.", L"\n- Page Up and Page Down are already free.");
    }

    report += T(L"\n\nЗапустите игру как обычно. При запуске вы услышите «", L"\n\nStart the game as usual. At start you hear \"") + START_PHRASE +
              T(L"». Приятной игры!", L"\". Have fun!");
    Tell(report);
    return 0;
}

static int Uninstall(const Paths& p) {
    std::vector<Item> items = LoadManifest();
    std::wstring dinput = p.bin + L"\\dinput8.dll";
    for (const Item& it : items) {
        std::wstring dest = Dest(p, it);
        if (it.where == L"bin" && Lower(it.rel) == L"dinput8.dll") {
            if (FileExists(dest) && !SameAsEmbedded(dest, it)) { Log(L"kept foreign " + dest); continue; }
        }
        if (FileExists(dest)) Log((DeleteFileW(dest.c_str()) ? L"deleted " : L"delete FAILED ") + dest);
    }
    if (!FileExists(dinput) && FileExists(dinput + L".before-access"))
        Log((MoveFileW((dinput + L".before-access").c_str(), dinput.c_str()) ? L"restored " : L"restore FAILED ") + dinput);
    DeleteTree(p.scripts + L"\\" MOD_NAME);
    // The mod's logs and dumps next to the .asi: scripts\<MOD_NAME>.*
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((p.scripts + L"\\" MOD_NAME L".*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::wstring f = p.scripts + L"\\" + fd.cFileName;
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) Log((DeleteFileW(f.c_str()) ? L"deleted " : L"delete FAILED ") + f);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryW(p.scripts.c_str());  // only when nothing else is left there

    std::wstring report = T(L"Мод ", L"") + MOD_TITLE + T(L" удалён из папки игры.", L" is removed from the game folder.");
    std::wstring backupExe = p.exe + L".before-laa", err;
    bool exeBack = FileExists(backupExe);
    if (exeBack) {
        // Only a backup of the same build goes back (after a game update it would be older).
        PeInfo now = ReadPe(p.exe), old = ReadPe(backupExe);
        exeBack = now.ok && old.ok && now.ts == old.ts && now.size == old.size;
        if (!exeBack) Log(L"exe backup is another build, kept as it is");
    }
    bool keysBack = FileExists(p.keys) && ZoomKeys(p.keys, false, false, err) > 0;
    if (exeBack || keysBack) {
        int r = Ask(T(L"Вернуть игре и исходные настройки?\n", L"Also put the game's settings back?\n") +
                        (exeBack ? T(L"\n- Исходный файл игры без доступа к 4 ГБ памяти.", L"\n- The original game exe without 4 GB of memory.") : L"") +
                        (keysBack ? T(L"\n- Клавиши Page Up и Page Down снова управляют камерой.", L"\n- Page Up and Page Down zoom the camera again.") : L""),
                    MB_YESNO | MB_ICONQUESTION, IDYES);
        if (r == IDYES) {
            if (exeBack) {
                bool ok = CopyFileW(backupExe.c_str(), p.exe.c_str(), FALSE) != 0;
                Log((ok ? L"exe restored from " : L"exe restore FAILED from ") + backupExe);
                if (ok) DeleteFileW(backupExe.c_str());
                report += ok ? T(L"\n- Исходный файл игры возвращён.", L"\n- The original game exe is back.")
                             : T(L"\n- Не удалось вернуть исходный файл игры.", L"\n- Could not put the original game exe back.");
            }
            if (keysBack) {
                int n = ZoomKeys(p.keys, false, true, err);
                Log(L"zoom keys restored: " + std::to_wstring(n) + L" " + err);
                report += n > 0 ? T(L"\n- Клавиши камеры возвращены.", L"\n- Camera keys are back.")
                                : T(L"\n- Не удалось вернуть клавиши камеры.", L"\n- Could not put the camera keys back.");
            }
        }
    }
    Tell(report);
    return 0;
}

static void WriteLog(const Paths* p) {
    std::string text;
    for (const std::wstring& l : g_log) text += Narrow(l) + "\r\n";
    std::wstring err;
    std::wstring temp = Env(L"TEMP");
    if (!temp.empty()) WriteAll(temp + L"\\" MOD_NAME L"-setup.log", text.data(), text.size(), err);
    if (p && DirExists(p->scripts) && FileExists(p->scripts + L"\\" MOD_NAME L".asi"))
        WriteAll(p->scripts + L"\\" MOD_NAME L".setup.log", text.data(), text.size(), err);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    g_ru = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_RUSSIAN;
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        if (_wcsicmp(a.c_str(), L"/auto") == 0) g_auto = true;
        else if (_wcsicmp(a.c_str(), L"/uninstall") == 0) g_uninstall = true;
        else if (_wcsicmp(a.c_str(), L"/find") == 0) {  // test: only log the found game folders
            FindGames();
            WriteLog(nullptr);
            return 0;
        }
        else if (_wcsnicmp(a.c_str(), L"/game:", 6) == 0) g_gameArg = a.substr(6);
        else if (_wcsnicmp(a.c_str(), L"/docs:", 6) == 0) g_docsArg = a.substr(6);
    }
    LocalFree(argv);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t stamp[64];
    swprintf_s(stamp, L"%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    Log(std::wstring(MOD_TITLE) + L" setup " + stamp + (g_auto ? L" auto" : L"") + (g_uninstall ? L" uninstall" : L""));

    for (;;) {
        std::wstring running = RunningGame();
        if (running.empty()) break;
        Log(L"game running: " + running);
        int r = Ask(T(L"Игра сейчас запущена (", L"The game is running (") + running +
                        T(L"). Закройте её и нажмите «Повторить».", L"). Close it and press Retry."),
                    MB_RETRYCANCEL | MB_ICONWARNING, IDCANCEL);
        if (r != IDRETRY) { WriteLog(nullptr); return 1; }
    }

    std::wstring game = ChooseGame();
    if (game.empty()) {
        Tell(T(L"Установка отменена. Ничего не изменено.", L"Setup cancelled. Nothing was changed."));
        WriteLog(nullptr);
        return 1;
    }
    Paths p = MakePaths(game);
    Log(L"game " + p.game + L" | docs " + p.gameDocs);

    int code;
    bool installed = FileExists(p.scripts + L"\\" MOD_NAME L".asi");
    if (installed) {
        int r = Ask(T(L"Мод ", L"") + MOD_TITLE + T(L" уже установлен.\n\nДа: обновить мод.\nНет: удалить мод.\nОтмена: выйти.",
                                                   L" is already installed.\n\nYes: update the mod.\nNo: remove the mod.\nCancel: quit."),
                    MB_YESNOCANCEL | MB_ICONQUESTION, g_uninstall ? IDNO : IDYES);
        if (r == IDCANCEL) code = 1;
        else code = r == IDNO ? Uninstall(p) : Install(p);
    } else if (g_uninstall) {
        Tell(T(L"Мод не установлен в этой папке, удалять нечего.", L"The mod is not installed in this folder; nothing to remove."));
        code = 1;
    } else {
        code = Install(p);
    }
    WriteLog(&p);
    CoUninitialize();
    return code;
}
