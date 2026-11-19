// Speech service: the only file that calls Prism. One thread owns the backend;
// Speak() only queues. Every spoken line is logged with its result.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>
#include "prism.h"
#include "log.h"
#include "speech.h"

namespace {

decltype(&prism_init) p_init;
decltype(&prism_registry_create_best) p_create_best;
decltype(&prism_backend_name) p_backend_name;
decltype(&prism_backend_output) p_output;
decltype(&prism_error_string) p_error_string;
decltype(&prism_version_string) p_version_string;

std::wstring g_modDir;
std::mutex g_mutex;
std::condition_variable g_cv;
std::deque<std::pair<std::wstring, bool>> g_queue;

template <class T> bool Resolve(HMODULE m, T& fn, const char* name) {
    fn = reinterpret_cast<T>(GetProcAddress(m, name));
    if (!fn) LogLine(L"speech: missing Prism function " + Widen(name));
    return fn != nullptr;
}

DWORD WINAPI SpeechThread(LPVOID) {
    std::wstring dllPath = g_modDir + L"\\prism.dll";
    HMODULE m = LoadLibraryW(dllPath.c_str());
    if (!m) {
        LogLine(L"speech: cannot load " + dllPath + L" (error " + std::to_wstring(GetLastError()) + L")");
        return 1;
    }
    bool ok = Resolve(m, p_init, "prism_init") & Resolve(m, p_create_best, "prism_registry_create_best") &
              Resolve(m, p_backend_name, "prism_backend_name") & Resolve(m, p_output, "prism_backend_output") &
              Resolve(m, p_error_string, "prism_error_string") & Resolve(m, p_version_string, "prism_version_string");
    if (!ok) return 1;
    LogLine(L"speech: loaded " + dllPath + L", Prism " + Widen(p_version_string()));

    PrismContext* ctx = p_init(nullptr);
    PrismBackend* backend = ctx ? p_create_best(ctx) : nullptr;
    if (!backend) {
        LogLine(L"speech: no Prism backend started, the mod stays silent");
        return 1;
    }
    LogLine(L"speech: backend " + Widen(p_backend_name(backend)));

    for (;;) {
        std::pair<std::wstring, bool> item;
        {
            std::unique_lock<std::mutex> lock(g_mutex);
            g_cv.wait(lock, [] { return !g_queue.empty(); });
            item = std::move(g_queue.front());
            g_queue.pop_front();
        }
        std::string utf8 = Utf8(item.first);
        PrismError r = p_output(backend, utf8.c_str(), item.second);
        LogLine(L"speak [" + std::wstring(item.second ? L"interrupt" : L"queue") + L"] \"" + item.first +
                L"\" -> " + Widen(p_error_string(r)));
    }
}

}  // namespace

void SpeechStart(const std::wstring& modDir) {
    g_modDir = modDir;
    HANDLE t = CreateThread(nullptr, 0, SpeechThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
}

void Speak(const std::wstring& text, bool interrupt) {
    if (text.empty()) return;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (interrupt) g_queue.clear();
        g_queue.emplace_back(text, interrupt);
    }
    g_cv.notify_one();
}
