// Mod log. Lines are queued and written by a background thread through one open
// handle, so the game thread never waits for the disk. Before 2026-11-18 every
// line opened the file, wrote, FlushFileBuffers and closed it on the game thread;
// timing showed single frames of 50-80 ms in the scanner while the scan itself took
// under 30 ms (player report: heavy lag of the whole game and the scanner).
// Data handed to WriteFile survives a crash of the process (it sits in the
// system's cache), so no flush is needed; LogLineNow writes at once for the crash
// path, where the background thread may never run again.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>
#include "log.h"

namespace {

std::wstring g_logPath;
std::mutex g_mutex;              // guards g_pending and g_file writes
std::condition_variable g_cv;
std::string g_pending;
HANDLE g_file = INVALID_HANDLE_VALUE;
bool g_threadStarted = false;
constexpr ULONGLONG kRotateBytes = 20ull * 1024 * 1024;

std::wstring Stamp() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t stamp[64];
    swprintf_s(stamp, L"%04d-%02d-%02d %02d:%02d:%02d.%03d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
               t.wSecond, t.wMilliseconds);
    return stamp;
}

// Caller holds g_mutex.
void WriteLocked(const std::string& data) {
    if (g_file == INVALID_HANDLE_VALUE)
        g_file = CreateFileW(g_logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                             OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (g_file == INVALID_HANDLE_VALUE || data.empty()) return;
    DWORD written = 0;
    WriteFile(g_file, data.data(), (DWORD)data.size(), &written, nullptr);
}

DWORD WINAPI WriterThread(LPVOID) {
    for (;;) {
        std::string batch;
        {
            std::unique_lock<std::mutex> lock(g_mutex);
            g_cv.wait_for(lock, std::chrono::milliseconds(250), [] { return !g_pending.empty(); });
            batch.swap(g_pending);
            WriteLocked(batch);
        }
    }
}

void Queue(const std::string& data) {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_pending += data;
    }
    g_cv.notify_one();
}

}  // namespace

std::wstring LogPath() { return g_logPath; }

void LogInit(const std::wstring& logPath) {
    g_logPath = logPath;
    // A log over 20 MB is moved aside at start (it reached 9 MB in two days).
    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (GetFileAttributesExW(logPath.c_str(), GetFileExInfoStandard, &fa) &&
        ((ULONGLONG)fa.nFileSizeHigh << 32 | fa.nFileSizeLow) > kRotateBytes) {
        std::wstring old = logPath.substr(0, logPath.rfind(L'.')) + L".old.log";
        MoveFileExW(logPath.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING);
    }
    if (!g_threadStarted) {
        g_threadStarted = true;
        HANDLE t = CreateThread(nullptr, 0, WriterThread, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
}

std::string Utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

std::wstring Widen(const char* utf8) {
    if (!utf8 || !*utf8) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &w[0], n);
    w.resize(n - 1);
    return w;
}

void LogLines(const std::vector<std::wstring>& lines) {
    if (lines.empty()) return;
    std::wstring stamp = Stamp();
    std::string all;
    for (const auto& l : lines) all += Utf8(stamp + l) + "\r\n";
    Queue(all);
}

void LogLine(const std::wstring& text) { Queue(Utf8(Stamp() + text) + "\r\n"); }

void LogLineNow(const std::wstring& text) {
    std::string line = Utf8(Stamp() + text) + "\r\n";
    // The crashing thread may itself hold the lock: wait a little, then write alone.
    for (int i = 0; i < 40; ++i) {
        if (g_mutex.try_lock()) {
            std::string all;
            all.swap(g_pending);  // what was queued comes first
            all += line;
            WriteLocked(all);
            g_mutex.unlock();
            return;
        }
        Sleep(5);
    }
    HANDLE f = CreateFileW(g_logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(f, line.data(), (DWORD)line.size(), &written, nullptr);
    CloseHandle(f);
}
