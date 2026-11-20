// Menu reader. Two ways to read a Flash menu, chosen per movie:
//
// 1. Agent mode (preferred): the mod loads its own ActionScript helper
//    (build/daoaccess.gfx, source src/agent/daoaccess.as) into the movie with
//    _root.__daoaccess.loadMovie. DAOAccess.Scan() returns every visible widget
//    (path, kind, label, value, screen x/y, enabled); the mod keeps its own cursor
//    over that list, sorted by screen position. DAOAccess.Press/Adjust act on it.
//    Only one movie is "active" at a time: the popup layer when it shows buttons,
//    else the newest movie that shows any.
//
// 2. FocusManager mode (fallback when the agent cannot load): drives the menus'
//    own console FocusManager, proven on the main menu (tests 2026-11-16 23:37, 23:45):
//    FocusManager.Navigate*, m_CurrentFocus (a clip path), <clip>.SimulateButtonPush,
//    label at <clip>.RollOver_mc.ButtonText_MC.TextField.text; hidden screens keep
//    their items, so a focus counts only when every clip on its path is _visible.
//
// Runs on the game thread inside each movie's Advance (gfx.cpp). A movie pointer is
// used only during its own Advance call; maps key on it but never dereference it.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "game.h"
#include "gfx.h"
#include "log.h"
#include "objects.h"
#include "scanner.h"
#include "speech.h"

#pragma comment(lib, "shell32.lib")

namespace {

// ---------------------------------------------------------------- keys

struct KeyState { bool down = false; DWORD next = 0; };
constexpr DWORD kRepeatDelay = 450, kRepeatRate = 150;  // ms, held arrow

// Edge plus auto-repeat. Call once per frame for each key by its single consumer.
DWORD g_lastKeyAt = 0;  // any key of ours fired: screens are rescanned faster after it

bool KeyFired(KeyState& ks, int vk, bool repeats, bool fg, DWORD now) {
    bool down = fg && (GetAsyncKeyState(vk) & 0x8000) != 0;
    if (down) g_lastKeyAt = now;
    bool fire = false;
    if (down && !ks.down) { fire = true; ks.next = now + kRepeatDelay; }
    else if (down && repeats && (int)(now - ks.next) >= 0) { fire = true; ks.next = now + kRepeatRate; }
    ks.down = down;
    return fire;
}

bool GameIsForeground() {
    HWND fg = GetForegroundWindow();
    DWORD pid = 0;
    if (fg) GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

// ---------------------------------------------------------------- state

struct Item {
    std::wstring path, kind, label, value;
    int x = 0, y = 0;
    bool enabled = true;
    // A row of a scrolling list (OptimizedList): its place in the whole list.
    int listIdx = -1, listTotal = 0;
};

enum AgentState { AGENT_NONE, AGENT_LOADING, AGENT_READY, AGENT_FAILED };

struct MovieState {
    std::wstring file;          // _root.Filename, set by the game
    std::wstring root;          // FocusManager.sm_CurrentRootContainer (empty = none)
    unsigned seq = 0;           // creation order
    DWORD lastCheck = 0, lastSeen = 0;
    unsigned advances = 0;
    KeyState dumpKey, exploreKey;
    // agent mode
    AgentState agent = AGENT_NONE;
    size_t agentAttempt = 0;
    DWORD agentAttemptAt = 0, agentStart = 0, lastScan = 0;
    std::vector<Item> items;
    unsigned scanSerial = 0;    // bumped by every Scan
    size_t prevCount = SIZE_MAX;  // item count of the scan before the last
    DWORD born = 0;             // first Advance
    bool identified = false;    // _root.Filename read (or none after 1 s)
    // FocusManager mode
    std::wstring focus;
    bool focusShown = false;
    DWORD lastShownCheck = 0, allHiddenAt = 0;
    KeyState fmKeys[5];
    std::unordered_set<std::wstring> passive;  // HUD items already announced
};

std::unordered_map<void*, MovieState> g_movies;
unsigned g_seq = 0;
DWORD g_lastSweep = 0;
bool g_firstAdvanceLogged = false;
std::vector<std::string> g_agentUrls;

// Agent-mode cursor, global: one active screen at a time.
void* g_active = nullptr;
std::wstring g_activeFile;
std::wstring g_curPath;     // identity of the current item
size_t g_curIndex = 0;
std::wstring g_curSpoken;   // what was last said for it (detects value changes)
std::wstring g_pendingOpen; // picker just pressed: jump into its list once it shows
DWORD g_pendingOpenAt = 0;
KeyState g_keys[12];

std::wstring Ptr(void* p) {
    wchar_t b[24];
    swprintf_s(b, L"%p", p);
    return b;
}

// ---------------------------------------------------------------- agent loading

std::string Slashes(std::wstring p) {
    std::replace(p.begin(), p.end(), L'\\', L'/');
    return Utf8(p);
}

void InitAgentUrls() {
    if (!g_agentUrls.empty()) return;
    // Our mod folder next to the .asi: bin_ship\scripts\<MOD_NAME>\<AGENT_FILE>
    wchar_t self[MAX_PATH] = L"";
    HMODULE me = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&InitAgentUrls, &me);
    GetModuleFileNameW(me, self, MAX_PATH);
    std::wstring modDir = self;
    modDir = modDir.substr(0, modDir.rfind(L'.'));
    std::string modFile = Slashes(modDir + L"\\" AGENT_FILE);
    // The game's official override folder in Documents.
    wchar_t docs[MAX_PATH] = L"";
    SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docs);
    std::string ovr = Slashes(std::wstring(docs) + L"\\" DOCS_DIR L"\\packages\\core\\override\\" AGENT_FILE);
    std::string bare = Utf8(AGENT_FILE);
    g_agentUrls = {bare, modFile, "file:///" + modFile, ovr, bare.substr(0, bare.rfind('.'))};
}

void StepAgentLoad(void* movie, MovieState& st, DWORD now) {
    if (st.agent == AGENT_READY) return;
    if (st.agent == AGENT_FAILED) return;
    if (st.agent == AGENT_NONE) {
        InitAgentUrls();
        std::wstring r;
        GfxCall(movie, "_root.createEmptyMovieClip", {"__daoaccess", 16001.0}, &r);
        std::wstring clip;
        bool made = GfxGet(movie, "_root.__daoaccess", clip);
        LogLine(L"agent: " + st.file + L" holder clip " + (made ? clip : L"NOT created"));
        if (!made) { st.agent = AGENT_FAILED; return; }
        st.agent = AGENT_LOADING;
        st.agentAttempt = 0;
        st.agentStart = now;
        st.agentAttemptAt = now - 1000;
    }
    // Readiness is polled every frame (the agent is a small local file, usually
    // ready within a few frames); the next URL is tried only after a second.
    std::wstring ver;
    if (st.agentAttempt > 0 && GfxGet(movie, "DAOAccess.ver", ver)) {
        st.agent = AGENT_READY;
        LogLine(L"agent: ready in " + st.file + L" (ver " + ver + L") via " +
                Widen(g_agentUrls[st.agentAttempt - 1].c_str()) + L" after " +
                std::to_wstring(now - st.agentStart) + L" ms");
        return;
    }
    if (now - st.agentAttemptAt < 1000) return;
    if (st.agentAttempt >= g_agentUrls.size()) {
        st.agent = AGENT_FAILED;
        LogLine(L"agent: could not load in " + st.file + L", FocusManager fallback");
        return;
    }
    const std::string& url = g_agentUrls[st.agentAttempt++];
    bool ok = GfxCall(movie, "_root.__daoaccess.loadMovie", {url});
    st.agentAttemptAt = now;
    LogLine(L"agent: " + st.file + L" loadMovie " + Widen(url.c_str()) + (ok ? L"" : L" (invoke failed)"));
}

// ---------------------------------------------------------------- agent scan

std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep) {
    std::vector<std::wstring> out;
    size_t a = 0;
    while (true) {
        size_t b = s.find(sep, a);
        out.push_back(s.substr(a, b == std::wstring::npos ? std::wstring::npos : b - a));
        if (b == std::wstring::npos) break;
        a = b + 1;
    }
    return out;
}

std::wstring GroupOf(const Item& it);
bool SectionMode(const std::wstring& file);

bool BareNumber(const std::wstring& s) {
    return !s.empty() && s.find_first_not_of(L"0123456789.,-+/% ") == std::wstring::npos &&
           s.find_first_of(L"0123456789") != std::wstring::npos;
}

// Equipment slots in body order (the screen lists them in two columns).
int SlotRank(const std::wstring& label) {
    static const wchar_t* const kOrder[] = {L"Оружие", L"Вторая рука", L"Боеприпасы", L"Доспех", L"Шлем",
                                            L"Перчатки", L"Сапоги", L"Пояс", L"Амулет", L"Кольцо 1", L"Кольцо 2"};
    for (int i = 0; i < 11; ++i)
        if (label.compare(0, wcslen(kOrder[i]), kOrder[i]) == 0) return i;
    return 50;
}

// Money fields show gold, silver and copper as numbers between coin pictures
// ("3  29  21"; the sale popup "25  0" = silver and copper, log 17:35-17:36).
// Read as words like the store prices: "3 зол. 29 сер. 21 мед."; zeros left out.
std::wstring MoneyWords(const std::wstring& s) {
    std::vector<long> v;
    std::wstring cur;
    for (wchar_t c : s + L" ") {
        if (c >= L'0' && c <= L'9') { cur += c; continue; }
        if (c != L' ') return s;  // anything else: not a plain money field
        if (!cur.empty()) v.push_back(wcstol(cur.c_str(), nullptr, 10));
        cur.clear();
    }
    if (v.size() < 2 || v.size() > 3) return s;
    static const wchar_t* const kUnits[] = {L" зол.", L" сер.", L" мед."};
    size_t first = 3 - v.size();
    std::wstring out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] == 0) continue;
        if (!out.empty()) out += L" ";
        out += std::to_wstring(v[i]) + kUnits[first + i];
    }
    return out.empty() ? L"0" : out;
}

// After ordering: a bare number joins the label before it ("Сила" + "17" ->
// "Сила, 17"; level-up points "Характеристики, 3"), list screens lose their
// noise (the paperdoll's unlabeled hit area, the experience bar's caption, the
// "hover with the mouse" hint), slots go in body order. Test 16:55 / 17:51.
void Tidy(const std::wstring& file, std::vector<Item>& items) {
    std::vector<Item> out;
    for (Item& it : items) {
        bool money = false;
        if (it.kind == L"text" && it.path.find(L"Money_mc") != std::wstring::npos && BareNumber(it.label)) {
            std::wstring w = MoneyWords(it.label);
            money = w != it.label;
            it.label = w;
        }
        if (it.kind == L"text" && (money || BareNumber(it.label))) {
            if (!out.empty() && out.back().kind == L"text" && GroupOf(out.back()) == GroupOf(it) &&
                std::abs(out.back().y - it.y) <= 30 && !BareNumber(out.back().label)) {
                out.back().label += L", " + it.label;
                continue;
            }
            if (GroupOf(it) == L"Money_mc") it.label = L"Деньги, " + it.label;
        }
        // DA2 start menu noise (player test 2026-11-19 00:18): the advert ticker that
        // changes by itself, the build version, unlocalised placeholders "?(Email:)".
        if (it.kind == L"text" && (it.path.find(L"NewContentText") != std::wstring::npos ||
                                   it.path.find(L"tf_BuildVersion") != std::wstring::npos ||
                                   it.label.compare(0, 2, L"?(") == 0))
            continue;
        if (SectionMode(file)) {
            bool hit = it.label.empty() && it.path.size() >= 10 &&
                       it.path.compare(it.path.size() - 10, 10, L"HitArea_mc") == 0;
            if (hit || it.path.find(L"ExperienceLabel_mc") != std::wstring::npos ||
                it.label.compare(0, 14, L"Наведите мышь ") == 0)
                continue;
        }
        out.push_back(std::move(it));
    }
    // Slots: sort the slot items among themselves, in their own positions.
    std::vector<size_t> pos;
    std::vector<Item> slots;
    for (size_t i = 0; i < out.size(); ++i)
        if (out[i].kind == L"slot") { pos.push_back(i); slots.push_back(out[i]); }
    std::stable_sort(slots.begin(), slots.end(),
                     [](const Item& a, const Item& b) { return SlotRank(a.label) < SlotRank(b.label); });
    for (size_t k = 0; k < pos.size(); ++k) out[pos[k]] = slots[k];
    items.swap(out);
}

bool IsHud(const std::wstring& file);

// The HUD movie the world scanner runs in: Minimap in Origins, CombinedHUD in
// Dragon Age II (it has no Minimap movie; no CombinedHUD was ever seen in Origins).
bool IsScannerHost(const std::wstring& file) { return file == L"Minimap" || file == L"CombinedHUD"; }

// Party picker and the first chargen step (partypicker.gfx): the choices are 3D
// models picked with the mouse. The game highlights the model under the Windows
// cursor (PartyPicker.HighlightedPartyMember follows SetCursorPos: test 2026-11-19
// 00:12, ids 286..308 in chargen) and picks the highlighted one with the bottom
// action (PartyPicker.DoAction 11 -> SelectPartyMemberUnderCursor, or ChargenConfirm
// in confirm mode). The console messages SelectLeft/RightPartyPickerTarget do
// nothing without a current target, and MakeDefaultSelection crashed (test 00:10).
// So the mod sweeps the cursor over the screen once (about 7 s), keeps the centre of
// each highlighted character, and Left/Right put the cursor on the next one.
struct PickCand { unsigned id = 0; double sx = 0, sy = 0; int hits = 0; };
struct PickerState {
    void* movie = nullptr;
    bool running = false, done = false;
    size_t step = 0;
    DWORD lastStep = 0;
    POINT prev{};
    std::vector<PickCand> cands;
    int cur = -1;
} g_pick;

const double kPickRows[] = {0.32, 0.44, 0.56, 0.68};
const size_t kPickCols = 40;
const size_t kPickSamples = sizeof(kPickRows) / sizeof(kPickRows[0]) * kPickCols;

BOOL CALLBACK FindGameWindow(HWND h, LPARAM out) {
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid == GetCurrentProcessId() && IsWindowVisible(h)) { *(HWND*)out = h; return FALSE; }
    return TRUE;
}

// Client-area fraction -> screen point of the game window.
POINT GamePoint(double fx, double fy) {
    HWND w = nullptr;
    EnumWindows(FindGameWindow, (LPARAM)&w);
    RECT rc{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    POINT o{0, 0};
    if (w) { GetClientRect(w, &rc); ClientToScreen(w, &o); }
    POINT p{o.x + (LONG)((rc.right - rc.left) * fx), o.y + (LONG)((rc.bottom - rc.top) * fy)};
    return p;
}

unsigned HighlightedId(void* movie) {
    std::wstring hid;
    GfxCall(movie, "DAOAccess.Val", {"PartyPicker.HighlightedPartyMember"}, &hid);
    unsigned long v = wcstoul(hid.c_str(), nullptr, 10);
    return (v == 0 || v == 0xFFFFFFFFul) ? 0 : (unsigned)v;
}

void PickerPoint(int idx) {
    const PickCand& c = g_pick.cands[idx];
    SetCursorPos((int)c.sx, (int)c.sy);
}

// Runs every frame of the PartyPicker movie.
void PickerTick(void* movie, DWORD now) {
    if (g_pick.movie != movie) {
        g_pick = PickerState{};
        g_pick.movie = movie;
    }
    if (!g_pick.running) return;
    if (now - g_pick.lastStep < 45) return;
    if (g_pick.step > 0) {
        unsigned id = HighlightedId(movie);
        if (id) {
            PickCand* c = nullptr;
            for (auto& x : g_pick.cands) if (x.id == id) c = &x;
            if (!c) { g_pick.cands.push_back(PickCand{}); c = &g_pick.cands.back(); c->id = id; }
            c->sx = (c->sx * c->hits + g_pick.prev.x) / (c->hits + 1);
            c->sy = (c->sy * c->hits + g_pick.prev.y) / (c->hits + 1);
            ++c->hits;
        }
    }
    if (g_pick.step >= kPickSamples) {
        g_pick.running = false;
        g_pick.done = true;
        std::sort(g_pick.cands.begin(), g_pick.cands.end(),
                  [](const PickCand& a, const PickCand& b) { return a.sx < b.sx; });
        std::wstring sum = L"party picker sweep: " + std::to_wstring(g_pick.cands.size()) + L" found;";
        for (auto& c : g_pick.cands)
            sum += L" " + std::to_wstring(c.id) + L"@" + std::to_wstring((int)c.sx) + L"," +
                   std::to_wstring((int)c.sy) + L"x" + std::to_wstring(c.hits);
        LogLine(sum);
        if (g_pick.cands.empty()) {
            Speak(L"Персонажи не найдены", true);
            return;
        }
        g_pick.cur = 0;
        PickerPoint(0);
        Speak(L"Найдено персонажей: " + std::to_wstring(g_pick.cands.size()) +
              L". Стрелки влево и вправо, Enter выбрать", true);
        return;
    }
    size_t row = g_pick.step / kPickCols, col = g_pick.step % kPickCols;
    // Rows alternate direction so the cursor does not jump back across the screen.
    size_t k = (row % 2) ? kPickCols - 1 - col : col;
    double fx = 0.02 + 0.96 * (k + 0.5) / kPickCols;
    g_pick.prev = GamePoint(fx, kPickRows[row]);
    SetCursorPos(g_pick.prev.x, g_pick.prev.y);
    ++g_pick.step;
    g_pick.lastStep = now;
}

void PickerStart(DWORD now) {
    if (g_pick.running) return;
    g_pick.running = true;
    g_pick.done = false;
    g_pick.step = 0;
    g_pick.cands.clear();
    g_pick.cur = -1;
    g_pick.lastStep = now;
    LogLine(L"party picker sweep start");
}

// Dragon Age II: one item "#pick" carries the highlighted character.
void AddPartyChooser(void* movie, std::vector<Item>& items) {
    std::wstring mode, confirm, name;
    GfxCall(movie, "DAOAccess.Val", {"PartyPicker.PartyPickerMode"}, &mode);
    GfxCall(movie, "DAOAccess.Val", {"PartyPicker.ChargenConfirmMode"}, &confirm);
    unsigned hid = HighlightedId(movie);
    bool chargen = mode != L"1";
    Item it;
    it.path = L"#pick";
    it.kind = L"chooser";
    it.label = chargen ? L"Выбор героя" : L"Выбор спутника";
    if (g_pick.running) {
        it.value = L"идёт поиск персонажей";
    } else if (!hid) {
        it.value = g_pick.done ? L"никто не выделен, стрелки влево и вправо" : L"стрелки влево и вправо найдут персонажей";
    } else {
        GfxCall(movie, "DAOAccess.Val", {"PartyPicker.HighlightedPartyMember.Name"}, &name);
        it.value = name.empty() ? L"персонаж " + std::to_wstring(hid) : name;
        for (size_t i = 0; i < g_pick.cands.size(); ++i)
            if (g_pick.cands[i].id == hid)
                it.value += L", " + std::to_wstring(i + 1) + L" из " + std::to_wstring(g_pick.cands.size());
        if (chargen && confirm == L"1") it.value += L", выбран. Enter подтверждает, кнопка Назад отменяет";
    }
    it.y = -100;  // above the Flash widgets
    static std::wstring s_last;
    std::wstring sum = L"party picker: mode " + mode + L" highlighted " + std::to_wstring(hid) + L" confirm " + confirm + L" -> " + it.value;
    if (sum != s_last) LogLine(sum);
    s_last = sum;
    items.insert(items.begin(), it);
}

// Dragon Age: Origins party selection (partypicker.gfx): the Flash screen only holds
// the four chosen portraits, "Удалить все" and "Принять"; the candidates stand in the
// 3D scene and are chosen by clicking their model (player: nothing to read but two
// buttons, 2026-11-18). The candidates come from the game mode's own list
// (objects.cpp PartyCandidates) as items "#pp:<id>", first on the screen; Enter adds
// or removes.
void AddPartyList(void* movie, std::vector<Item>& items) {
    std::vector<PartyCandidate> cands;
    int count = 0;
    if (!PartyCandidates(cands, count) || cands.empty()) return;
    static std::unordered_map<unsigned, std::wstring> s_names;
    std::vector<Item> add;
    for (const PartyCandidate& c : cands) {
        std::wstring& name = s_names[c.id];
        if (name.empty()) {
            std::wstring n, lvl;
            GfxCall(movie, "DAOAccess.Val", {"Objects." + std::to_string(c.id) + ".Name"}, &n);
            GfxCall(movie, "DAOAccess.Val", {"Objects." + std::to_string(c.id) + ".Attributes.15.Current"}, &lvl);
            name = n.empty() ? c.name : n;
            if (name.empty()) name = L"спутник " + std::to_wstring(c.id);
            if (!lvl.empty() && lvl != L"0") name += L", уровень " + lvl.substr(0, lvl.find(L'.'));
        }
        Item it;
        it.path = L"#pp:" + std::to_wstring(c.id);
        it.kind = L"member";
        it.label = name;
        it.value = (c.state == 1 ? L"in" : c.state == 2 ? L"out" : L"busy") + std::wstring(c.locked ? L"|locked" : L"");
        it.y = -100;  // above the Flash widgets
        add.push_back(it);
    }
    add.insert(add.end(), items.begin(), items.end());
    items.swap(add);
    static std::wstring s_last;
    std::wstring sum = L"party picker: " + std::to_wstring(count) + L" chosen;";
    for (const PartyCandidate& c : cands)
        sum += L" " + std::to_wstring(c.id) + L"=" + std::to_wstring(c.state) + (c.locked ? L"L" : L"");
    if (sum != s_last) LogLine(sum);
    s_last = sum;
}

// The party screen of each game (game.h: PARTY_PICKER_LIST for Origins, whose game
// mode gives the candidate list; Dragon Age II picks with the cursor).
void AddPartyCandidates(void* movie, std::vector<Item>& items) {
#ifdef PARTY_PICKER_LIST
    AddPartyList(movie, items);
#else
    AddPartyChooser(movie, items);
#endif
}

void Scan(void* movie, MovieState& st, DWORD now) {
    st.lastScan = now;
    std::wstring txt;
    // HUD movies are never read as screens (PickActive skips them); only the
    // notifications and the pause sign are spoken. Their full scans every 400 ms
    // (FloatyLayer holds a clip per creature) were suspected in the lag report of
    // 2026-11-18.
    bool ok;
    if (st.file == L"FloatyLayer") {
        // The game's pause state first (agent PauseLine); the sign's clip only when
        // that state cannot be read.
        ok = GfxCall(movie, "DAOAccess.PauseLine", {}, &txt);
        if (!ok || txt == L"?") {
            txt.clear();
            ok = GfxCall(movie, "DAOAccess.ScanUnder", {"_level0.PauseIndicator_mc"}, &txt);
        }
    } else if (IsHud(st.file) && st.file != L"Notifications") {
        st.items.clear();
        return;
    } else {
        ok = GfxCall(movie, "DAOAccess.Scan", {}, &txt);
    }
    if (!ok) {
        LogLine(L"agent: Scan failed in " + st.file);
        st.items.clear();
        return;
    }
    std::vector<Item> items;
    for (const std::wstring& line : Split(txt, L'\n')) {
        if (line.empty()) continue;
        auto f = Split(line, L'\t');
        if (f.size() < 7) continue;
        Item it;
        it.path = f[0]; it.kind = f[1]; it.label = f[2]; it.value = f[3];
        it.x = _wtoi(f[4].c_str()); it.y = _wtoi(f[5].c_str());
        it.enabled = f[6] != L"0";
        if (f.size() > 7 && !f[7].empty()) {
            size_t sl = f[7].find(L'/');
            if (sl != std::wstring::npos) {
                it.listIdx = _wtoi(f[7].substr(0, sl).c_str());
                it.listTotal = _wtoi(f[7].substr(sl + 1).c_str());
            }
        }
        items.push_back(it);
    }
    // Drop repeated paths (the agent dedupes too).
    {
        std::unordered_set<std::wstring> seen;
        std::vector<Item> uniq;
        for (Item& i : items)
            if (seen.insert(i.path).second) uniq.push_back(std::move(i));
        items.swap(uniq);
    }
    // Reading order follows the clip tree: the widgets of one container (a column,
    // a list, an open picker) stay together. At each level the containers are
    // ordered by their top-left item, rows top to bottom (10 px tolerance), then
    // left to right; an item that owns others (a picker) comes before them.
    std::vector<std::vector<std::wstring>> segs(items.size());
    for (size_t i = 0; i < items.size(); ++i) segs[i] = Split(items[i].path, L'.');
    auto byRow = [&](size_t a, size_t b) {
        if (std::abs(items[a].y - items[b].y) > 10) return items[a].y < items[b].y;
        return items[a].x < items[b].x;
    };
    std::vector<Item> ordered;
    std::function<void(std::vector<size_t>, size_t)> order = [&](std::vector<size_t> idx, size_t depth) {
        std::vector<size_t> self;
        std::vector<std::pair<std::wstring, std::vector<size_t>>> groups;
        for (size_t i : idx) {
            if (segs[i].size() <= depth) { self.push_back(i); continue; }
            auto g = std::find_if(groups.begin(), groups.end(),
                                  [&](const auto& x) { return x.first == segs[i][depth]; });
            if (g == groups.end()) groups.push_back({segs[i][depth], {i}});
            else g->second.push_back(i);
        }
        std::stable_sort(self.begin(), self.end(), byRow);
        for (size_t i : self) ordered.push_back(items[i]);
        auto anchor = [&](const std::vector<size_t>& v) {
            return *std::min_element(v.begin(), v.end(), byRow);
        };
        std::stable_sort(groups.begin(), groups.end(),
                         [&](const auto& x, const auto& y) { return byRow(anchor(x.second), anchor(y.second)); });
        for (auto& g : groups) order(g.second, depth + 1);
    };
    std::vector<size_t> all(items.size());
    for (size_t i = 0; i < all.size(); ++i) all[i] = i;
    order(all, 0);
    Tidy(st.file, ordered);
    if (st.file == L"PartyPicker") AddPartyCandidates(movie, ordered);
    st.prevCount = st.items.size();
    st.items.swap(ordered);
    ++st.scanSerial;
}

// Index of the item with this path, or SIZE_MAX.
size_t FindPath(const MovieState& st, const std::wstring& path) {
    for (size_t i = 0; i < st.items.size(); ++i)
        if (st.items[i].path == path) return i;
    return SIZE_MAX;
}

// The nearest item whose path is a prefix of this one (its owner on screen).
size_t FindOwner(const MovieState& st, std::wstring path) {
    while (true) {
        size_t dot = path.rfind(L'.');
        if (dot == std::wstring::npos) return SIZE_MAX;
        path.resize(dot);
        size_t i = FindPath(st, path);
        if (i != SIZE_MAX) return i;
    }
}

std::wstring Describe(const MovieState& st, size_t idx) {
    const Item& it = st.items[idx];
    std::wstring s = it.label.empty() ? L"без подписи" : it.label;
    if (it.kind == L"toggle") s += it.value == L"1" ? L", флажок, отмечен" : L", флажок, не отмечен";
    else if (it.kind == L"group") s += L", " + it.value;  // codex group: found, open or closed
    else if (it.kind == L"tab") s += it.value == L"1" ? L", вкладка, выбрана" : L", вкладка";
    else if (it.kind == L"radio") s += it.value == L"1" ? L", выбрано" : L"";
    else if (it.kind == L"slider") s += L", ползунок, " + it.value;
    else if (it.kind == L"spin") s += L", " + it.value;
    else if (it.kind == L"item") s += it.value.empty() ? L"" : L", " + it.value;
    else if (it.kind == L"button" && !it.value.empty()) s += L", " + it.value;  // world map pin state
    else if (it.kind == L"slot") s += L": " + (it.value.empty() ? std::wstring(L"пусто") : it.value);
    else if (it.kind == L"ability")
        s += it.value == L"have" ? L", изучено" : it.value == L"locked" ? L", требования не выполнены" : L", можно изучить";
    else if (it.kind == L"spec")
        s += L", специализация" +
             std::wstring(it.value == L"have"     ? L", выбрана"
                          : it.value == L"avail"  ? L", можно выбрать"
                          : it.value == L"closed" ? L", не открыта, её открывает наставник или книга"
                                                  : L", недоступна");
    else if (it.kind == L"picker") {
        // A picker without its own label finds the chosen entry's text as label
        // ("Средне, список, Средне", difficulty dialog 2026-11-19 21:36).
        if (it.label.empty() || it.value.compare(0, it.label.size(), it.label) == 0)
            s = L"список, " + it.value;
        else
            s += L", список, " + it.value;
    }
    else if (it.kind == L"chooser") s += L": " + it.value;
    else if (it.kind == L"member") {
        s += it.value.compare(0, 2, L"in") == 0 ? L", в отряде" : it.value.compare(0, 3, L"out") == 0 ? L", не в отряде" : L"";
        if (it.value.find(L"locked") != std::wstring::npos) s += L", закреплён сюжетом";
        size_t n = 0, pos = 0;
        for (size_t i = 0; i < st.items.size(); ++i)
            if (st.items[i].kind == L"member") { ++n; if (i == idx) pos = n; }
        s += L", " + std::to_wstring(pos) + L" из " + std::to_wstring(n);
    }
    else if (it.kind == L"edit") s = L"поле ввода, " + (it.label.empty() ? std::wstring(L"пусто") : it.label);
    else if (it.kind == L"password")
        s = L"поле пароля, " + (it.value.empty() || it.value == L"0" ? std::wstring(L"пусто") : it.value + L" симв.");
    else if (it.kind == L"text") {
        s = it.label;
        // A bare number ("3") gets the text before it ("Характеристики, 3"):
        // points left on the level-up screen (test 17:51), stats on the record.
        if (!s.empty() && s.find_first_not_of(L"0123456789.,-+/% ") == std::wstring::npos && idx > 0 &&
            st.items[idx - 1].kind == L"text" && !st.items[idx - 1].label.empty())
            s = st.items[idx - 1].label + L", " + s;
    }
    else if (it.kind == L"entry") {
        // "N из M" among the entries of the same owner.
        size_t own = FindOwner(st, it.path), n = 0, pos = 0;
        for (size_t i = 0; i < st.items.size(); ++i) {
            if (st.items[i].kind != L"entry" || FindOwner(st, st.items[i].path) != own) continue;
            ++n;
            if (i == idx) pos = n;
        }
        if (it.value == L"1") s += L", выбрано";
        s += L", " + std::to_wstring(pos) + L" из " + std::to_wstring(n);
    }
    if (!it.enabled) s += L", недоступно";
    return s;
}

std::wstring PositionSuffix(const MovieState& st, size_t idx);

void SpeakItem(const MovieState& st, size_t idx, const wchar_t* why, bool interrupt = true) {
    const Item& it = st.items[idx];
    g_curIndex = idx;
    g_curPath = it.path;
    g_curSpoken = Describe(st, idx);
    std::wstring say = g_curSpoken + PositionSuffix(st, idx);
    // What is in an input field (an e-mail address typed for the EA sign-in) is
    // spoken but never logged; a password field says only its length.
    bool secret = it.kind == L"edit" && !it.label.empty();
    LogLine(std::wstring(L"item ") + why + L" " + std::to_wstring(idx + 1) + L"/" +
            std::to_wstring(st.items.size()) + L" " + st.file + L" " + it.kind + L" " +
            (secret ? std::wstring(L"<скрыто>") : L"\"" + say + L"\"") + L" y=" + std::to_wstring(it.y) +
            L" x=" + std::to_wstring(it.x) + L" path=" + it.path);
    if (secret) SpeakPrivate(say, interrupt);
    else Speak(say, interrupt);
}

bool IsWidget(const Item& it) { return it.kind != L"text"; }

// Texts watched after a press: what changed on screen is read after the item.
std::unordered_map<std::wstring, std::wstring> g_watchTexts;
DWORD g_watchUntil = 0;

void SnapshotTexts(const MovieState& st, DWORD now) {
    g_watchTexts.clear();
    for (const Item& it : st.items)
        if (it.kind == L"text") g_watchTexts[it.path] = it.label;
    g_watchUntil = now + 1500;
}

void DumpItems(const MovieState& st) {
    std::vector<std::wstring> lines;
    lines.push_back(L"dump " + st.file + L" agent=" + std::to_wstring(st.agent) + L" items=" +
                    std::to_wstring(st.items.size()));
    for (size_t i = 0; i < st.items.size(); ++i) {
        const Item& it = st.items[i];
        lines.push_back(L"  " + std::to_wstring(i + 1) + L" " + it.kind + L" \"" + it.label + L"\" value=\"" +
                        it.value + L"\" y=" + std::to_wstring(it.y) + L" x=" + std::to_wstring(it.x) +
                        (it.enabled ? L"" : L" disabled") + L" " + it.path);
    }
    LogLines(lines);
}

// The active screen: popup layer first, else the newest movie that shows items.
// In-game HUD movies (always on screen while exploring) never take the arrows:
// there the arrow keys move the camera. Seen in test 2026-11-17 01:45.
bool IsHud(const std::wstring& file) {
    static const wchar_t* const kHud[] = {L"Tooltips", L"Quickbar", L"Portraits", L"Minimap", L"NavBar",
                                          L"ArmyControl", L"FloatyLayer", L"Notifications", L"SaveScreen", L"CombinedHUD"};
    for (const wchar_t* h : kHud)
        if (file == h) return true;
    return false;
}

// HUD news spoken without taking the cursor: new notifications ("Задание
// обновлено") and the pause indicator.
bool HasWidget(const MovieState& st) {
    for (const Item& it : st.items)
        if (IsWidget(it)) return true;
    return false;
}

// Dragon Age II training tutorials (popuplayer.gfx TrainingTutorial*_mc, shown by
// PopupLayer.DisplayTutorial): the game runs on under them and their only widget
// is a "Закрыть" that just hides them. As an active screen they took the mod's keys
// (scanner, walking) for as long as they stayed up; a tutorial that waits for an
// action (the camera one) can come back, and a tester was stuck (2026-11-20).
bool TutorialOnly(const MovieState& st) {
    bool any = false;
    for (const Item& it : st.items) {
        if (!IsWidget(it)) continue;
        if (it.path.find(L"TrainingTutorial") == std::wstring::npos) return false;
        any = true;
    }
    return any;
}

void AnnouncePassive(void* movie, MovieState& st) {
    bool notif = st.file == L"Notifications", floaty = st.file == L"FloatyLayer";
    // A popup with text only (training tutorials that wait for an action, test
    // 2026-11-17 03:36) is read once and leaves the keys to the screen below.
    bool textPopup = st.file == L"PopupLayer" && (!HasWidget(st) || TutorialOnly(st));
    if (!notif && !floaty && !textPopup) {
        st.passive.clear();
        return;
    }
    std::unordered_set<std::wstring> now;
    for (const Item& it : st.items) {
        if (floaty && it.path.find(L"PauseIndicator") == std::wstring::npos) continue;
        if (textPopup && IsWidget(it)) continue;  // a tutorial's "Закрыть"
        std::wstring key = textPopup ? it.path + L"|" + it.label : it.path;  // a changed text is new
        now.insert(key);
        if (st.passive.count(key) || it.label.empty()) continue;
        LogLine(L"passive " + st.file + L" \"" + it.label + L"\"");
        Speak(it.label, false);
        // An items alert (agent value "3|<token>") says only "Предметы получены".
        // Research: log what its catalogue entry holds, to find the item names.
        if (notif && it.value.compare(0, 2, L"3|") == 0) {
            std::wstring tree;
            std::string token = Utf8(it.value.substr(2));
            if (GfxCall(movie, "DAOAccess.Explore", {token.c_str(), 2.0, 60.0}, &tree)) {
                std::vector<std::wstring> lines{L"items alert " + it.value.substr(2) + L":"};
                for (auto& l : Split(tree, L'\n')) if (!l.empty()) lines.push_back(L"  " + l);
                LogLines(lines);
            }
        }
    }
    if (floaty && !st.passive.empty() && now.empty()) {
        LogLine(L"passive FloatyLayer pause off");
        Speak(L"Пауза снята", false);
    }
    st.passive.swap(now);
}

void* PickActive(DWORD now) {
    void* best = nullptr;
    unsigned bestSeq = 0;
    for (auto& [m, s] : g_movies) {
        if (s.agent != AGENT_READY || !s.identified || s.items.empty() || now - s.lastSeen > 1000) continue;
        if (IsHud(s.file)) continue;
        if (s.file == L"PopupLayer") {
            if (HasWidget(s) && !TutorialOnly(s)) return m;
            continue;  // text-only popup: announced passively (AnnouncePassive)
        }
        if (!best || s.seq > bestSeq) { best = m; bestSeq = s.seq; }
    }
    return best;
}

// Sections of a screen: items under the same top-level clip (_level0.<clip>...).
// PageUp / PageDown jump between them; known clips get a name.
std::wstring GroupOf(const Item& it) {
    size_t a = it.path.find(L'.');
    if (a == std::wstring::npos) return L"";
    size_t b = it.path.find(L'.', a + 1);
    return it.path.substr(a + 1, b == std::wstring::npos ? std::wstring::npos : b - a - 1);
}

std::wstring GroupName(const std::wstring& g) {
    struct N { const wchar_t* clip; const wchar_t* name; };
    static const N kNames[] = {
        {L"Equipment_mc", L"Экипировка"}, {L"InventoryContents_mc", L"Рюкзак"}, {L"Stats_mc", L"Характеристики"},
        {L"charSelect_mc", L"Персонаж"}, {L"ItemCategories_mc", L"Категории предметов"},
        {L"Container_mc", L"Содержимое"}, {L"ProgressBar_mc", L"Вместимость"}, {L"Money_mc", L"Деньги"},
        {L"OptionTabsGroup_mc", L"Вкладки"}, {L"ScrollingAbilities_mc", L"Способности"},
        {L"ability_description_mc", L"Описание"}, {L"race_description_mc", L"Описание"},
        {L"content_race_mc", L"Выбор"}, {L"StartButtons_mc", L"Главное меню"},
    };
    for (const N& n : kNames)
        if (g == n.clip) return n.name;
    return L"";
}

// ---------------------------------------------------------------- list screens
// Inventory, skills, character record, level-up and stores: the player found the
// one long list tiring (2026-11-17 19:40). There, Left/Right (and PageUp/PageDown)
// switch sections, Up/Down stay inside the section and say "N из M". Skill
// icons: Up/Down go from chain to chain, Left/Right along the chain.

bool SectionMode(const std::wstring& file) {
    // The store movie calls itself "Store" (log 2026-11-18 17:35: "screen Store"), so
    // the old check for "store" never matched and the store was one long list.
    return file == L"inventory" || file == L"abilities" || file == L"CharacterRecord" || file == L"store" ||
           file == L"Store" || file == L"LevelUp";
}

// Store sections (store.gfx Store.as): the merchant's goods sit under StorePage_mc
// with their category tabs in CategoryTabs_store_mc; the party's goods under
// PartyItemWindow_mc with tabs in CategoryTabs_mc; money and the rest at the top.
std::wstring StoreSection(const std::wstring& g) {
    if (g == L"StorePage_mc" || g == L"StoreName_mc") return L"Купить";
    if (g == L"CategoryTabs_store_mc") return L"Товары купца по видам";
    if (g == L"PartyItemWindow_mc" || g == L"ProgressBar_mc" || g == L"Capacity_mc") return L"Продать";
    if (g == L"CategoryTabs_mc") return L"Свои вещи по видам";
    return L"Прочее";
}

// "...AbilityClassContainer20" for a skill icon or its class header, else "".
std::wstring ClassContainer(const std::wstring& path) {
    size_t a = path.find(L"AbilityClassContainer");
    if (a == std::wstring::npos) return L"";
    size_t b = path.find(L'.', a);
    return path.substr(0, b);
}

std::wstring SectionOf(const MovieState& st, const Item& it) {
    std::wstring cc = ClassContainer(it.path);
    if (!cc.empty()) {  // a class of skills: named by its header ("Воин", "Парное оружие")
        for (const Item& x : st.items)
            if (x.kind == L"text" && x.path.compare(0, cc.size(), cc) == 0 &&
                x.path.find(L"LabelBar") != std::wstring::npos && !x.label.empty())
                return x.label;
        return L"Способности";
    }
    if (st.file == L"Store" || st.file == L"store") return StoreSection(GroupOf(it));
    struct N { const wchar_t* clip; const wchar_t* name; };
    static const N kNames[] = {
        {L"InventoryContents_mc", L"Рюкзак"}, {L"ProgressBar_mc", L"Рюкзак"}, {L"Container_mc", L"Содержимое"},
        {L"Equipment_mc", L"Экипировка"}, {L"Stats_mc", L"Характеристики"}, {L"stats_mc", L"Атрибуты"},
        {L"charSelect_mc", L"Персонаж"}, {L"Money_mc", L"Персонаж"}, {L"ItemCategories_mc", L"Фильтры"},
        {L"ItemSortPicker_mc", L"Фильтры"}, {L"ScrollingAbilities_mc", L"Способности"},
        {L"ability_description_mc", L"Описание"}, {L"specializations_mc", L"Специализации"},
        {L"content_attributes_mc", L"Характеристики"}, {L"attribute_points_mc", L"Характеристики"},
    };
    std::wstring g = GroupOf(it);
    for (const N& n : kNames)
        if (g == n.clip) return n.name;
    return L"Прочее";
}

bool SectionHasWidget(const MovieState& st, const std::wstring& sec) {
    for (const Item& x : st.items)
        if (IsWidget(x) && SectionOf(st, x) == sec) return true;
    return false;
}

// The items Up/Down visit in a section: its widgets, or its texts when it has none.
std::vector<size_t> SectionNav(const MovieState& st, const std::wstring& sec) {
    bool widgets = SectionHasWidget(st, sec);
    std::vector<size_t> v;
    for (size_t i = 0; i < st.items.size(); ++i)
        if (SectionOf(st, st.items[i]) == sec && (!widgets || IsWidget(st.items[i]))) v.push_back(i);
    return v;
}

// Sections in screen order.
std::vector<std::wstring> Sections(const MovieState& st) {
    std::vector<std::wstring> v;
    for (const Item& x : st.items) {
        std::wstring s = SectionOf(st, x);
        if (std::find(v.begin(), v.end(), s) == v.end()) v.push_back(s);
    }
    return v;
}

std::wstring ChainOf(const std::wstring& path) {
    size_t d = path.rfind(L'.');
    return d == std::wstring::npos ? path : path.substr(0, d);
}

// The skill icons of one chain, in order.
std::vector<size_t> ChainItems(const MovieState& st, const std::wstring& chain) {
    std::vector<size_t> v;
    for (size_t i = 0; i < st.items.size(); ++i)
        if (st.items[i].kind == L"ability" && ChainOf(st.items[i].path) == chain) v.push_back(i);
    return v;
}

// Chains that share the current one's container, in order.
std::vector<std::wstring> SiblingChains(const MovieState& st, const std::wstring& chain) {
    std::wstring parent = ChainOf(chain);
    std::vector<std::wstring> v;
    for (const Item& x : st.items) {
        if (x.kind != L"ability") continue;
        std::wstring c = ChainOf(x.path);
        if (ChainOf(c) == parent && std::find(v.begin(), v.end(), c) == v.end()) v.push_back(c);
    }
    return v;
}

// ", 3 из 12" (place in the section) or ", 2 из 4" (place in a skill chain).
std::wstring PositionSuffix(const MovieState& st, size_t idx) {
    const Item& it = st.items[idx];
    if (it.listTotal > 0 && it.listIdx >= 0)
        return L", " + std::to_wstring(it.listIdx + 1) + L" из " + std::to_wstring(it.listTotal);
    std::vector<size_t> v;
    if (it.kind == L"ability") v = ChainItems(st, ChainOf(it.path));
    else if (SectionMode(st.file) && IsWidget(it)) v = SectionNav(st, SectionOf(st, it));
    if (v.size() < 2) return L"";
    size_t p = std::find(v.begin(), v.end(), idx) - v.begin();
    if (p >= v.size()) return L"";
    return L", " + std::to_wstring(p + 1) + L" из " + std::to_wstring(v.size());
}

std::unordered_map<std::wstring, std::wstring> g_sectionMemory;  // section -> last item path

// Cursor move: the game's own mouse-over leaves the old widget and enters the new
// one (tooltips, description panels), then the item is spoken. Texts the hover
// changes are read after it (watch).
std::wstring g_hoverPath;

// Typing into an input field. The game's keyboard focus follows the mod's cursor:
// arriving on an "edit" or "password" item focuses it (caret at the end), so what
// the player types goes where the cursor is; leaving it takes the focus away. Before
// 2026-11-20 typing started only with Enter, but the Dragon Age II EA sign-in sends
// the form on any Enter, and its e-mail box kept the focus while the mod's cursor
// moved on, so a password went into the e-mail box (test 13:05).
std::wstring g_editPath, g_editText;  // the field being typed into, its text so far
bool g_editSecret = false;            // a password field: never spoken or logged
bool g_fieldFocused = false;          // the game's focus is in a field the mod put it in
std::wstring g_editDonePath;          // finished with Enter: not refocused until the cursor leaves

bool IsField(const Item& it) { return it.kind == L"edit" || it.kind == L"password"; }

void StopTyping(void* movie) {
    if (g_fieldFocused) GfxCall(movie, "DAOAccess.Unfocus", {});
    g_fieldFocused = false;
    g_editPath.clear();
}

bool StartTyping(void* movie, const Item& it, const char* mode) {
    std::wstring r;
    GfxCall(movie, "DAOAccess.Focus", {Utf8(it.path), std::string(mode)}, &r);
    LogLine(L"edit start " + it.path + L" " + Widen(mode) + L" -> " + r);
    if (r != L"ok") return false;
    g_fieldFocused = true;
    g_editPath = it.path;
    g_editSecret = it.kind == L"password";
    g_editText.clear();
    GfxGet(movie, (Utf8(it.path) + ".text").c_str(), g_editText);
    return true;
}

void MoveTo(void* movie, const MovieState& st, size_t idx, const wchar_t* why, bool interrupt = true) {
    const Item& it = st.items[idx];
    if (it.path != g_editDonePath) g_editDonePath.clear();
    if (g_fieldFocused && it.path != g_editPath) StopTyping(movie);
    if (it.path != g_hoverPath) {
        if (!g_hoverPath.empty()) GfxCall(movie, "DAOAccess.Hover", {Utf8(g_hoverPath), 0.0});
        g_hoverPath.clear();
        SnapshotTexts(st, GetTickCount());
        if (IsWidget(it) && it.path[0] != L'#') {
            GfxCall(movie, "DAOAccess.Hover", {Utf8(it.path), 1.0});
            g_hoverPath = it.path;
        }
    }
    if (SectionMode(st.file)) g_sectionMemory[SectionOf(st, it)] = it.path;
    SpeakItem(st, idx, why, interrupt);
    if (IsField(it) && it.path != g_editPath && it.path != g_editDonePath) StartTyping(movie, it, "end");
}

// Left/Right or PageUp/PageDown on a list screen: the next section, on the item
// last visited there, introduced by its name and its texts ("Рюкзак 9 / 70",
// "Персонаж, Айдан, Деньги, 0").
void SwitchSection(void* movie, const MovieState& st, int dir) {
    auto secs = Sections(st);
    if (secs.size() < 2) { SpeakItem(st, g_curIndex, L"section-none"); return; }
    std::wstring cur = SectionOf(st, st.items[g_curIndex]);
    size_t i = std::find(secs.begin(), secs.end(), cur) - secs.begin();
    if (i >= secs.size()) i = 0;
    // skip sections with nothing to visit
    std::wstring target;
    std::vector<size_t> nav;
    for (size_t step = 1; step <= secs.size(); ++step) {
        target = secs[(i + secs.size() + (dir > 0 ? step : secs.size() - step)) % secs.size()];
        nav = SectionNav(st, target);
        if (!nav.empty()) break;
    }
    if (nav.empty()) return;
    size_t pick = nav[0];
    auto mem = g_sectionMemory.find(target);
    if (mem != g_sectionMemory.end()) {
        size_t m = FindPath(st, mem->second);
        if (m != SIZE_MAX && std::find(nav.begin(), nav.end(), m) != nav.end()) pick = m;
    }
    // Intro: the name, then the section's texts when it also has widgets.
    std::wstring intro = target;
    if (SectionHasWidget(st, target)) {
        int n = 0;
        for (const Item& x : st.items) {
            if (IsWidget(x) || x.label.empty() || SectionOf(st, x) != target || n >= 3) continue;
            if (n == 0 && x.label.compare(0, target.size(), target) == 0) intro = x.label;  // "Рюкзак 9 / 70"
            else intro += L", " + x.label;
            ++n;
        }
    }
    LogLine(L"section " + target + L" \"" + intro + L"\"");
    Speak(intro, true);
    MoveTo(movie, st, pick, dir > 0 ? L"section-next" : L"section-prev", false);
}

// Up/Down on a list screen: inside the section; on skill icons: chain to chain.
void Scan(void* movie, MovieState& st, DWORD now);

// A row of a scrolling list: the list is scrolled so the wanted entry exists on
// screen, the screen is scanned again, and the cursor goes to that entry. Only the
// rows on screen exist (OptimizedList), so before this the inventory read about 12
// items however full it was (player report 2026-11-18).
bool MoveInList(void* movie, MovieState& st, int target, const wchar_t* why) {
    const Item cur = st.items[g_curIndex];
    if (cur.listTotal <= 0) return false;
    std::wstring r;
    GfxCall(movie, "DAOAccess.ListGo", {Utf8(cur.path), (double)target}, &r);
    LogLine(L"list go " + std::to_wstring(target) + L" of " + std::to_wstring(cur.listTotal) + L" -> " + r);
    if (r == L"edge") {
        Speak(target < 0 ? L"Начало списка" : L"Конец списка", true);
        return true;
    }
    if (r.empty() || r == L"missing") return false;
    Scan(movie, st, GetTickCount());
    size_t to = FindPath(st, r);
    if (to == SIZE_MAX) return false;
    MoveTo(movie, st, to, why);
    return true;
}

void MoveInSection(void* movie, MovieState& st, int dir) {
    const Item& cur = st.items[g_curIndex];
    if (cur.listTotal > 0 && MoveInList(movie, st, cur.listIdx + dir, dir > 0 ? L"down" : L"up")) return;
    if (cur.kind == L"ability") {
        std::wstring chain = ChainOf(cur.path);
        // List screens: every chain of the screen in order, so Down at the end of
        // "Воин" goes on into "Парное оружие" (test 20:09: the player was stuck
        // at the end of a branch and used Tab). Character creation: one block.
        std::vector<std::wstring> chains;
        if (SectionMode(st.file)) {
            for (const Item& x : st.items)
                if (x.kind == L"ability" && std::find(chains.begin(), chains.end(), ChainOf(x.path)) == chains.end())
                    chains.push_back(ChainOf(x.path));
        } else {
            chains = SiblingChains(st, chain);
        }
        auto here = ChainItems(st, chain);
        size_t c = std::find(chains.begin(), chains.end(), chain) - chains.begin();
        size_t k = std::find(here.begin(), here.end(), g_curIndex) - here.begin();
        // A Dragon Age II ability tree is one chain container with every icon of the
        // tree (AbilityChainContainer_mc.AbilityIcon1..N), so chain to chain found
        // nothing and Up/Down only said "Конец списка" (log 2026-11-20 20:52, tree
        // Авангард, 11 abilities, reachable only with Left/Right). With one chain on
        // the screen, Up/Down step through it.
        if (SectionMode(st.file) && chains.size() == 1 && k < here.size()) {
            size_t to = dir > 0 ? k + 1 : k - 1;  // wraps to a huge value below 0
            if (to >= here.size()) {
                LogLine(L"ability list edge");
                Speak(dir > 0 ? L"Конец списка" : L"Начало списка", true);
                return;
            }
            MoveTo(movie, st, here[to], dir > 0 ? L"down" : L"up");
            return;
        }
        size_t t = dir > 0 ? c + 1 : c - 1;  // wraps to a huge value below 0
        if (c < chains.size() && t < chains.size()) {
            auto there = ChainItems(st, chains[t]);
            size_t to = there[std::min(k, there.size() - 1)];
            std::wstring oldSec = SectionOf(st, cur), newSec = SectionOf(st, st.items[to]);
            bool crossed = SectionMode(st.file) && newSec != oldSec;
            if (crossed) Speak(newSec, true);  // entering another branch: its name first
            MoveTo(movie, st, to, dir > 0 ? L"chain-down" : L"chain-up", !crossed);
            return;
        }
        if (SectionMode(st.file)) {
            LogLine(L"ability list edge");
            Speak(dir > 0 ? L"Конец списка" : L"Начало списка", true);
            return;
        }
        // Character creation: leave the skill block for the buttons around it.
        size_t from = dir > 0 ? here.back() : here.front();
        size_t n = st.items.size();
        size_t to = dir > 0 ? std::min(from + 1, n - 1) : (from > 0 ? from - 1 : 0);
        MoveTo(movie, st, to, dir > 0 ? L"down" : L"up");
        return;
    }
    auto nav = SectionNav(st, SectionOf(st, cur));
    size_t p = std::find(nav.begin(), nav.end(), g_curIndex) - nav.begin();
    if (nav.empty()) return;
    size_t to;
    if (p >= nav.size()) to = nav[0];
    else to = nav[dir > 0 ? std::min(p + 1, nav.size() - 1) : (p > 0 ? p - 1 : 0)];
    MoveTo(movie, st, to, dir > 0 ? L"down" : L"up");
}

// Left/Right on a skill icon: along its chain (the skill and its upgrades).
void MoveInChain(void* movie, const MovieState& st, int dir) {
    auto here = ChainItems(st, ChainOf(st.items[g_curIndex].path));
    size_t k = std::find(here.begin(), here.end(), g_curIndex) - here.begin();
    if (k >= here.size()) return;
    if ((dir > 0 && k + 1 >= here.size()) || (dir < 0 && k == 0)) {
        Speak(dir > 0 ? L"Конец цепочки" : L"Начало цепочки", true);
        return;
    }
    MoveTo(movie, st, here[dir > 0 ? k + 1 : k - 1], dir > 0 ? L"chain-right" : L"chain-left");
}

// A new screen, or a new scene in the same movie: texts above the first widget
// (title, a dialog's message), then that widget.
DWORD g_announceAt = 0;  // when the last screen or scene was announced

void AnnounceScreen(void* movie, const MovieState& st, const wchar_t* why) {
    g_announceAt = GetTickCount();
    LogLine(std::wstring(why) + L" " + st.file + L" items=" + std::to_wstring(st.items.size()));
    g_curPath.clear();
    g_curSpoken.clear();
    g_pendingOpen.clear();
    g_hoverPath.clear();
    g_watchUntil = 0;
    size_t first = 0;
    while (first < st.items.size() && !IsWidget(st.items[first])) ++first;
    if (first == st.items.size()) first = 0;
    // Inventory, containers, stores: start on the first item, not on "Закрыть".
    for (size_t i = 0; i < st.items.size(); ++i)
        if (st.items[i].kind == L"item") { first = i; break; }
    // Popups (tutorials, dialog boxes) are short: read all their text, then the
    // first widget. Other screens: only the texts above the first widget.
    bool popup = st.file == L"PopupLayer";
    size_t spoken = 0;
    if (st.file == L"conversation") {
        // Lines without voice acting (Conversation.HasVO 0) are read before the answers.
        std::wstring vo, line;
        GfxCall(movie, "DAOAccess.Val", {"Conversation.HasVO"}, &vo);
        GfxCall(movie, "DAOAccess.Val", {"Conversation.NPCLine"}, &line);
        LogLine(L"conversation line vo=" + vo + L" \"" + line + L"\"");
        if (vo != L"1" && !line.empty()) Speak(line, spoken++ == 0);
    }
    // Skills screen: start on the first skill.
    if (st.file == L"abilities")
        for (size_t i = 0; i < st.items.size(); ++i)
            if (st.items[i].kind == L"ability") { first = i; break; }
    bool sections = SectionMode(st.file);
    if (sections) g_sectionMemory.clear();
    for (size_t i = 0; i < st.items.size(); ++i) {
        if (IsWidget(st.items[i]) || (!popup && i >= first)) continue;
        // List screens: only the titles here; each section reads its own texts.
        if (sections && SectionOf(st, st.items[i]) != L"Прочее") continue;
        LogLine(L"screen text " + st.items[i].label);
        Speak(st.items[i].label, spoken++ == 0);
    }
    if (sections && !st.items.empty()) {
        std::wstring sec = SectionOf(st, st.items[first]);
        if (sec != L"Прочее") Speak(sec, spoken++ == 0);
    }
    MoveTo(movie, st, first, why, spoken == 0);
    static bool s_hinted = false;  // once per game session
    if (sections && !s_hinted) {
        s_hinted = true;
        Speak(L"Разделы: стрелки влево и вправо", false);
    }
}

unsigned g_lastSerial = 0;
std::unordered_set<std::wstring> g_lastPaths;

std::wstring CountWord(size_t n) { return std::to_wstring(n) + L" симв."; }

// Enter in a field: the field keeps its text, the game gets the same Enter (the EA
// sign-in sends its form). The cursor stays; the field is focused again only after
// the cursor has left it, or with another Enter on it.
void EndEdit(void* movie, MovieState& st) {
    std::wstring t;
    GfxGet(movie, (Utf8(g_editPath) + ".text").c_str(), t);
    LogLine(L"edit end " + g_editPath + L" " + CountWord(t.size()));
    std::wstring said = t.empty() ? std::wstring(L"пусто") : g_editSecret ? CountWord(t.size()) : t;
    SpeakPrivate(L"Готово, " + said, true);
    g_editDonePath = g_editPath;
    StopTyping(movie);
    st.lastScan = 0;
}

// Returns false when the player leaves the field with Up, Down or Tab: those keys
// are left untouched so the normal key handling moves the cursor in the same frame.
bool RunEdit(void* movie, MovieState& st, DWORD now) {
    std::wstring t;
    GfxGet(movie, (Utf8(g_editPath) + ".text").c_str(), t);
    if (t != g_editText) {
        // Echo typing: what was added, or what was erased. A password says only
        // "звёздочка" per typed character.
        std::wstring say;
        if (t.size() > g_editText.size() && t.compare(0, g_editText.size(), g_editText) == 0)
            say = g_editSecret ? std::wstring(L"звёздочка") : t.substr(g_editText.size());
        else if (g_editText.size() > t.size() && g_editText.compare(0, t.size(), t) == 0)
            say = g_editSecret ? std::wstring(L"удалено, ") + CountWord(t.size())
                               : L"удалено " + g_editText.substr(t.size());
        else
            say = t.empty() ? std::wstring(L"пусто") : g_editSecret ? CountWord(t.size()) : t;
        LogLine(L"edit text " + CountWord(t.size()));
        SpeakPrivate(say, true);
        g_editText = t;
    }
    bool fg = GameIsForeground();
    auto pressed = [&](int vk) { return fg && (GetAsyncKeyState(vk) & 0x8000) != 0; };
    // Up / Down / Tab leave the field (enum order in RunActive: 0 Up, 1 Down, 7 Tab).
    if ((pressed(VK_UP) && !g_keys[0].down) || (pressed(VK_DOWN) && !g_keys[1].down) ||
        (pressed(VK_TAB) && !g_keys[7].down)) {
        StopTyping(movie);
        return false;
    }
    // Left, Right, Home, End move the caret in the field (their state is still
    // tracked so they do not fire when typing ends); Enter finishes.
    const int caret[] = {VK_LEFT, VK_RIGHT, VK_HOME, VK_END};
    for (int k = 0; k < 4; ++k) KeyFired(g_keys[2 + k], caret[k], false, fg, now);
    KeyFired(g_keys[0], VK_UP, false, fg, now);
    KeyFired(g_keys[1], VK_DOWN, false, fg, now);
    KeyFired(g_keys[7], VK_TAB, false, fg, now);
    if (KeyFired(g_keys[6], VK_RETURN, false, fg, now)) EndEdit(movie, st);
    return true;
}

// Keys and announcements for the active agent movie.
void RunActive(void* movie, MovieState& st, DWORD now) {
    if (st.file == L"PartyPicker") PickerTick(movie, now);
    if (g_active != movie) {
        // A screen still being built (its widgets appear over several frames):
        // wait until two scans in a row give the same count.
        if (now - st.born < 1500 && st.items.size() != st.prevCount) return;
        g_active = movie;
        g_activeFile = st.file;
        // A new screen: the old movie's field is not touched (it may be gone).
        g_editPath.clear();
        g_editDonePath.clear();
        g_fieldFocused = false;
        g_lastSerial = st.scanSerial;
        g_lastPaths.clear();
        for (const Item& it : st.items) g_lastPaths.insert(it.path);
        AnnounceScreen(movie, st, L"screen");
        return;
    }
    if (st.scanSerial != g_lastSerial) {
        g_lastSerial = st.scanSerial;
        // A scene change inside the movie (chargen steps): most paths are new.
        size_t kept = 0;
        for (const Item& it : st.items) kept += g_lastPaths.count(it.path);
        size_t before = g_lastPaths.size();
        g_lastPaths.clear();
        for (const Item& it : st.items) g_lastPaths.insert(it.path);
        if (FindPath(st, g_curPath) == SIZE_MAX && kept * 2 < std::max(before, st.items.size())) {
            StopTyping(movie);
            g_editDonePath.clear();
            AnnounceScreen(movie, st, L"scene");
            return;
        }
    }
    if (!g_editPath.empty() && RunEdit(movie, st, now)) return;

    // Find the current item again after a rescan.
    size_t idx = FindPath(st, g_curPath);
    if (idx == SIZE_MAX) {
        // Gone (a list closed): its owner if still shown, else the item now there.
        size_t own = FindOwner(st, g_curPath);
        MoveTo(movie, st, own != SIZE_MAX ? own : std::min(g_curIndex, st.items.size() - 1), L"replaced");
    } else {
        g_curIndex = idx;
        // Level-up skills and talents scene: the scene is announced while it still
        // builds (test 20:08: 2 items, then 67), so the cursor sat on "Закрыть".
        // When the skill icons appear soon after, go to the first one, saying the
        // points left first ("Умения, 1").
        if (g_announceAt != 0 && (int)(now - g_announceAt) < 3000 &&
            (st.file == L"LevelUp" || st.file == L"abilities") && st.items[idx].kind != L"ability") {
            for (size_t i = 0; i < st.items.size(); ++i) {
                if (st.items[i].kind != L"ability") continue;
                g_announceAt = 0;
                bool first = true;
                for (const Item& x : st.items)
                    if (x.kind == L"text" && x.path.find(L"points") != std::wstring::npos && !x.label.empty()) {
                        Speak(x.label, first);
                        first = false;
                    }
                Speak(SectionOf(st, st.items[i]), first);
                MoveTo(movie, st, i, L"skills", false);
                return;
            }
        }
        // A picker just opened: move into its list, on the selected entry.
        if (!g_pendingOpen.empty() && g_pendingOpen == g_curPath) {
            size_t first = SIZE_MAX, sel = SIZE_MAX;
            for (size_t i = 0; i < st.items.size(); ++i) {
                if (st.items[i].kind != L"entry" || FindOwner(st, st.items[i].path) != idx) continue;
                if (first == SIZE_MAX) first = i;
                if (sel == SIZE_MAX && st.items[i].value == L"1") sel = i;
            }
            if (first != SIZE_MAX) {
                g_pendingOpen.clear();
                MoveTo(movie, st, sel != SIZE_MAX ? sel : first, L"opened");
            } else if (now - g_pendingOpenAt > 1500) {
                g_pendingOpen.clear();
            }
        }
        // A pressed button that just turned disabled (a chosen conversation reply,
        // test 19:24: "2. ..., недоступно" after every answer) is not news.
        std::wstring nowSaid = Describe(st, g_curIndex);
        bool justDisabled = (int)(g_watchUntil - now) > 0 && nowSaid == g_curSpoken + L", недоступно";
        if (justDisabled) g_curSpoken = nowSaid;
        else if (nowSaid != g_curSpoken) SpeakItem(st, g_curIndex, L"changed");
    }
    // After a press or a move: read texts that appeared or changed (descriptions).
    if ((int)(g_watchUntil - now) > 0) {
        for (size_t i = 0; i < st.items.size(); ++i) {
            const Item& it = st.items[i];
            if (it.kind != L"text") continue;
            auto w = g_watchTexts.find(it.path);
            if (w != g_watchTexts.end() && w->second == it.label) continue;
            g_watchTexts[it.path] = it.label;
            if (it.path == g_curPath) continue;
            // A bare number ("4") is said with the text before it ("Расходуемых очков").
            std::wstring say = it.label;
            if (say.find_first_not_of(L"0123456789.,-+/% ") == std::wstring::npos && i > 0 &&
                st.items[i - 1].kind == L"text")
                say = st.items[i - 1].label + L", " + say;
            LogLine(L"text changed " + it.path + L" \"" + say + L"\"");
            Speak(say, false);
        }
    }

    bool fg = GameIsForeground();
    // Ctrl+1..Ctrl+0: put the current item or ability on quickbar slot 1..10 (the
    // drop message of the quickbar itself; the game's 1..0 keys then use it).
    {
        static KeyState qs[10];
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        for (int d = 0; d < 10; ++d) {
            int vk = d == 9 ? '0' : '1' + d;
            if (!KeyFired(qs[d], vk, false, fg, now) || !ctrl || st.items.empty()) continue;
            const Item& cur = st.items[g_curIndex];
            std::wstring r;
            GfxCall(movie, "DAOAccess.Quickslot", {Utf8(cur.path), (double)d}, &r);
            LogLine(L"quickslot " + std::to_wstring(d + 1) + L" " + cur.path + L" -> " + r);
            if (r.compare(0, 2, L"ok") == 0)
                Speak(r.substr(3) + L" в ячейке " + std::to_wstring(d + 1), true);
            else
                Speak(L"Это нельзя положить на панель", true);
        }
    }
    enum { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_HOME, K_END, K_ENTER, K_TAB, K_SPACE, K_PGUP, K_PGDN };
    const int vks[] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_HOME, VK_END, VK_RETURN, VK_TAB, VK_SPACE, VK_PRIOR, VK_NEXT};
    for (int k = 0; k <= K_PGDN; ++k) {
        bool repeats = k <= K_RIGHT;
        if (!KeyFired(g_keys[k], vks[k], repeats, fg, now)) continue;
        size_t n = st.items.size();
        const Item& cur = st.items[g_curIndex];
        switch (k) {
        case K_UP:
        case K_DOWN:
            if (SectionMode(st.file) || cur.kind == L"ability") MoveInSection(movie, st, k == K_DOWN ? 1 : -1);
            else if (k == K_UP) MoveTo(movie, st, g_curIndex > 0 ? g_curIndex - 1 : 0, L"up");
            else MoveTo(movie, st, g_curIndex + 1 < n ? g_curIndex + 1 : n - 1, L"down");
            break;
        case K_HOME:
        case K_END:
            if (cur.listTotal > 0 &&
                MoveInList(movie, st, k == K_HOME ? 0 : cur.listTotal - 1, k == K_HOME ? L"home" : L"end")) {
                break;  // first / last of the whole scrolling list
            }
            if (SectionMode(st.file)) {  // first / last of the section
                auto nav = SectionNav(st, SectionOf(st, cur));
                if (!nav.empty()) MoveTo(movie, st, k == K_HOME ? nav.front() : nav.back(), k == K_HOME ? L"home" : L"end");
            } else {
                MoveTo(movie, st, k == K_HOME ? 0 : n - 1, k == K_HOME ? L"home" : L"end");
            }
            break;
        case K_PGUP:
        case K_PGDN: {
            if (SectionMode(st.file)) { SwitchSection(movie, st, k == K_PGDN ? 1 : -1); break; }
            // Next / previous section: the first widget of the next group (texts only
            // when the group has no widget), announced with the section's name.
            std::wstring g = GroupOf(cur);
            size_t i = g_curIndex;
            bool found = false;
            for (size_t step = 1; step < n && !found; ++step) {
                i = k == K_PGDN ? (g_curIndex + step) % n : (g_curIndex + n - step) % n;
                if (GroupOf(st.items[i]) == g) continue;
                // walk to the start of that group, then to its first widget
                std::wstring tg = GroupOf(st.items[i]);
                size_t start = i;
                while (start > 0 && GroupOf(st.items[start - 1]) == tg) --start;
                size_t pick = start;
                for (size_t j = start; j < n && GroupOf(st.items[j]) == tg; ++j)
                    if (IsWidget(st.items[j])) { pick = j; break; }
                i = pick;
                found = true;
            }
            if (!found) { SpeakItem(st, g_curIndex, L"section-none"); break; }
            std::wstring name = GroupName(GroupOf(st.items[i]));
            size_t count = 0, all = 0;
            for (const Item& x : st.items)
                if (GroupOf(x) == GroupOf(st.items[i])) { ++all; if (IsWidget(x)) ++count; }
            if (count == 0) count = all;  // a section of texts (stats)
            if (!name.empty()) Speak(name + L", " + std::to_wstring(count), true);
            MoveTo(movie, st, i, k == K_PGDN ? L"section-next" : L"section-prev", name.empty());
            break;
        }
        case K_SPACE: {
            // Longer description: an item's tooltip (stats), or the widget's tooltip.
            // In a conversation: the other person's last line (catalogue NPCLine).
            std::wstring r;
            if (st.file == L"conversation") GfxCall(movie, "DAOAccess.Val", {"Conversation.NPCLine"}, &r);
            else GfxCall(movie, "DAOAccess.Describe", {Utf8(cur.path)}, &r);
            LogLine(L"describe " + cur.path + L" \"" + r + L"\"");
            Speak(r.empty() ? L"Нет описания" : r, true);
            break;
        }
        case K_TAB: {
            // Next widget (Shift+Tab: previous), skipping plain text.
            bool back = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            size_t i = g_curIndex;
            for (size_t step = 0; step < n; ++step) {
                i = back ? (i + n - 1) % n : (i + 1) % n;
                if (IsWidget(st.items[i])) break;
            }
            MoveTo(movie, st, i, back ? L"shift-tab" : L"tab");
            break;
        }
        case K_LEFT:
        case K_RIGHT:
            if (cur.kind == L"slider" || cur.kind == L"spin") {
                std::wstring r;
                GfxCall(movie, "DAOAccess.Adjust", {Utf8(cur.path), k == K_RIGHT ? 1.0 : -1.0}, &r);
                LogLine(L"adjust " + cur.path + L" -> " + r);
                // Dragon Age II attribute rows (agent A.IsAttrRow): why nothing changed.
                if (r == L"nopoints") Speak(L"Нет свободных очков", true);
                else if (r == L"nospent") Speak(L"Сюда в этот раз очки не добавлялись", true);
                SnapshotTexts(st, now);
                st.lastScan = 0;  // rescan next frame; a new value is spoken as a change
            } else if (cur.kind == L"picker") {
                // DA2 pickers (difficulty dialog) step with their arrow buttons; the
                // agent no longer lists those arrows (they read "без подписи").
                std::wstring r;
                GfxCall(movie, "DAOAccess.Adjust", {Utf8(cur.path), k == K_RIGHT ? 1.0 : -1.0}, &r);
                LogLine(L"picker step " + cur.path + L" -> " + r);
                if (r == L"edge") {
                    Speak(k == K_RIGHT ? L"Конец списка" : L"Начало списка", true);
                } else if (r == L"none") {
                    if (SectionMode(st.file)) SwitchSection(movie, st, k == K_RIGHT ? 1 : -1);
                } else {
                    SnapshotTexts(st, now);
                    st.lastScan = 0;  // the new choice is spoken as a change
                }
            } else if (cur.kind == L"chooser") {
                if (g_pick.movie != movie || (!g_pick.done && !g_pick.running)) {
                    g_pick.movie = movie;
                    PickerStart(now);
                    Speak(L"Ищу персонажей, подождите несколько секунд", true);
                } else if (g_pick.running) {
                    Speak(L"Идёт поиск", true);
                } else if (!g_pick.cands.empty()) {
                    int n = (int)g_pick.cands.size();
                    g_pick.cur = ((g_pick.cur < 0 ? 0 : g_pick.cur) + (k == K_RIGHT ? 1 : -1) + n) % n;
                    PickerPoint(g_pick.cur);
                    LogLine(L"party picker cursor to " + std::to_wstring(g_pick.cands[g_pick.cur].id));
                    SnapshotTexts(st, now);
                    st.lastScan = 0;
                } else {
                    PickerStart(now);
                    Speak(L"Ищу персонажей ещё раз", true);
                }
            } else if (cur.kind == L"ability") {
                MoveInChain(movie, st, k == K_RIGHT ? 1 : -1);
            } else if (SectionMode(st.file)) {
                SwitchSection(movie, st, k == K_RIGHT ? 1 : -1);
            }
            break;
        case K_ENTER: {
            if (IsField(cur)) {
                // Not typing here (finished with Enter, or the focus failed): type
                // again, the old text selected so typing replaces it.
                g_editDonePath.clear();
                if (StartTyping(movie, cur, "all")) Speak(L"Ввод текста, старый текст заменится", true);
                else Speak(L"Не удалось начать ввод", true);
                break;
            }
            if (!IsWidget(cur)) { SpeakItem(st, g_curIndex, L"reread"); break; }
            if (cur.kind == L"spin" || cur.kind == L"slider") {
                Speak(L"Меняйте стрелками влево и вправо", true);
                break;
            }
            if (!cur.enabled) { Speak(L"недоступно", true); break; }
            if (cur.kind == L"member") {  // Origins party list (AddPartyList)
                std::string r = PartyToggle((unsigned)_wtoi(cur.path.c_str() + 4));
                std::wstring name = cur.label.substr(0, cur.label.find(L", уровень"));
                if (r == "added") Speak(L"В отряде: " + name, true);
                else if (r == "removed") Speak(L"Убран из отряда: " + name, true);
                else if (r == "full") Speak(L"В отряде уже четверо. Сначала уберите кого-нибудь", true);
                else if (r == "locked") Speak(name + L": сюжет не даёт изменить", true);
                else if (r == "busy") Speak(L"Подождите, сцена ещё двигается", true);
                else Speak(L"Не получилось", true);
                SnapshotTexts(st, now);
                st.lastScan = 0;
                break;
            }
            if (cur.kind == L"chooser") {  // Dragon Age II cursor pick (AddPartyChooser)
                // PartyPicker.DoAction 11 did nothing on PC (test 00:18: its gamepad
                // button clips do not exist there). A PC player clicks the model, so
                // the mod clicks: the cursor is already on the highlighted character.
                std::wstring confirm;
                GfxCall(movie, "DAOAccess.Val", {"PartyPicker.ChargenConfirmMode"}, &confirm);
                unsigned hid = HighlightedId(movie);
                if (confirm == L"1") {
                    std::wstring r;
                    GfxCall(movie, "DAOAccess.Send", {"GameModeController.ChargenConfirm"}, &r);
                    LogLine(L"party picker chargen confirm -> " + r);
                    Speak(L"Подтверждено", true);
                } else if (!hid) {
                    Speak(L"Сначала выберите персонажа стрелками влево и вправо", true);
                    break;
                } else {
                    if (g_pick.cur >= 0 && g_pick.cur < (int)g_pick.cands.size()) PickerPoint(g_pick.cur);
                    INPUT in[2] = {};
                    in[0].type = in[1].type = INPUT_MOUSE;
                    in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    UINT sent = SendInput(2, in, sizeof(INPUT));
                    LogLine(L"party picker click on " + std::to_wstring(hid) + L" sent " + std::to_wstring(sent));
                }
                SnapshotTexts(st, now);
                st.lastScan = 0;
                break;
            }
            std::wstring r;
            std::wstring path = cur.path;  // cur may move when the list changes
            // Shift+Enter on a backpack item: equip into its second slot (off hand,
            // second ring), as the game's drag onto that slot does.
            if (cur.kind == L"item" && (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
                GfxCall(movie, "DAOAccess.EquipAlt", {Utf8(path)}, &r);
                LogLine(L"equip alt " + path + L" -> " + r);
                if (r.compare(0, 3, L"ok\t") == 0) {
                    size_t tab = r.find(L'\t', 3);
                    std::wstring slot = r.substr(3, tab == std::wstring::npos ? std::wstring::npos : tab - 3);
                    std::wstring name = tab == std::wstring::npos ? L"" : r.substr(tab + 1);
                    std::wstring where = slot == L"1" ? L"Во вторую руку: " : slot == L"10" ? L"На второе кольцо: " : L"Во второй слот: ";
                    Speak(where + name, true);
                } else if (r == L"heavy") {
                    Speak(L"Тяжёлое оружие во второй руке требует высшего таланта парного оружия", true);
                } else if (r == L"one" || r == L"none") {
                    Speak(L"Этот предмет надевается только в один слот, используйте Enter", true);
                } else if (r == L"locked") {
                    Speak(L"Сейчас нельзя менять снаряжение", true);
                } else if (r == L"container" || r == L"notbackpack") {
                    Speak(L"Надеть во второй слот можно только из рюкзака", true);
                } else {
                    Speak(L"Не получилось", true);
                }
                SnapshotTexts(st, now);
                st.lastScan = 0;
                break;
            }
            bool ok = GfxCall(movie, "DAOAccess.Press", {Utf8(path)}, &r);
            LogLine(L"press " + path + L" -> " + (ok ? r : L"invoke failed"));
            if (r == L"picker") { g_pendingOpen = path; g_pendingOpenAt = now; }
            // Gifts (inventory): "gift\t<item>\t<companion>", or the hero selected.
            if (r.compare(0, 5, L"gift\t") == 0) {
                size_t tab = r.find(L'\t', 5);
                std::wstring item = r.substr(5, tab == std::wstring::npos ? std::wstring::npos : tab - 5);
                std::wstring who = tab == std::wstring::npos ? L"" : r.substr(tab + 1);
                Speak(L"Подарено: " + item + (who.empty() ? L"" : L", " + who), true);
            } else if (r == L"gift-hero") {
                Speak(L"Подарок отдают спутнику. Выберите его в разделе Персонаж кнопкой Следующий, затем Enter на подарке",
                      true);
            }
            SnapshotTexts(st, now);
            st.lastScan = 0;
            break;
        }
        }
    }
}

// ---------------------------------------------------------------- FocusManager mode

std::wstring CurrentFocus(void* movie) {
    std::wstring f;
    GfxGet(movie, "FocusManager.m_CurrentFocus", f);
    return f;
}

bool IsShown(void* movie, const std::wstring& clip) {
    std::string p = Utf8(clip);
    size_t pos = p.find('.');  // skip "_level0"
    while (true) {
        size_t next = p.find('.', pos == std::string::npos ? p.size() : pos + 1);
        std::wstring vis;
        if (GfxGet(movie, (p.substr(0, next) + "._visible").c_str(), vis) && vis == L"false") return false;
        if (next == std::string::npos) return true;
        pos = next;
    }
}

std::wstring FmLabel(void* movie, const std::wstring& clip) {
    static const char* const kPaths[] = {".RollOver_mc.ButtonText_MC.TextField.text",
                                         ".ButtonText_MC.TextField.text", ".TextField.text"};
    for (const char* suffix : kPaths) {
        std::wstring t;
        if (GfxGet(movie, (Utf8(clip) + suffix).c_str(), t) && !t.empty()) return t;
    }
    return L"";
}

void RunFocusManager(void* movie, MovieState& st, DWORD now) {
    static const char* const kMethods[] = {"FocusManager.NavigateUp", "FocusManager.NavigateDown",
                                           "FocusManager.NavigateLeft", "FocusManager.NavigateRight"};
    const int vks[] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_RETURN};
    bool fg = GameIsForeground();
    for (int k = 0; k < 5; ++k) {
        if (!KeyFired(st.fmKeys[k], vks[k], k < 4, fg, now)) continue;
        if (k < 4) {
            if (st.allHiddenAt && now - st.allHiddenAt < 1500) continue;
            std::unordered_set<std::wstring> seen;
            bool found = false;
            for (int i = 0; i < 40; ++i) {
                if (!GfxInvoke(movie, kMethods[k])) break;
                std::wstring f = CurrentFocus(movie);
                if (f.empty()) break;
                if (IsShown(movie, f)) { found = true; break; }
                if (!seen.insert(f).second) break;
            }
            st.allHiddenAt = found ? 0 : now;
        } else {
            std::wstring f = CurrentFocus(movie);
            if (f.empty() || !IsShown(movie, f)) continue;
            bool ok = GfxInvoke(movie, (Utf8(f) + ".SimulateButtonPush").c_str());
            LogLine(L"fm enter " + f + (ok ? L"" : L" invoke failed"));
        }
    }
    std::wstring focus = CurrentFocus(movie);
    bool changed = focus != st.focus;
    if (changed || now - st.lastShownCheck >= 250) {
        st.lastShownCheck = now;
        bool shown = !focus.empty() && IsShown(movie, focus);
        bool reappeared = !changed && shown && !st.focusShown;
        st.focus = focus;
        st.focusShown = shown;
        if ((changed || reappeared) && shown) {
            std::wstring label = FmLabel(movie, focus);
            LogLine(L"fm focus " + st.file + L" " + focus + L" \"" + label + L"\"");
            Speak(label.empty() ? L"без подписи" : label, true);
        }
    }
}

}  // namespace

const wchar_t* MovieFileName(void* movie) {
    auto it = g_movies.find(movie);
    if (it == g_movies.end() || it->second.file.empty()) return L"?";
    return it->second.file.c_str();
}

void OnMovieAdvance(void* movie) {
    DWORD now = GetTickCount();
    if (!g_firstAdvanceLogged) {
        g_firstAdvanceLogged = true;
        LogLine(L"advance hook fired, first movie " + Ptr(movie) +
                L", thread " + std::to_wstring(GetCurrentThreadId()));
    }
    auto it = g_movies.find(movie);
    if (it == g_movies.end()) {
        it = g_movies.emplace(movie, MovieState{}).first;
        it->second.seq = ++g_seq;
        it->second.born = now;
        it->second.lastCheck = now - 1000;  // identity check right away
        LogLine(L"movie new " + Ptr(movie));
    }
    MovieState& st = it->second;
    st.lastSeen = now;
    ++st.advances;

    // Identity, focus system, agent health: every 50 ms until the movie has told
    // its name (or a second has passed), then once a second.
    if (now - st.lastCheck >= (st.identified ? 1000u : 50u)) {
        st.lastCheck = now;
        std::wstring file, root;
        GfxGet(movie, "_root.Filename", file);
        if (!st.identified && (!file.empty() || now - st.born >= 1000)) {
            st.identified = true;
            LogLine(L"movie " + Ptr(movie) + L" identified after " + std::to_wstring(now - st.born) + L" ms");
        }
        if (file != st.file) {
            st.file = file;
            LogLine(L"movie " + Ptr(movie) + L" file=" + (file.empty() ? L"(none)" : file));
        }
        GfxGet(movie, "FocusManager.sm_CurrentRootContainer", root);
        st.root = root;
        std::wstring ver;
        if (st.agent == AGENT_READY && !GfxGet(movie, "DAOAccess.ver", ver)) {
            LogLine(L"agent: lost in " + st.file + L", reloading");
            st.agent = AGENT_NONE;
        }
        if (st.agent == AGENT_NONE) StepAgentLoad(movie, st, now);
    }
    if (st.agent == AGENT_LOADING) StepAgentLoad(movie, st, now);

    // A young movie is scanned every 100 ms so a new screen is read quickly; it
    // is announced once two scans agree (RunActive).
    // An idle screen (no key for 3 s) is rescanned once a second: the world map cost
    // 7-9 % of the time at 400 ms (perf log 2026-11-18 01:59).
    DWORD every = now - st.born < 1500 ? 100u : (now - g_lastKeyAt > 3000 ? 1000u : 400u);
    if (st.agent == AGENT_READY && now - st.lastScan >= every) {
        Scan(movie, st, now);
        AnnouncePassive(movie, st);
    }

    // F10: dump the game's data catalogue and the minimap creature batch (research
    // for the scanner). Runs in the scanner's movie, or PopupLayer when there is none.
    if (KeyFired(st.exploreKey, VK_F10, false, GameIsForeground(), now) && st.agent == AGENT_READY) {
        bool hasMinimap = false;
        for (auto& [m, s2] : g_movies)
            if (IsScannerHost(s2.file) && s2.agent == AGENT_READY && now - s2.lastSeen < 1000) hasMinimap = true;
        if (IsScannerHost(st.file) || (!hasMinimap && st.file == L"PopupLayer")) {
            DWORD t0 = GetTickCount();
            std::wstring tree, batch;
            GfxCall(movie, "DAOAccess.Explore", {"", 3.0, 1500.0}, &tree);
            DWORD t1 = GetTickCount();
            GfxCall(movie, "DAOAccess.Batch", {}, &batch);
            std::vector<std::wstring> lines;
            lines.push_back(L"explore in " + st.file + L" took " + std::to_wstring(t1 - t0) + L" ms");
            for (auto& l : Split(tree, L'\n')) if (!l.empty()) lines.push_back(L"  " + l);
            for (auto& l : Split(batch, L'\n')) if (!l.empty()) lines.push_back(L"  batch " + l);
            LogLines(lines);
            if (IsScannerHost(st.file)) ScannerDumpObjects(movie);
            Speak(L"Дамп данных записан", true);
        }
    }
    if (KeyFired(st.dumpKey, VK_F11, false, GameIsForeground(), now)) {
        if (st.agent == AGENT_READY) DumpItems(st);
        else LogLine(L"dump " + st.file + L" agent=" + std::to_wstring(st.agent) + L" root=" + st.root +
                     L" focus=" + CurrentFocus(movie));
    }

    void* active = PickActive(now);
    if (!active && g_active) {
        LogLine(L"screen none (was " + g_activeFile + L")");
        g_active = nullptr;
    }
    if (active == movie) RunActive(movie, st, now);
    else if (!active && st.agent == AGENT_FAILED && !st.root.empty()) RunFocusManager(movie, st, now);
    // World scanner: lives in the always-present HUD movie, silent while a screen is active.
    if (IsScannerHost(st.file) && st.agent == AGENT_READY) ScannerTick(movie, now, active != nullptr);

    // Forget movies not advanced for 5 seconds (closed screens).
    if (now - g_lastSweep >= 5000) {
        g_lastSweep = now;
        for (auto i = g_movies.begin(); i != g_movies.end();) {
            if (now - i->second.lastSeen > 5000) {
                LogLine(L"movie gone " + Ptr(i->first) + L" file=" + i->second.file);
                if (i->first == g_active) g_active = nullptr;
                i = g_movies.erase(i);
            } else {
                ++i;
            }
        }
    }
}
