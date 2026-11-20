// Dragon Age Access installer: one exe for both games (Dragon Age: Origins and
// Dragon Age II). It holds no mod files: it downloads the newest release of the
// chosen game's mod from GitHub (<repo>/releases, asset <Mod>-<version>.zip with
// bin_ship\ and override\ inside), so it never has to be rebuilt for a new release.
// Running it again updates the mod (it compares the installed version) or removes it.
//
// Steps: finds both games and asks which one (or both, or a folder by hand); waits
// while the game runs; downloads and unpacks the release (tar.exe); warns when the
// game exe is not a proven build; replaces a foreign dinput8.dll only with consent;
// copies the files; offers the 4 GB memory flag (Large Address Aware) and freeing
// Page Up / Page Down from the camera; writes bin_ship\scripts\<Mod>\install.txt
// (version, files, loader hash) for later updates and removal.
//
// Only standard Windows dialogs (message boxes and task dialogs with named buttons),
// which screen readers read at once. Names come from games\<game>\game.h
// (games_table.h).
//
// Command line (tests): /find logs the game folders found; /auto answers every
// question with its default and shows no window (needs /gamekey:dao|da2);
// /uninstall removes; /game:<folder> /docs:<Documents folder> replace the found ones;
// /source:<zip or folder> installs from a local release instead of GitHub;
// /repo:<owner/name> reads releases of another repository; /check:<Mod> only finds,
// downloads and unpacks the newest <Mod> release. Log: %TEMP%\DragonAgeAccess-setup.log
// (and bin_ship\scripts\<Mod>.setup.log).
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <imagehlp.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include "games_table.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' " \
                        "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static const wchar_t kSetupVersion[] = L"1.1";
static const wchar_t kDefaultRepo[] = L"blindadventurer/DragonAgeAccess";
// Ultimate ASI Loader 9.7.4 x86 dinput8.dll (NOTEBOOK.md): recognised as ours when an
// older install has no install.txt.
static const char kKnownLoaderSha256[] = "d5a059aa467a7a7127c8f6169f79fa63ff0f55986ee9eb2fd9a281bebf2aa2e6";

static bool g_ru = true, g_auto = false, g_uninstall = false;
static std::wstring g_gameArg, g_docsArg, g_gameKey, g_source, g_repo = kDefaultRepo;
static std::vector<std::wstring> g_log;

static void Log(const std::wstring& s) { g_log.push_back(s); }
static std::wstring T(const wchar_t* ru, const wchar_t* en) { return g_ru ? ru : en; }
static std::wstring Caption() { return T(L"Установка Dragon Age Access", L"Dragon Age Access setup"); }

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

static HRESULT CALLBACK ForegroundCb(HWND h, UINT msg, WPARAM, LPARAM, LONG_PTR) {
    if (msg == TDN_CREATED) SetForegroundWindow(h);
    return S_OK;
}

// A task dialog with named command buttons ("Обновить до 0.3.0", "Удалить мод") and
// Cancel. Returns the chosen id, or IDCANCEL.
struct Choice { int id; std::wstring text; };
static int Choose(const std::wstring& title, const std::wstring& text, const std::vector<Choice>& choices, int autoAnswer) {
    std::wstring s = L"choose: " + title + L" | " + text + L" |";
    for (const Choice& c : choices) s += L" [" + std::to_wstring(c.id) + L"] " + c.text;
    Log(s);
    if (g_auto) { Log(L"answer: " + std::to_wstring(autoAnswer) + L" (auto)"); return autoAnswer; }
    std::vector<TASKDIALOG_BUTTON> buttons;
    for (const Choice& c : choices) buttons.push_back({c.id, c.text.c_str()});
    std::wstring caption = Caption();
    TASKDIALOGCONFIG cfg = {sizeof cfg};
    cfg.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
    cfg.pszWindowTitle = caption.c_str();
    cfg.pszMainInstruction = title.c_str();
    cfg.pszContent = text.empty() ? nullptr : text.c_str();
    cfg.cButtons = (UINT)buttons.size();
    cfg.pButtons = buttons.data();
    cfg.nDefaultButton = choices.empty() ? IDCANCEL : choices[0].id;
    cfg.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    cfg.pfCallback = ForegroundCb;
    int r = IDCANCEL;
    if (FAILED(TaskDialogIndirect(&cfg, &r, nullptr, nullptr))) r = IDCANCEL;
    Log(L"answer: " + std::to_wstring(r));
    return r;
}

// Runs work on a thread while a task dialog with a moving progress bar says what is
// going on (downloads take a few seconds of silence otherwise).
struct BusyWork {
    bool (*fn)(void*) = nullptr;
    void* arg = nullptr;
    bool ok = false, done = false, cancelled = false;
};
static DWORD WINAPI BusyThread(LPVOID p) {
    auto* w = (BusyWork*)p;
    w->ok = w->fn(w->arg);
    return 0;
}
struct BusyCtx { BusyWork* work; HANDLE thread; };
static HRESULT CALLBACK BusyCb(HWND h, UINT msg, WPARAM wp, LPARAM, LONG_PTR ref) {
    auto* c = (BusyCtx*)ref;
    if (msg == TDN_CREATED) {
        SetForegroundWindow(h);
        SendMessageW(h, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 0);
    } else if (msg == TDN_TIMER) {
        if (WaitForSingleObject(c->thread, 0) == WAIT_OBJECT_0) {
            c->work->done = true;
            SendMessageW(h, TDM_CLICK_BUTTON, IDCANCEL, 0);
        }
    } else if (msg == TDN_BUTTON_CLICKED && (int)wp == IDCANCEL && !c->work->done) {
        c->work->cancelled = true;
    }
    return S_OK;
}
static bool RunBusy(const std::wstring& text, bool (*fn)(void*), void* arg) {
    BusyWork w;
    w.fn = fn;
    w.arg = arg;
    if (g_auto) return fn(arg);
    HANDLE th = CreateThread(nullptr, 0, BusyThread, &w, 0, nullptr);
    if (!th) return fn(arg);
    BusyCtx ctx{&w, th};
    std::wstring caption = Caption();
    TASKDIALOGCONFIG cfg = {sizeof cfg};
    cfg.dwFlags = TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CALLBACK_TIMER | TDF_ALLOW_DIALOG_CANCELLATION;
    cfg.pszWindowTitle = caption.c_str();
    cfg.pszMainInstruction = text.c_str();
    cfg.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    cfg.pfCallback = BusyCb;
    cfg.lpCallbackData = (LONG_PTR)&ctx;
    TaskDialogIndirect(&cfg, nullptr, nullptr, nullptr);
    WaitForSingleObject(th, INFINITE);  // a cancelled download still has to end
    CloseHandle(th);
    if (w.cancelled) { Log(L"busy: cancelled by the player"); return false; }
    return w.ok;
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
    if (!s.empty()) CharLowerBuffW(&s[0], (DWORD)s.size());
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

// A folder path this installer may list or delete: absolute ("X:\...") and not a
// drive root. An empty path would mean the current drive's root.
static bool SafeDir(const std::wstring& dir) {
    bool ok = dir.size() > 3 && dir[1] == L':' && (dir[2] == L'\\' || dir[2] == L'/');
    if (!ok) Log(L"refused unsafe folder path \"" + dir + L"\"");
    return ok;
}

static void DeleteTree(const std::wstring& dir) {
    if (!SafeDir(dir)) return;
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

// Relative paths of every file under dir ("scripts\\DAOAccess.asi").
static void ListFiles(const std::wstring& dir, const std::wstring& rel, std::vector<std::wstring>& out) {
    if (!SafeDir(dir)) return;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + (rel.empty() ? L"" : L"\\" + rel) + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        std::wstring n = fd.cFileName;
        if (n == L"." || n == L"..") continue;
        std::wstring r = rel.empty() ? n : rel + L"\\" + n;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ListFiles(dir, r, out);
        else out.push_back(r);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
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

static std::string Sha256(const std::string& data) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    unsigned char hash[32] = {};
    std::string hex;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0) {
        if (BCryptHash(alg, nullptr, 0, (PUCHAR)data.data(), (ULONG)data.size(), hash, 32) == 0) {
            static const char* d = "0123456789abcdef";
            for (unsigned char b : hash) { hex += d[b >> 4]; hex += d[b & 15]; }
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return hex;
}

static std::string FileSha256(const std::wstring& p) {
    std::string data;
    return ReadAll(p, data) ? Sha256(data) : "";
}

static std::wstring Env(const wchar_t* name) {
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetEnvironmentVariableW(name, buf, MAX_PATH * 2);
    return n && n < MAX_PATH * 2 ? buf : L"";
}

// ---------------------------------------------------------------- versions

static std::vector<int> VersionParts(const std::wstring& v) {
    std::vector<int> parts;
    int cur = -1;
    for (wchar_t c : v) {
        if (c >= L'0' && c <= L'9') cur = (cur < 0 ? 0 : cur * 10) + (c - L'0');
        else if (cur >= 0) { parts.push_back(cur); cur = -1; }
    }
    if (cur >= 0) parts.push_back(cur);
    return parts;
}

// <0 when a is older than b, 0 equal, >0 newer.
static int CompareVersions(const std::wstring& a, const std::wstring& b) {
    std::vector<int> x = VersionParts(a), y = VersionParts(b);
    for (size_t i = 0; i < x.size() || i < y.size(); ++i) {
        int p = i < x.size() ? x[i] : 0, q = i < y.size() ? y[i] : 0;
        if (p != q) return p < q ? -1 : 1;
    }
    return 0;
}

// ---------------------------------------------------------------- download

// GET over HTTPS. With location != nullptr redirects are not followed: a 3xx answer
// returns its Location header there (and true).
static bool HttpGet(const std::wstring& url, std::string& body, std::wstring& err, std::wstring* location = nullptr) {
    body.clear();
    wchar_t host[256] = L"", path[2048] = L"", extra[2048] = L"";
    URL_COMPONENTS u = {sizeof u};
    u.lpszHostName = host; u.dwHostNameLength = 256;
    u.lpszUrlPath = path; u.dwUrlPathLength = 2048;
    u.lpszExtraInfo = extra; u.dwExtraInfoLength = 2048;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &u)) { err = L"bad URL " + url; return false; }
    HINTERNET s = WinHttpOpen(L"DragonAgeAccess-Setup", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) s = WinHttpOpen(L"DragonAgeAccess-Setup", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) { err = L"WinHttpOpen " + ErrText(GetLastError()); return false; }
    bool ok = false;
    HINTERNET c = WinHttpConnect(s, host, u.nPort, 0);
    HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", (std::wstring(path) + extra).c_str(), nullptr, WINHTTP_NO_REFERER,
                                         WINHTTP_DEFAULT_ACCEPT_TYPES,
                                         u.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
                    : nullptr;
    if (r && location) {
        DWORD never = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(r, WINHTTP_OPTION_REDIRECT_POLICY, &never, sizeof never);
    }
    if (r) {
        const wchar_t* headers = L"Accept: application/vnd.github+json\r\n";
        if (WinHttpSendRequest(r, headers, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(r, nullptr)) {
            DWORD status = 0, len = sizeof status;
            WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                                &status, &len, WINHTTP_NO_HEADER_INDEX);
            for (;;) {
                DWORD avail = 0;
                if (!WinHttpQueryDataAvailable(r, &avail) || avail == 0) break;
                std::string chunk(avail, '\0');
                DWORD got = 0;
                if (!WinHttpReadData(r, &chunk[0], avail, &got) || got == 0) break;
                body.append(chunk.data(), got);
            }
            ok = status == 200;
            if (location && status >= 300 && status < 400) {
                wchar_t loc[2048] = L"";
                DWORD size = sizeof loc;
                if (WinHttpQueryHeaders(r, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, loc, &size,
                                        WINHTTP_NO_HEADER_INDEX))
                    *location = loc;
                ok = !location->empty();
            }
            if (!ok) err = L"HTTP " + std::to_wstring(status);
        } else {
            err = ErrText(GetLastError());
        }
    } else {
        err = ErrText(GetLastError());
    }
    if (r) WinHttpCloseHandle(r);
    if (c) WinHttpCloseHandle(c);
    WinHttpCloseHandle(s);
    Log(L"http " + url + L" -> " + (ok ? std::to_wstring(body.size()) + L" bytes" : L"FAILED " + err));
    return ok;
}

// A release ready to install: dir holds bin_ship\ and override\.
struct Package { std::wstring dir, version, source; };

static bool Unzip(const std::wstring& zip, const std::wstring& dir, std::wstring& err) {
    DeleteTree(dir);
    MakeDirs(dir);
    std::wstring cmd = L"\"" + Env(L"SystemRoot") + L"\\System32\\tar.exe\" -x -f \"" + zip + L"\" -C \"" + dir + L"\"";
    STARTUPINFOW si = {sizeof si};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        err = L"tar.exe " + ErrText(GetLastError());
        return false;
    }
    WaitForSingleObject(pi.hProcess, 120000);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (code != 0) { err = L"tar.exe exit " + std::to_wstring(code); return false; }
    return true;
}

static std::wstring WorkDir() {
    std::wstring d = Env(L"TEMP") + L"\\DragonAgeAccess-setup";
    MakeDirs(d);
    return d;
}

static bool CheckPackage(const GameInfo& g, const Package& p, std::wstring& err) {
    if (FileExists(p.dir + L"\\bin_ship\\scripts\\" + g.mod + L".asi")) return true;
    err = T(L"в архиве нет файла bin_ship\\scripts\\", L"the archive has no bin_ship\\scripts\\") + g.mod + L".asi";
    return false;
}

// Newest release asset <Mod>-<version>.zip of the repository.
// First the web page github.com/<repo>/releases/latest, which redirects to
// .../releases/tag/v<version> and has no rate limit (the REST API allows 60 requests
// an hour per address, and was used up on the player's PC on 2026-11-20). It needs
// releases published as normal releases (a prerelease is never "latest") and the
// zip named <Mod>-<version>.zip for tag v<version>. Then, as a fallback, the REST
// releases list (newest first, prereleases included).
static bool FindLatest(const GameInfo& g, std::wstring& url, std::wstring& version, std::wstring& err) {
    std::string page;
    std::wstring loc;
    if (HttpGet(L"https://github.com/" + g_repo + L"/releases/latest", page, err, &loc) && !loc.empty()) {
        size_t t = loc.find(L"/releases/tag/");
        if (t != std::wstring::npos) {
            std::wstring tag = loc.substr(t + 14);
            std::wstring v = (!tag.empty() && (tag[0] == L'v' || tag[0] == L'V')) ? tag.substr(1) : tag;
            if (!VersionParts(v).empty()) {
                url = L"https://github.com/" + g_repo + L"/releases/download/" + tag + L"/" + g.mod + L"-" + v + L".zip";
                version = v;
                Log(L"latest " + g.mod + L" (web): " + version + L" " + url);
                return true;
            }
        }
        Log(L"latest redirect not understood: " + loc);
    }
    std::string json;
    if (!HttpGet(L"https://api.github.com/repos/" + g_repo + L"/releases?per_page=30", json, err)) return false;
    const std::string key = "\"browser_download_url\"";
    std::wstring prefix = Lower(g.mod) + L"-";
    for (size_t pos = 0; (pos = json.find(key, pos)) != std::string::npos;) {
        size_t a = json.find('"', json.find(':', pos + key.size()) + 1);
        size_t b = a == std::string::npos ? a : json.find('"', a + 1);
        if (b == std::string::npos) break;
        std::wstring u = Widen(json.substr(a + 1, b - a - 1));
        std::wstring file = Lower(FileName(u));
        pos = b + 1;
        if (file.compare(0, prefix.size(), prefix) != 0 || file.size() < prefix.size() + 5) continue;
        if (file.compare(file.size() - 4, 4, L".zip") != 0) continue;
        std::wstring v = file.substr(prefix.size(), file.size() - prefix.size() - 4);
        if (VersionParts(v).empty() || v.find(L"setup") != std::wstring::npos) continue;
        url = u;
        version = v;
        Log(L"latest " + g.mod + L": " + version + L" " + url);
        return true;
    }
    err = T(L"в разделе Releases нет архива ", L"Releases have no archive ") + g.mod + L"-<версия>.zip";
    return false;
}

struct FetchJob { const GameInfo* game; Package pkg; std::wstring err; };

static bool FetchWork(void* p) {
    auto* job = (FetchJob*)p;
    const GameInfo& g = *job->game;
    std::wstring url, version;
    if (!FindLatest(g, url, version, job->err)) return false;
    std::string data;
    if (!HttpGet(url, data, job->err)) return false;
    std::wstring zip = WorkDir() + L"\\" + FileName(url);
    if (!WriteAll(zip, data.data(), data.size(), job->err)) return false;
    job->pkg.dir = WorkDir() + L"\\" + g.mod + L"-" + version;
    job->pkg.version = version;
    job->pkg.source = url;
    if (!Unzip(zip, job->pkg.dir, job->err)) return false;
    return CheckPackage(g, job->pkg, job->err);
}

// The release to install: /source (a zip or an unpacked folder) or GitHub.
static bool GetPackage(const GameInfo& g, Package& pkg, std::wstring& err) {
    if (!g_source.empty()) {
        pkg.source = g_source;
        std::wstring name = FileName(g_source);
        std::wstring low = Lower(name), prefix = Lower(g.mod) + L"-";
        pkg.version = low.compare(0, prefix.size(), prefix) == 0 ? name.substr(prefix.size()) : L"local";
        if (Lower(pkg.version).size() > 4 && Lower(pkg.version).compare(pkg.version.size() - 4, 4, L".zip") == 0)
            pkg.version.resize(pkg.version.size() - 4);
        if (DirExists(g_source)) pkg.dir = g_source;
        else {
            pkg.dir = WorkDir() + L"\\local-" + g.mod;
            if (!Unzip(g_source, pkg.dir, err)) return false;
        }
        Log(L"package from " + g_source + L" version " + pkg.version);
        return CheckPackage(g, pkg, err);
    }
    FetchJob job;
    job.game = &g;
    bool ok = RunBusy(T(L"Проверяю и скачиваю последнюю версию мода для ", L"Checking and downloading the newest mod for ") + g.title +
                          L"...", FetchWork, &job);
    if (!ok && job.err.empty()) job.err = T(L"отменено", L"cancelled");
    err = job.err;
    pkg = job.pkg;
    return ok;
}

// ---------------------------------------------------------------- game exe

// wrapped: the entry point lies outside the first section, i.e. a DRM stub runs
// first and decrypts the code (Steam's SteamStub ".bind" in Origins, EA's wrapper in
// Dragon Age II, both proven on the Steam copies 2026-11-20). Such an exe must not be
// changed: players report "failed to start (error code 51)" after setting LAA on the
// Steam Origins exe.
struct PeInfo { bool ok = false, wrapped = false; DWORD ts = 0, size = 0; WORD chars = 0; };

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
    auto* sec = IMAGE_FIRST_SECTION(nt);
    if (nt->FileHeader.NumberOfSections > 0 && (const char*)(sec + 1) <= buf + got) {
        DWORD entry = nt->OptionalHeader.AddressOfEntryPoint;
        DWORD span = max(sec->Misc.VirtualSize, sec->SizeOfRawData);
        pe.wrapped = entry < sec->VirtualAddress || entry >= sec->VirtualAddress + span;
    }
    return pe;
}

static std::wstring Hex(DWORD v) {
    wchar_t s[16];
    swprintf_s(s, L"0x%08x", v);
    return s;
}

// Sets the Large Address Aware flag and updates the PE checksum (what 4GB Patch and
// games\da2\tools\set_laa.py do). The original is kept as <exe>.before-laa.
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

// The games zoom the camera with Page Down / Page Up as second bindings
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
        std::string core = data.substr(pos, next - pos), tail;
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

// ---------------------------------------------------------------- finding the games

static bool GameRoot(const GameInfo& g, std::wstring dir, std::wstring& root) {
    while (!dir.empty() && (dir.back() == L'\\' || dir.back() == L'/' || dir.back() == L'"')) dir.pop_back();
    if (!dir.empty() && dir[0] == L'"') dir.erase(0, 1);
    if (dir.empty()) return false;
    if (FileExists(dir + L"\\bin_ship\\" + g.exe)) { root = dir; return true; }
    if (Lower(FileName(dir)) == L"bin_ship" && FileExists(dir + L"\\" + g.exe)) { root = Parent(dir); return true; }
    return false;
}

static void AddCandidate(const GameInfo& g, std::vector<std::wstring>& list, const std::wstring& dir, const wchar_t* source) {
    std::wstring root;
    if (!GameRoot(g, dir, root)) return;
    wchar_t full[MAX_PATH * 2];
    if (GetFullPathNameW(root.c_str(), MAX_PATH * 2, full, nullptr)) root = full;
    for (const std::wstring& r : list)
        if (Lower(r) == Lower(root)) return;
    Log(L"found " + g.key + L" (" + source + L"): " + root);
    list.push_back(root);
}

static void ScanSubdirs(const GameInfo& g, std::vector<std::wstring>& list, const std::wstring& base, const wchar_t* source) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((base + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        std::wstring n = fd.cFileName;
        if (n != L"." && n != L".." && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            AddCandidate(g, list, base + L"\\" + n, source);
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

static std::vector<std::wstring> FindGame(const GameInfo& g) {
    std::vector<std::wstring> list;
    if (!g_gameArg.empty()) { AddCandidate(g, list, g_gameArg, L"command line"); return list; }

    // The installer's own folder and the folders above it.
    wchar_t self[MAX_PATH * 2];
    GetModuleFileNameW(nullptr, self, MAX_PATH * 2);
    std::wstring d = Parent(self);
    for (int i = 0; i < 3 && !d.empty(); ++i, d = Parent(d)) AddCandidate(g, list, d, L"installer folder");

    for (const GameReg& r : g.regs)
        AddCandidate(g, list, RegString(HKEY_LOCAL_MACHINE, r.key, r.value.c_str(), KEY_WOW64_32KEY), L"registry");

    // GOG Galaxy and GOG installers: HKLM\SOFTWARE\GOG.com\Games\<id>, value path.
    for (REGSAM view : {KEY_WOW64_32KEY, KEY_WOW64_64KEY}) {
        HKEY k;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\GOG.com\\Games", 0, KEY_READ | view, &k) != ERROR_SUCCESS) continue;
        wchar_t name[256];
        for (DWORD i = 0;; ++i) {
            DWORD len = 256;
            if (RegEnumKeyExW(k, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
            AddCandidate(g, list, RegString(HKEY_LOCAL_MACHINE, std::wstring(L"SOFTWARE\\GOG.com\\Games\\") + name, L"path", view), L"GOG");
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
        for (const std::wstring& lib : libs) ScanSubdirs(g, list, lib + L"\\steamapps\\common", L"Steam");
    }

    // Usual install folders of the EA app, Origin and GOG (missing folders are skipped).
    std::wstring pf64 = Env(L"ProgramW6432"), pf86 = Env(L"ProgramFiles(x86)");
    if (pf64.empty()) pf64 = Env(L"ProgramFiles");
    if (pf86.empty()) pf86 = Env(L"ProgramFiles");
    ScanSubdirs(g, list, pf64 + L"\\EA Games", L"EA folder");
    ScanSubdirs(g, list, pf86 + L"\\EA Games", L"EA folder");
    ScanSubdirs(g, list, pf86 + L"\\Origin Games", L"Origin folder");
    ScanSubdirs(g, list, pf86 + L"\\GOG Galaxy\\Games", L"GOG folder");
    ScanSubdirs(g, list, L"C:\\GOG Games", L"GOG folder");
    return list;
}

static bool PickFolder(std::wstring& out) {
    IFileOpenDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg)))) return false;
    DWORD opt = 0;
    dlg->GetOptions(&opt);
    dlg->SetOptions(opt | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    std::wstring title = T(L"Выберите папку игры Dragon Age", L"Choose the Dragon Age game folder");
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

static std::wstring RunningProcess(const GameInfo& g) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return L"";
    PROCESSENTRY32W pe = {sizeof pe};
    std::wstring found;
    for (BOOL ok = Process32FirstW(snap, &pe); ok && found.empty(); ok = Process32NextW(snap, &pe))
        for (const std::wstring& n : g.processes)
            if (_wcsicmp(pe.szExeFile, n.c_str()) == 0) found = n;
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

struct Paths { std::wstring game, bin, scripts, modDir, exe, gameDocs, override_, keys, record; };

static Paths MakePaths(const GameInfo& g, const std::wstring& game) {
    Paths p;
    p.game = game;
    p.bin = game + L"\\bin_ship";
    p.scripts = p.bin + L"\\scripts";
    p.modDir = p.scripts + L"\\" + g.mod;
    p.exe = p.bin + L"\\" + g.exe;
    p.gameDocs = DocsFolder() + L"\\" + g.docs;
    p.override_ = p.gameDocs + L"\\packages\\core\\override";
    p.keys = p.gameDocs + L"\\Settings\\KeyBindings.ini";
    p.record = p.modDir + L"\\install.txt";
    return p;
}

// bin_ship\scripts\<Mod>\install.txt: what this installer put there.
struct Record { bool ok = false; std::wstring version; std::string loaderSha; std::vector<std::wstring> files; };

static Record ReadRecord(const Paths& p) {
    Record r;
    std::string data;
    if (!ReadAll(p.record, data)) return r;
    r.ok = true;
    std::wstring text = Widen(data);
    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find(L'\n', pos);
        if (end == std::wstring::npos) end = text.size();
        std::wstring line = text.substr(pos, end - pos);
        pos = end + 1;
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        if (line.compare(0, 8, L"version=") == 0) r.version = line.substr(8);
        else if (line.compare(0, 8, L"dinput8=") == 0) r.loaderSha = Narrow(line.substr(8));
        else if (line.compare(0, 5, L"file=") == 0) r.files.push_back(line.substr(5));
    }
    return r;
}

static std::wstring Dest(const Paths& p, const std::wstring& entry) {  // "bin:x" / "ovr:x"
    return (entry.compare(0, 4, L"ovr:") == 0 ? p.override_ : p.bin) + L"\\" + entry.substr(4);
}

static std::wstring Install(const GameInfo& g, const Paths& p, const Package& pkg, bool& ok) {
    ok = false;
    std::wstring head = g.modTitle + L" " + pkg.version + L", " + g.title + L":\n";
    PeInfo pe = ReadPe(p.exe);
    Log(L"exe " + p.exe + L" ts=" + Hex(pe.ts) + L" size=" + Hex(pe.size) + L" chars=" + Hex(pe.chars) +
        (pe.wrapped ? L" wrapped (DRM stub)" : L""));
    bool proven = false;
    std::wstring names;
    for (const GameBuild& b : g.builds) {
        if (pe.ok && pe.ts == b.ts && pe.size == b.size) proven = true;
        names += (names.empty() ? L"" : L", ") + b.name;
    }
    if (!proven) {
        int r = Ask(g.title + T(L": эта версия игры ещё не проверялась с модом. Мод проверен только на ",
                                L": this game version has not been tested with the mod. The mod is proven only on ") +
                        names + T(L".\n\nНа другой версии мод может молчать или работать с ошибками. Если всё же попробуете, пришлите нам журнал мода: bin_ship\\scripts\\",
                                  L".\n\nOn another version the mod may stay silent or misbehave. If you try anyway, please send us the mod's log: bin_ship\\scripts\\") +
                        g.mod + T(L".log.\n\nВсё равно установить?", L".log.\n\nInstall anyway?"),
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, IDYES);
        if (r != IDYES) return head + T(L"установка отменена, ничего не изменено.", L"cancelled, nothing changed.");
    }

    std::vector<std::wstring> binFiles, ovrFiles;
    ListFiles(pkg.dir + L"\\bin_ship", L"", binFiles);
    ListFiles(pkg.dir + L"\\override", L"", ovrFiles);

    // A foreign dinput8.dll (another mod's loader) is replaced only with consent.
    std::wstring dinput = p.bin + L"\\dinput8.dll", newLoader = pkg.dir + L"\\bin_ship\\dinput8.dll";
    std::string loaderSha = FileSha256(newLoader);
    Record old = ReadRecord(p);
    if (FileExists(dinput)) {
        std::string have = FileSha256(dinput);
        bool ours = have == loaderSha || have == kKnownLoaderSha256 || (old.ok && have == old.loaderSha);
        if (!ours) {
            int r = Ask(T(L"В папке игры уже есть другой файл dinput8.dll. Возможно, он от другого мода. Мод без своего dinput8.dll не загрузится.\n\nЗаменить? Старый файл сохранится как dinput8.dll.before-access и вернётся при удалении мода.",
                          L"The game folder already has another dinput8.dll, maybe from another mod. The mod cannot load without its own dinput8.dll.\n\nReplace it? The old file is kept as dinput8.dll.before-access and comes back when the mod is removed."),
                        MB_YESNO | MB_ICONWARNING, IDYES);
            if (r != IDYES) return head + T(L"установка отменена, ничего не изменено.", L"cancelled, nothing changed.");
            if (!Backup(dinput, dinput + L".before-access"))
                return head + T(L"не удалось сохранить копию dinput8.dll, установка отменена.", L"could not back up dinput8.dll, cancelled.");
        }
    }

    std::wstring failed;
    std::string record = "version=" + Narrow(pkg.version) + "\r\nsetup=" + Narrow(kSetupVersion) +
                         "\r\nsource=" + Narrow(pkg.source) + "\r\ndinput8=" + loaderSha + "\r\n";
    auto copy = [&](const std::wstring& from, const std::wstring& entry) {
        std::wstring dest = Dest(p, entry), err;
        std::string data;
        MakeDirs(Parent(dest));
        if (!ReadAll(from, data)) err = ErrText(GetLastError());
        else WriteAll(dest, data.data(), data.size(), err);
        Log((err.empty() ? L"copied " : L"FAILED " + err + L": ") + dest);
        if (!err.empty()) failed += L"\n" + dest + L": " + err;
        record += "file=" + Narrow(entry) + "\r\n";
    };
    for (const std::wstring& f : binFiles) copy(pkg.dir + L"\\bin_ship\\" + f, L"bin:" + f);
    for (const std::wstring& f : ovrFiles) copy(pkg.dir + L"\\override\\" + f, L"ovr:" + f);
    if (!failed.empty()) return head + T(L"не удалось скопировать файлы мода:", L"could not copy the mod files:") + failed;

    // Read every file back after a moment: an antivirus may quietly remove or block
    // the loader (dinput8.dll) or the mod right after it is written.
    Sleep(g_auto ? 0 : 1500);
    std::wstring missing;
    auto verify = [&](const std::wstring& from, const std::wstring& entry) {
        std::string want = FileSha256(from), have = FileSha256(Dest(p, entry));
        if (want.empty() || have != want) missing += L"\n" + Dest(p, entry);
    };
    for (const std::wstring& f : binFiles) verify(pkg.dir + L"\\bin_ship\\" + f, L"bin:" + f);
    for (const std::wstring& f : ovrFiles) verify(pkg.dir + L"\\override\\" + f, L"ovr:" + f);
    if (!missing.empty()) {
        Log(L"verify FAILED:" + missing);
        return head + T(L"файлы мода пропали или изменились сразу после установки. Скорее всего, их удалил или заблокировал антивирус: загрузчик модов dinput8.dll часто ошибочно считают угрозой. Добавьте папку игры в исключения антивируса (в Защитнике Windows: Защита от вирусов и угроз, Исключения) и запустите установщик ещё раз. Пропали:",
                        L"mod files vanished or changed right after installing. An antivirus most likely removed or blocked them: the mod loader dinput8.dll is often taken for a threat by mistake. Add the game folder to the antivirus exclusions (Windows Security: Virus & threat protection, Exclusions) and run the installer again. Missing:") +
               missing;
    }

    std::wstring err;
    ok = true;
    std::wstring report = head + T(L"- установлен в папку ", L"- installed in ") + p.game;

    // 4 GB memory flag.
    if (pe.ok && (pe.chars & IMAGE_FILE_LARGE_ADDRESS_AWARE)) {
        report += T(L"\n- доступ к 4 ГБ памяти уже был включён", L"\n- 4 GB of memory was already on");
    } else if (pe.ok && pe.wrapped) {
        Log(L"LAA not offered: the exe is wrapped by a DRM stub");
        report += T(L"\n- доступ к 4 ГБ памяти не включён: файл игры защищён магазином (Steam или EA), после изменения игра может не запуститься",
                    L"\n- 4 GB of memory left off: the game file is protected by its store (Steam or EA) and may not start once changed");
    } else if (pe.ok) {
        int r = Ask(g.title + T(L": включить игре доступ к 4 ГБ памяти?\n\nБез этого игра работает у предела в 2 ГБ и может вылетать в больших локациях. Меняется один флаг в файле ",
                                L": give the game access to 4 GB of memory?\n\nWithout it the game runs at its 2 GB limit and may crash in large areas. One flag changes in ") +
                        g.exe + T(L", как делает программа 4GB Patch. Копия исходного файла сохранится рядом как ",
                                  L", as the 4GB Patch tool does. The original is kept next to it as ") + g.exe + L".before-laa.",
                    MB_YESNO | MB_ICONQUESTION, IDYES);
        if (r == IDYES) {
            if (SetLaa(p.exe, err)) report += T(L"\n- доступ к 4 ГБ памяти включён", L"\n- 4 GB of memory is on");
            else {
                Log(L"LAA FAILED: " + err);
                report += T(L"\n- не удалось включить доступ к 4 ГБ памяти: ", L"\n- could not turn on 4 GB of memory: ") + err;
            }
        } else report += T(L"\n- доступ к 4 ГБ памяти не включён", L"\n- 4 GB of memory left off");
    }

    // Page Up / Page Down. Without the settings file (the game never started yet) the
    // answer goes into install.txt as freekeys=1: the mod frees the keys itself, once,
    // as soon as the game has written the file (core\src\main.cpp FreeZoomKeysOnce).
    if (!FileExists(p.keys)) {
        Log(L"no key bindings file: " + p.keys);
        int r = Ask(g.title + T(L": освободить для мода клавиши Page Up и Page Down?\n\nИгра по умолчанию приближает ими камеру, а мод использует их для сканера. Колесо мыши продолжит управлять камерой. Игра ещё ни разу не запускалась, поэтому файла настроек клавиш пока нет: мод сам освободит клавиши, когда игра создаст этот файл, обычно со второго запуска игры.",
                                L": free Page Up and Page Down for the mod?\n\nThe game zooms the camera with them by default, the mod uses them for the scanner. The mouse wheel still zooms. The game has never been started, so its key settings file does not exist yet: the mod frees the keys itself once the game has made that file, usually from the second start of the game."),
                    MB_YESNO | MB_ICONQUESTION, IDYES);
        if (r == IDYES) {
            record += "freekeys=1\r\n";
            report += T(L"\n- клавиши Page Up и Page Down мод освободит сам после первого запуска игры",
                        L"\n- the mod frees Page Up and Page Down itself after the first start of the game");
        } else report += T(L"\n- клавиши Page Up и Page Down оставлены камере", L"\n- Page Up and Page Down left on the camera");
    } else if (ZoomKeys(p.keys, true, false, err) > 0) {
        int r = Ask(g.title + T(L": снять с камеры клавиши Page Up и Page Down?\n\nМод использует их для сканера, а игра по умолчанию приближает ими камеру. Колесо мыши продолжит управлять камерой. Копия файла настроек сохранится как KeyBindings.ini.before-access.",
                                L": free Page Up and Page Down from the camera?\n\nThe mod uses them for the scanner, while the game zooms the camera with them by default. The mouse wheel still zooms. The settings file is backed up as KeyBindings.ini.before-access."),
                    MB_YESNO | MB_ICONQUESTION, IDYES);
        if (r == IDYES) {
            int n = ZoomKeys(p.keys, true, true, err);
            Log(L"zoom keys freed: " + std::to_wstring(n) + L" " + err);
            report += n > 0 ? T(L"\n- клавиши Page Up и Page Down сняты с камеры", L"\n- Page Up and Page Down freed from the camera")
                            : T(L"\n- не удалось изменить настройки клавиш: ", L"\n- could not change the key settings: ") + err;
        } else report += T(L"\n- клавиши Page Up и Page Down оставлены камере", L"\n- Page Up and Page Down left on the camera");
    } else {
        report += T(L"\n- клавиши Page Up и Page Down свободны", L"\n- Page Up and Page Down are free");
    }
    WriteAll(p.record, record.data(), record.size(), err);
    report += T(L"\nПри запуске игры вы услышите «", L"\nAt the game's start you hear \"") + g.start + T(L"».", L"\".");
    return report;
}

static std::wstring Uninstall(const GameInfo& g, const Paths& p) {
    Record rec = ReadRecord(p);
    std::wstring dinput = p.bin + L"\\dinput8.dll";
    std::vector<std::wstring> entries = rec.files;
    if (!rec.ok) {  // installed before install.txt existed (deploy.ps1 or a manual copy)
        entries = {L"bin:dinput8.dll", L"bin:scripts\\" + g.mod + L".asi", L"ovr:" + g.agent};
        Log(L"no install.txt: removing the known files");
    }
    for (const std::wstring& e : entries) {
        std::wstring dest = Dest(p, e);
        if (Lower(dest) == Lower(dinput)) {
            std::string have = FileSha256(dest);
            if (!(have == kKnownLoaderSha256 || (rec.ok && have == rec.loaderSha))) { Log(L"kept foreign " + dest); continue; }
        }
        if (FileExists(dest)) Log((DeleteFileW(dest.c_str()) ? L"deleted " : L"delete FAILED ") + dest);
    }
    if (!FileExists(dinput) && FileExists(dinput + L".before-access"))
        Log((MoveFileW((dinput + L".before-access").c_str(), dinput.c_str()) ? L"restored " : L"restore FAILED ") + dinput);
    DeleteTree(p.modDir);
    // The mod's logs and dumps next to the .asi: scripts\<Mod>.*
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((p.scripts + L"\\" + g.mod + L".*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::wstring f = p.scripts + L"\\" + fd.cFileName;
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) Log((DeleteFileW(f.c_str()) ? L"deleted " : L"delete FAILED ") + f);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryW(p.scripts.c_str());  // only when nothing else is left there

    std::wstring report = g.modTitle + L", " + g.title + T(L": мод удалён", L": the mod is removed");
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
        int r = Ask(g.title + T(L": вернуть игре и исходные настройки?\n", L": also put the game's settings back?\n") +
                        (exeBack ? T(L"\n- Исходный файл игры без доступа к 4 ГБ памяти.", L"\n- The original game exe without 4 GB of memory.") : L"") +
                        (keysBack ? T(L"\n- Клавиши Page Up и Page Down снова управляют камерой.", L"\n- Page Up and Page Down zoom the camera again.") : L""),
                    MB_YESNO | MB_ICONQUESTION, IDYES);
        if (r == IDYES) {
            if (exeBack) {
                bool ok = CopyFileW(backupExe.c_str(), p.exe.c_str(), FALSE) != 0;
                Log((ok ? L"exe restored from " : L"exe restore FAILED from ") + backupExe);
                if (ok) DeleteFileW(backupExe.c_str());
                report += ok ? T(L"\n- исходный файл игры возвращён", L"\n- the original game exe is back")
                             : T(L"\n- не удалось вернуть исходный файл игры", L"\n- could not put the original game exe back");
            }
            if (keysBack) {
                int n = ZoomKeys(p.keys, false, true, err);
                Log(L"zoom keys restored: " + std::to_wstring(n) + L" " + err);
                report += n > 0 ? T(L"\n- клавиши камеры возвращены", L"\n- camera keys are back")
                                : T(L"\n- не удалось вернуть клавиши камеры", L"\n- could not put the camera keys back");
            }
        }
    }
    return report;
}

// One game: folder, running check, version, then install / update / remove.
static std::wstring RunForGame(const GameInfo& g, std::wstring root) {
    if (root.empty()) {
        if (g_auto) return g.title + T(L": игра не найдена.", L": game not found.");
        Tell(g.title + T(L": игра не найдена автоматически.\n\nСейчас откроется окно выбора папки. Выберите папку, в которую установлена игра: в ней есть папка bin_ship.",
                         L": the game was not found automatically.\n\nA folder window opens now. Choose the folder the game is installed in: it holds a bin_ship folder."));
        for (;;) {
            std::wstring pick;
            if (!PickFolder(pick)) return g.title + T(L": отменено.", L": cancelled.");
            Log(L"picked: " + pick);
            if (GameRoot(g, pick, root)) break;
            int r = Ask(T(L"В этой папке нет игры ", L"This folder holds no ") + g.title + L":\n" + pick +
                            T(L"\n\nНужна папка, внутри которой есть bin_ship\\", L"\n\nThe right folder holds bin_ship\\") + g.exe +
                            T(L". Выбрать ещё раз?", L". Choose again?"),
                        MB_RETRYCANCEL | MB_ICONWARNING, IDCANCEL);
            if (r != IDRETRY) return g.title + T(L": отменено.", L": cancelled.");
        }
    }
    for (;;) {
        std::wstring running = RunningProcess(g);
        if (running.empty()) break;
        Log(L"game running: " + running);
        int r = Ask(g.title + T(L" сейчас запущена (", L" is running (") + running +
                        T(L"). Закройте игру и нажмите «Повторить».", L"). Close it and press Retry."),
                    MB_RETRYCANCEL | MB_ICONWARNING, IDCANCEL);
        if (r != IDRETRY) return g.title + T(L": отменено, игра запущена.", L": cancelled, the game is running.");
    }
    Paths p = MakePaths(g, root);
    Log(L"game " + g.key + L" " + p.game + L" | docs " + p.gameDocs);
    bool installed = FileExists(p.scripts + L"\\" + g.mod + L".asi");
    Record rec = ReadRecord(p);
    std::wstring have = rec.ok && !rec.version.empty() ? rec.version : L"";
    if (installed && g_uninstall) return Uninstall(g, p);
    if (!installed && g_uninstall) return g.title + T(L": мод не установлен, удалять нечего.", L": the mod is not installed.");

    Package pkg;
    std::wstring err;
    bool got = GetPackage(g, pkg, err);
    if (!got) Log(L"package FAILED: " + err);
    if (installed) {
        std::wstring title = g.modTitle + T(L" уже установлен", L" is already installed") +
                             (have.empty() ? L"" : T(L", версия ", L", version ") + have) + L".";
        std::wstring text;
        std::vector<Choice> choices;
        enum { kUpdate = 201, kRemove = 202 };
        if (got) {
            int cmp = have.empty() ? -1 : CompareVersions(have, pkg.version);
            if (cmp < 0) {
                text = T(L"Доступна новая версия ", L"A new version is available: ") + pkg.version + L".";
                choices.push_back({kUpdate, T(L"Обновить до версии ", L"Update to version ") + pkg.version});
            } else {
                text = T(L"У вас последняя версия.", L"You have the newest version.");
                choices.push_back({kUpdate, T(L"Переустановить версию ", L"Reinstall version ") + pkg.version});
            }
        } else {
            text = T(L"Не удалось проверить обновления: ", L"Could not check for updates: ") + err;
        }
        choices.push_back({kRemove, T(L"Удалить мод\nУбрать мод из игры и по желанию вернуть исходные настройки",
                                      L"Remove the mod\nTake the mod out of the game and optionally put the settings back")});
        int r = Choose(title, text, choices, got ? kUpdate : IDCANCEL);
        if (r == kRemove) return Uninstall(g, p);
        if (r != kUpdate) return g.title + T(L": без изменений.", L": nothing changed.");
    } else if (!got) {
        return g.title + T(L": не удалось скачать мод: ", L": could not download the mod: ") + err +
               T(L"\nПроверьте подключение к интернету и запустите установщик ещё раз.", L"\nCheck the internet connection and run the installer again.");
    }
    bool ok = false;
    std::wstring report = Install(g, p, pkg, ok);
    if (ok) {
        std::string text;
        for (const std::wstring& l : g_log) text += Narrow(l) + "\r\n";
        WriteAll(p.scripts + L"\\" + g.mod + L".setup.log", text.data(), text.size(), err);
    }
    return report;
}

static void WriteLog() {
    std::string text;
    for (const std::wstring& l : g_log) text += Narrow(l) + "\r\n";
    std::wstring err, temp = Env(L"TEMP");
    if (!temp.empty()) WriteAll(temp + L"\\DragonAgeAccess-setup.log", text.data(), text.size(), err);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    g_ru = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_RUSSIAN;
    bool findOnly = false;
    std::wstring checkMod;
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        if (_wcsicmp(a.c_str(), L"/auto") == 0) g_auto = true;
        else if (_wcsicmp(a.c_str(), L"/uninstall") == 0) g_uninstall = true;
        else if (_wcsicmp(a.c_str(), L"/find") == 0) findOnly = true;
        else if (_wcsnicmp(a.c_str(), L"/game:", 6) == 0) g_gameArg = a.substr(6);
        else if (_wcsnicmp(a.c_str(), L"/docs:", 6) == 0) g_docsArg = a.substr(6);
        else if (_wcsnicmp(a.c_str(), L"/gamekey:", 9) == 0) g_gameKey = Lower(a.substr(9));
        else if (_wcsnicmp(a.c_str(), L"/source:", 8) == 0) g_source = a.substr(8);
        else if (_wcsnicmp(a.c_str(), L"/repo:", 6) == 0) g_repo = a.substr(6);
        else if (_wcsnicmp(a.c_str(), L"/check:", 7) == 0) checkMod = a.substr(7);
    }
    LocalFree(argv);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t stamp[64];
    swprintf_s(stamp, L"%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    Log(std::wstring(L"Dragon Age Access setup ") + kSetupVersion + L" " + stamp + L" repo " + g_repo +
        (g_auto ? L" auto" : L"") + (g_uninstall ? L" uninstall" : L""));

    std::vector<GameInfo> games = AllGames();
    if (!checkMod.empty()) {  // test: find, download and unpack the newest release of a mod
        GameInfo g = games[0];
        g.mod = checkMod;
        FetchJob job;
        job.game = &g;
        bool ok = FetchWork(&job);
        std::vector<std::wstring> files;
        ListFiles(job.pkg.dir, L"", files);
        Log(std::wstring(L"check ") + (ok ? L"ok" : L"FAILED " + job.err) + L", " + std::to_wstring(files.size()) + L" files in " + job.pkg.dir);
        for (const std::wstring& f : files) Log(L"  " + f);
        WriteLog();
        return ok ? 0 : 1;
    }
    std::vector<std::wstring> found(games.size());
    for (size_t i = 0; i < games.size(); ++i) {
        std::vector<std::wstring> list = FindGame(games[i]);
        if (!list.empty()) found[i] = list[0];
    }
    if (findOnly) { WriteLog(); return 0; }

    // Which game: named buttons, one per game, both when both are found, a folder by hand.
    std::vector<size_t> chosen;
    if (!g_gameKey.empty()) {
        for (size_t i = 0; i < games.size(); ++i)
            if (games[i].key == g_gameKey || g_gameKey == L"all") chosen.push_back(i);
    } else if (g_auto) {
        Log(L"auto mode needs /gamekey:dao, da2 or all");
    } else {
        enum { kBoth = 300, kFolder = 301 };
        std::vector<Choice> choices;
        size_t nFound = 0;
        for (size_t i = 0; i < games.size(); ++i) {
            std::wstring note = found[i].empty() ? T(L"Не найдена: откроется окно выбора папки", L"Not found: a folder window opens")
                                                 : T(L"Найдена в папке ", L"Found in ") + found[i];
            choices.push_back({(int)(100 + i), games[i].title + L"\n" + note});
            if (!found[i].empty()) ++nFound;
        }
        if (nFound == games.size()) choices.push_back({kBoth, T(L"Обе игры", L"Both games")});
        choices.push_back({kFolder, T(L"Указать папку игры вручную\nЕсли у вас игра стоит в другой папке",
                                      L"Choose the game folder by hand\nIf your game is in another folder")});
        if (nFound == 1)  // the found game first: Enter picks it
            for (size_t i = 0; i < games.size(); ++i)
                if (!found[i].empty() && i != 0) std::swap(choices[0], choices[i]);
        int r = Choose(T(L"Для какой игры установить или обновить мод?", L"Which game should the mod be installed or updated for?"),
                       T(L"Установщик скачает последнюю версию мода с GitHub. Запустите его ещё раз в любой момент, чтобы обновить или удалить мод.",
                         L"The installer downloads the newest mod from GitHub. Run it again any time to update or remove the mod."),
                       choices, IDCANCEL);
        if (r == kBoth) chosen = {0, 1};
        else if (r >= 100 && r < 100 + (int)games.size()) chosen.push_back((size_t)(r - 100));
        else if (r == kFolder) {
            std::wstring pick, root;
            while (chosen.empty() && PickFolder(pick)) {
                for (size_t i = 0; i < games.size(); ++i)
                    if (GameRoot(games[i], pick, root)) { chosen.push_back(i); found[i] = root; }
                if (chosen.empty() &&
                    Ask(T(L"В этой папке нет ни Dragon Age: Origins, ни Dragon Age II:\n", L"This folder holds neither game:\n") + pick +
                            T(L"\n\nНужна папка, внутри которой есть папка bin_ship. Выбрать ещё раз?",
                              L"\n\nThe right folder holds a bin_ship folder. Choose again?"),
                        MB_RETRYCANCEL | MB_ICONWARNING, IDCANCEL) != IDRETRY)
                    break;
            }
        }
    }
    if (chosen.empty()) {
        Tell(T(L"Установка отменена. Ничего не изменено.", L"Setup cancelled. Nothing was changed."));
        WriteLog();
        CoUninitialize();
        return 1;
    }
    std::wstring summary;
    for (size_t i : chosen) summary += (summary.empty() ? L"" : L"\n\n") + RunForGame(games[i], found[i]);
    Tell(summary + T(L"\n\nЗапустите установщик ещё раз в любой момент, чтобы обновить или удалить мод.",
                     L"\n\nRun this installer again any time to update or remove the mod."));
    WriteLog();
    CoUninitialize();
    return 0;
}
