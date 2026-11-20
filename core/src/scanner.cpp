// World scanner and walk-to, shared by both games. Runs on the game thread inside the
// HUD movie that exists while exploring (Minimap in Origins, CombinedHUD in Dragon
// Age II; menu.cpp IsScannerHost), using the Flash agent's
// DAOAccess.World(), which reads the game's data catalogue: map notes, quest
// destinations, quest givers and party members, with world X/Y in metres.
//
// Keys (exploring only, silent while a menu, popup or conversation is active):
//   PageDown next entry        PageUp previous entry       Home repeat
//   Ctrl+PageDown / Ctrl+PageUp  next / previous category (Все, Персонажи, Статисты,
//        Добыча, Объекты, Выходы, Места, Задания, Отряд, Враги)
//   End  walk to the entry (the game's own click-to-move, move.cpp); End again
//        while walking repeats the order
//   Delete  the current quest's journal text (again within 15 s: the next open quest);
//        changed quest texts are also read out by themselves while exploring
//   ] / [  nearest enemies within 40 m
//   F1..F4 (the game's party selection) are followed by the selected name;
//   Shift+Home  everyone's health and distance; Shift+End  companions come to the
//        controlled character; the game's H (hold position) is spoken
//   Backspace  quickbar list (keys 1..0 with ability names and readiness); the
//        game's 1..0 are echoed, cooldown ends and sustained on/off are spoken
// PageUp/PageDown are the games' secondary zoom keys; the installer frees them in
// KeyBindings.ini (mouse wheel still zooms).
//
// Direction: forward = (-sin o, cos o) for the camera orientation o in degrees;
// a positive relative angle is to the left (proven with the F10 dumps).
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "game.h"
#include "gfx.h"
#include "log.h"
#include "move.h"
#include "objects.h"
#include <unordered_map>
#include <unordered_set>
#include "scanner.h"
#include "speech.h"

namespace {

constexpr double kMetresPerUnit = 1.0;  // proven: "Главный зал, рядом" after walking onto it
constexpr DWORD kStaleMs = 3000;
constexpr double kStaleMove = 5.0;
constexpr double kArrive = 2.5;         // metres: arrival
constexpr DWORD kStuckMs = 3000;        // no movement this long while walking: stopped
                                        // (1.5 s was too short: the path to the cook was
                                        // still being found, test 2026-11-17 03:33)

// Kinds: note (map note), exit (area transition, or a map note with the exit
// icon), quest, giver, party, creature (hostile), person (friendly, has a
// conversation), extra (friendly, nothing to say: a статист), object.
struct Entry {
    std::wstring kind, id, name, extra, tag;
    std::wstring origin;  // "mention" entries: the kind of the entry they copy
    unsigned short act0 = 0, act1 = 0;  // placeables: the game's two actions now
    double x = 0, y = 0;
    bool hpKnown = true;  // creatures: the catalogue gives a health total
    int count = 1;        // > 1: a folded group of this many with the same name (nearest kept)
    std::wstring Key() const {
        if (count > 1) return kind + L":g:" + name;
        return kind + L":" + (id.empty() ? name : id);
    }
};

struct Category { const wchar_t* name; const wchar_t* kinds; };
const Category kCats[] = {
    {L"Все", L"note quest giver party creature person object exit loot"},  // статисты only in their own
    {L"Персонажи", L"person giver party"},
    {L"Статисты", L"extra"},
    {L"Добыча", L"loot"},  // bodies and containers to search (player request 2026-11-18)
    {L"Объекты", L"object"},
    {L"Выходы", L"exit"},
    {L"Места", L"note"},
    {L"Задания", L"quest giver mention"},
    {L"Отряд", L"party"},
    {L"Враги", L"enemy"},  // live hostile creatures: ] and [ jump here
};
constexpr int kCatCount = sizeof(kCats) / sizeof(kCats[0]);
// The same name this many times or more in one list folds into one entry.
constexpr int kFoldMin = 3;

struct KeyState { bool down = false; };

std::vector<Entry> g_list;
std::vector<Entry> g_all;  // every entry of the last refresh, all categories
size_t g_sel = 0;
int g_cat = 0;
std::wstring g_selKey, g_area;
DWORD g_snapAt = 0;
double g_snapX = 0, g_snapY = 0;
KeyState g_next, g_prev, g_repeat, g_walk, g_use, g_enemyNext, g_enemyPrev;
// Names of engine objects, by id, for the current area (agent DAOAccess.Names).
std::unordered_map<unsigned, std::wstring> g_objNames;
// When each creature's catalogue answer was taken: hostility changes during a
// scene (test 17:47: Howe's soldiers turned hostile, the cache still said friendly).
std::unordered_map<unsigned, DWORD> g_objNamesAt;
// Health total from the catalogue (Objects.<id>.Attributes.7.Total); 0 = none.
// Test 20:58: creature 1714 "Солдат Хоу" had none, was attacked 53 times over an
// hour and never died: an enemy without health is not offered as a target.
std::unordered_map<unsigned, double> g_objHp;
std::unordered_set<unsigned> g_unreachable;  // enemies an attack could not reach, this area
// Creatures seen not in play in this area (RawObject::inPlay), to log when one wakes.
std::unordered_set<unsigned> g_dormantSeen;
constexpr DWORD kCreatureNameMs = 4000;
constexpr double kRecheckRange = 60.0;
// Last Refresh timing parts (ms), logged when the whole took long.
double g_tEnum = 0, g_tNames = 0;
size_t g_tAsked = 0;

double NowMs() {
    static LARGE_INTEGER f{};
    if (!f.QuadPart) QueryPerformanceFrequency(&f);
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
}
constexpr unsigned kMaxObjectId = 8000;

constexpr int kQuestCat = 7;           // "Задания"
constexpr double kEnemyRange = 40.0;   // ] / [ : enemies this close only (test 17:23: 55 soldiers
                                       // over the whole castle, [ jumped to one 115 m away)
constexpr double kDoorNoteRange = 12.0;  // a door is labelled with the nearest map note this close

// Journal quests (agent DAOAccess.Quests), polled while exploring so a changed
// quest text is read out, and read on demand with Delete.
struct Quest {
    std::wstring resref, parent, name, desc;
    bool completed = false, updated = false;
    unsigned stamp = 0;  // higher = changed more recently
};
std::vector<Quest> g_quests;
std::unordered_map<std::wstring, std::wstring> g_questSeen;  // resref -> description already known
bool g_questBaseline = false;
unsigned g_questStampNext = 10;
DWORD g_questPollAt = 0, g_questKeyAt = 0;
size_t g_questCycle = 0;
KeyState g_questKey;

// Walk in progress.
bool g_walking = false;
Entry g_target;
DWORD g_walkNextCheck = 0, g_lastMoveAt = 0;
double g_lastX = 0, g_lastY = 0;
std::wstring g_walkArea;     // area of the order: a load ends the watch (test 21:47:
                             // "Буфет, 713 м" said 12 minutes later in Ostagar)
unsigned g_walkerId = 0;
int g_walkRetries = 0;       // orders repeated after a stop (test 22:22: 235 m walk
                             // stopped at 111 m)
constexpr int kWalkRetries = 2;
constexpr double kNear = 5.0;  // a stop this close counts as arrived (people and
                               // objects cannot be stood on: 3.2 m, 4.1 m in test 22:23)
// Enter on a target farther than this (out of a fight): walk there first, then the
// game's default action on arrival. The default action alone accepted "talk" on Сорис
// 117 m away (result 1) but the leader never moved (Origins city elf home, test
// 2026-11-20 16:18); 8 m away (Цирион) it worked.
constexpr double kUseFar = 10.0;
unsigned g_useOnArrive = 0;    // object id to act on when the walk arrives

bool Pressed(KeyState& ks, int vk) {
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool fire = down && !ks.down;
    ks.down = down;
    return fire;
}

std::vector<std::wstring> SplitW(const std::wstring& s, wchar_t sep) {
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

double Num(const std::wstring& s) { return s.empty() ? 0.0 : wcstod(s.c_str(), nullptr); }

constexpr int kEnemyCat = 9;

bool InCategory(const Entry& e) {
    // Enemies that an attack could not reach ("Не получается достать") are left out
    // for the rest of the area. Health alone cannot decide it: in the Korcari Wilds
    // (test 23:32) 58 darkspawn had no catalogue health at all.
    if (g_cat == kEnemyCat)
        return e.kind == L"creature" && e.extra.find(L"враг") != std::wstring::npos &&
               !g_unreachable.count((unsigned)_wtoi(e.id.c_str()));
    for (const std::wstring& k : SplitW(kCats[g_cat].kinds, L' '))
        if (k == e.kind) return true;
    return false;
}

struct Player { std::wstring id, name; double x = 0, y = 0, orient = 0; bool ok = false; };

// Reads the agent's World() lines into the player and all entries.
std::vector<std::wstring> SplitW(const std::wstring& s, wchar_t sep);

// A plain word from a designer tag ("bhn000ip_door_main" -> "дверь") for objects
// that have no name.
std::wstring TagWord(const std::wstring& tag) {
    std::wstring t = tag;
    for (auto& c : t) c = (wchar_t)towlower(c);
    struct W { const wchar_t* part; const wchar_t* word; };
    static const W kWords[] = {
        {L"door", L"дверь"}, {L"gate", L"ворота"}, {L"chest", L"сундук"}, {L"crate", L"ящик"},
        {L"barrel", L"бочка"}, {L"sack", L"мешок"}, {L"box", L"ящик"}, {L"cabinet", L"шкаф"},
        {L"wardrobe", L"шкаф"}, {L"bookcase", L"книжный шкаф"}, {L"book", L"книга"}, {L"desk", L"стол"},
        {L"corpse", L"тело"}, {L"body", L"тело"}, {L"lever", L"рычаг"}, {L"ladder", L"лестница"},
        {L"stair", L"лестница"}, {L"transition", L"переход"}, {L"exit", L"выход"}, {L"container", L"контейнер"},
        {L"vase", L"ваза"}, {L"urn", L"урна"}, {L"pile", L"куча"}, {L"trap", L"ловушка"},
    };
    for (const W& w : kWords)
        if (t.find(w.part) != std::wstring::npos) return w.word;
    return L"";
}

// Raw memory of every live creature, once per area per game session, into
// DAOAccess.objdump.txt next to the log (capped at 6 MB): kept for finding the
// game's "hidden" flag by comparing people the player sees with those he does not
// (camp emissaries, test 2026-11-18 16:20).
bool g_dumpPending = false;
DWORD g_dumpAt = 0;  // the dump waits 5 s after an area change (flags settle after a load)
std::unordered_set<std::wstring> g_dumpedAreas;

void DumpCreatures(const std::vector<RawObject>& objs, const std::wstring& area) {
    std::wstring path = LogPath();
    path = path.substr(0, path.rfind(L'\\') + 1) + L"DAOAccess.objdump.txt";
    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fa) && fa.nFileSizeLow > 6u * 1024 * 1024) return;
    HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    std::string out = "== area " + Utf8(area) + "\n";
    static unsigned char buf[0xC00];
    for (const RawObject& o : objs) {
        if (o.cls != "CCCreature" || o.dead) continue;
        if (!ReadObjectBytes(o.id, buf, sizeof buf)) continue;
        char head[200];
        sprintf_s(head, "obj %u pos %.1f %.1f sel %u %u talk %d f7f0 %d tag ", o.id, o.x, o.y, o.sel23c, o.sel23d,
                  o.talk ? 1 : 0, o.flag7f0);
        out += head + Utf8(o.tag) + " name " + Utf8(o.name) + "\n";
        static const char* hx = "0123456789abcdef";
        for (size_t i = 0; i < sizeof buf; i += 64) {
            char line[200];
            int n = sprintf_s(line, "%04x ", (unsigned)i);
            for (size_t k = 0; k < 64; ++k) {
                line[n++] = hx[buf[i + k] >> 4];
                line[n++] = hx[buf[i + k] & 15];
            }
            line[n++] = '\n';
            out.append(line, n);
        }
    }
    DWORD w = 0;
    WriteFile(h, out.data(), (DWORD)out.size(), &w, nullptr);
    CloseHandle(h);
    LogLine(L"objdump written for \"" + area + L"\" (" + std::to_wstring(out.size()) + L" bytes)");
}

// Creatures parked out of the world: Flemeth at (-58, -18) in the Korcari Wilds,
// 1380 m from the player (test 23:31). Interiors use negative coordinates too (the
// Lothering chantry, -37..4), so only a negative position over 1000 m away counts.
// The target flag +0x23C is NOT a hidden mark: it is 0 for most creatures for a
// second or two after a load and then 1 for all (log 18:51:54 in the chantry: 20
// of 42 at 0, the revered mother and the priests among them, 0 of 42 at 18:56),
// so agent v26's filter on it hid real people and was removed.
bool HiddenCreature(const RawObject& o, const Player& pl, bool) {
    if (o.cls != "CCCreature" || o.dead) return false;
    return (o.x < 0 || o.y < 0) && std::hypot(o.x - pl.x, o.y - pl.y) > 1000.0;
}

// Where a usable placeable belongs, from the game's own action for it
// (placeable_interactions.gda: 15 Area Transition, 11 Loot, 1 Open, 3 Unlock):
// - a transition (the church door in Lothering, test 18:26, sat among 44
//   objects) is an exit;
// - something to loot, or a closed thing to open or unlock that is named or
//   tagged as a container (chest, crate, barrel, sack...), is loot;
// - the rest (inner doors, levers, boards, herbs that are used) stay objects.
std::wstring Lower(std::wstring s);

bool ContainerWord(const Entry& e) {
    std::wstring low = Lower(e.name + L" " + e.tag);
    static const wchar_t* const kWords[] = {
        L"сундук", L"ящик", L"бочк", L"бочон", L"мешок", L"мешк", L"шкаф", L"урна", L"ваза", L"корзин", L"ларец",
        L"ларч", L"тайник", L"сумк", L"кошел", L"останки", L"труп", L"куча", L"шкатул", L"chest", L"crate",
        L"barrel", L"sack", L"box", L"cabinet", L"wardrobe", L"urn", L"vase", L"basket", L"bag", L"corpse",
        L"body", L"pile", L"container", L"cache", L"remains"};
    for (const wchar_t* w : kWords)
        if (low.find(w) != std::wstring::npos) return true;
    return false;
}

std::wstring PlaceableKind(const Entry& e) {
    auto has = [&](unsigned short a) { return e.act0 == a || e.act1 == a; };
    if (has(15)) return L"exit";
    if (has(11)) return L"loot";
    if ((has(1) || has(3)) && ContainerWord(e)) return L"loot";
    return L"object";
}

// Adds the engine's own objects: people, usable objects, exits, items on the
// ground. Catalogue entries with the same id (quest targets, party) win.
void AddEngineObjects(void* movie, const Player& pl, std::vector<Entry>& out) {
    std::vector<RawObject> objs;
    double t0 = NowMs();
    // Dragon Age II: the position offset inside objects is found on the player first
    // (Origins knows it: its PosOffset() is never negative).
    if (PosOffset() < 0 && pl.ok) CalibratePos((unsigned)_wtoi(pl.id.c_str()), pl.x, pl.y);
    EnumObjects(objs, kMaxObjectId);
    g_tEnum = NowMs() - t0;
    g_tNames = 0;
    g_tAsked = 0;
    if (g_dumpPending && (int)(GetTickCount() - g_dumpAt) >= 0) {
        g_dumpPending = false;
        if (!g_area.empty() && !g_dumpedAreas.count(g_area)) {
            g_dumpedAreas.insert(g_area);
            DumpCreatures(objs, g_area);
        }
    }
    int live = 0, unflagged = 0;
    for (const RawObject& o : objs)
        if (o.cls == "CCCreature" && !o.dead && o.posOk) {
            ++live;
            if (o.sel23c == 0) ++unflagged;
        }
    bool flagUsable = live > 0 && unflagged * 10 < live * 8;
    std::wstring hidden;
    std::unordered_set<std::wstring> known;
    for (const Entry& e : out) known.insert(e.id);
    known.insert(pl.id);
    std::vector<Entry> add;
    std::vector<bool> talkable;  // parallel to add: friendly creature with a conversation
    std::vector<bool> dormant;   // parallel to add: creature not in play (left out below)
    std::wstring woke, offObjs;
    size_t nOffObjs = 0;
    std::wstring ask;
    for (const RawObject& o : objs) {
        if (!o.posOk) continue;
        if (HiddenCreature(o, pl, flagUsable) && !known.count(std::to_wstring(o.id))) {
            hidden += L" " + std::to_wstring(o.id) + L":" + (o.name.empty() ? o.tag : o.name);
            continue;
        }
        // Objects and exits not in play are only logged (Origins: not proven to matter).
        if (!o.inPlay && ((o.cls == "CCPlaceable" && o.usable) || o.cls == "CCTriggerAreaTransition"))
            if (++nOffObjs <= 40) offObjs += L" " + std::to_wstring(o.id) + L":" + (o.name.empty() ? o.tag : o.name);
        std::wstring kind;
        if (o.cls == "CCCreature" && o.dead && !o.lootable) continue;  // the dead leave the list
        if (o.cls == "CCCreature" && o.dead) kind = L"loot";              // a body to search
        else if (o.cls == "CCCreature") kind = L"creature";
        else if (o.cls == "CCPlaceable" && o.usable) kind = L"object";
        else if (o.cls == "CCTriggerAreaTransition") kind = L"exit";
        // CCItem objects are mostly gear worn by people, with positions that are not
        // theirs (test 2026-11-17 03:30: dozens of guard shields "23 m" away); loot on
        // the ground is a placeable (bag), so items are left out.
        else continue;
        if (kind == L"item") {
            // Carried items may sit exactly on their owner: keep only items on the ground.
            bool carried = std::hypot(o.x - pl.x, o.y - pl.y) < 0.05;
            for (const Entry& m : out)
                if (m.kind == L"party" && std::hypot(o.x - m.x, o.y - m.y) < 0.05) carried = true;
            if (carried) continue;
        }
        std::wstring id = std::to_wstring(o.id);
        if (known.count(id)) continue;
        Entry e;
        e.kind = kind;
        e.id = id;
        e.x = o.x;
        e.y = o.y;
        e.name = o.name;  // from memory; the catalogue name below wins when there is one
        e.tag = o.tag;
        e.act0 = o.act0;
        e.act1 = o.act1;
        if (o.dead) e.extra = L"тело";
        add.push_back(e);
        talkable.push_back(o.talk && o.flag7f0 == 0);
        // Creatures not in play (RawObject::inPlay): reserve waves, ambushes, people of
        // a later scene, companions parked outside the party. Catalogue entries such
        // as the party never get here (known above).
        bool sleeping = o.cls == "CCCreature" && !o.dead && !o.inPlay;
        dormant.push_back(sleeping);
        if (sleeping) g_dormantSeen.insert(o.id);
        else if (o.cls == "CCCreature" && g_dormantSeen.erase(o.id))
            woke += L" " + id + L":" + (o.name.empty() ? o.tag : o.name);
        // Asked once; creatures within kRecheckRange again after kCreatureNameMs
        // (hostility changes in a scene). Re-asking every creature of the area on
        // every key press was suspected in the lag report of 2026-11-18.
        bool stale = !g_objNames.count(o.id) ||
                     (kind == L"creature" && std::hypot(o.x - pl.x, o.y - pl.y) <= kRecheckRange &&
                      (int)(GetTickCount() - g_objNamesAt[o.id]) > (int)kCreatureNameMs);
        if (stale) ask += (ask.empty() ? L"" : L",") + id;
    }
    if (!ask.empty()) {
        std::wstring txt;
        double t1 = NowMs();
        GfxCall(movie, "DAOAccess.Names", {Utf8(ask)}, &txt);
        g_tNames = NowMs() - t1;
        auto ids = SplitW(ask, L',');
        g_tAsked = ids.size();
        auto lines = SplitW(txt, L'\n');
        for (size_t i = 0; i < ids.size(); ++i) {
            std::wstring name, extra;
            if (i < lines.size()) {
                auto f = SplitW(lines[i], L'\t');
                name = f.size() > 0 ? f[0] : L"";
                if (f.size() > 2 && f[2] == L"1") extra += L"враг ";
                if (f.size() > 3 && f[3] == L"1") extra += L"мёртв";
                if (!extra.empty()) name += L"|" + extra;
                g_objHp[(unsigned)_wtoi(ids[i].c_str())] = f.size() > 4 ? Num(f[4]) : 0.0;
            }
            g_objNames[(unsigned)_wtoi(ids[i].c_str())] = name;
            g_objNamesAt[(unsigned)_wtoi(ids[i].c_str())] = GetTickCount();
        }
    }
    std::wstring slept, contra;
    size_t nSlept = 0;
    for (size_t k = 0; k < add.size(); ++k) {
        Entry& e = add[k];
        unsigned uid = (unsigned)_wtoi(e.id.c_str());
        if (dormant[k]) {
            // A creature not in play cannot be hit (Dragon Age II test 19:55: every
            // enemy attacked without health was one). Kept only if the catalogue
            // shows its health, which would mean the flag is wrong.
            if (g_objHp[uid] > 0) {
                contra += L" " + e.id;
            } else {
                ++nSlept;
                if (nSlept <= 40) slept += L" " + e.id;
                continue;
            }
        }
        std::wstring n = g_objNames[uid];
        size_t bar = n.find(L'|');
        if (bar != std::wstring::npos) {
            if (e.extra != L"тело") e.extra = n.substr(bar + 1);  // a body is no longer an enemy
            n.resize(bar);
        }
        if (e.kind == L"creature") {
            e.hpKnown = g_objHp[uid] > 0;
            // Friendly: a character with a conversation, or a статист (cursor code
            // 0x70CAD0 gives such creatures no action).
            if (e.extra.find(L"враг") == std::wstring::npos) e.kind = talkable[k] ? L"person" : L"extra";
        }
        if (n.empty()) n = e.name;  // memory name
        if (n.empty()) n = TagWord(e.tag);
        if (n.empty())
            n = e.kind == L"object" ? L"объект" : e.kind == L"exit" ? L"выход" : L"существо";
        e.name = n;
        if (e.kind == L"object") e.kind = PlaceableKind(e);
        out.push_back(e);
    }
    if (!woke.empty()) LogLine(L"scan woke:" + woke);
    static std::wstring s_lastSlept;
    std::wstring sl = L"scan not in play left out: " + std::to_wstring(nSlept) + L":" + slept +
                      (contra.empty() ? L"" : L" | flag contradicted by health:" + contra) +
                      L" | not in play objects kept: " + std::to_wstring(nOffObjs) + L":" + offObjs;
    if (sl != s_lastSlept) LogLine(sl);
    s_lastSlept = sl;
    static std::wstring s_lastHidden;
    std::wstring sum = L"scan hidden creatures (" + std::to_wstring(unflagged) + L" of " + std::to_wstring(live) +
                       L" without the target flag" + (flagUsable ? L"" : L", flag ignored") + L"):" + hidden;
    if (sum != s_lastHidden) LogLine(sum);
    s_lastHidden = sum;
}

// Doors all read "Дверь" (test 17:26: 20 of them in the castle). Each gets the
// nearest map note (room names such as "Комната Фергюса") as "возле: ...".
// HYPOTHESIS: map notes sit inside or at the door of their room.
void LabelDoors(std::vector<Entry>& all) {
    for (Entry& e : all) {
        if (e.kind != L"object" && e.kind != L"exit") continue;
        std::wstring low = e.name + L" " + e.tag;
        for (auto& c : low) c = (wchar_t)towlower(c);
        if (low.find(L"двер") == std::wstring::npos && low.find(L"ворот") == std::wstring::npos &&
            low.find(L"door") == std::wstring::npos && low.find(L"gate") == std::wstring::npos)
            continue;
        const Entry* best = nullptr;
        double bestD = kDoorNoteRange;
        for (const Entry& n : all) {
            if (n.kind != L"note" || n.name.empty()) continue;
            double d = std::hypot(n.x - e.x, n.y - e.y);
            if (d < bestD) { bestD = d; best = &n; }
        }
        if (best) e.name += L", возле: " + best->name;
    }
}

// Lower case for Latin and Cyrillic (the CRT's "C" locale only folds ASCII).
std::wstring Lower(std::wstring s) {
    for (auto& c : s) {
        if (c >= 0x410 && c <= 0x42F) c = (wchar_t)(c + 0x20);
        else if (c == 0x401) c = 0x451;
        else c = (wchar_t)towlower(c);
    }
    return s;
}

bool Letter(wchar_t c) { return (c >= 0x400 && c <= 0x4FF) || iswalpha(c); }

// Lower-case words of a text, markup (<emp>...</emp>) left out.
std::vector<std::wstring> Words(const std::wstring& text) {
    std::vector<std::wstring> v;
    std::wstring cur;
    bool tag = false;
    for (wchar_t c : Lower(text) + L" ") {
        if (c == L'<') tag = true;
        if (!tag && Letter(c)) { cur += c; continue; }
        if (c == L'>') tag = false;
        if (!cur.empty()) v.push_back(cur);
        cur.clear();
    }
    return v;
}

bool IsBeing(const Entry& e) {
    return e.kind == L"person" || e.kind == L"extra" || e.kind == L"party" || e.kind == L"creature";
}

// Doubles the player met in the camp (test 2026-11-18 15:10): every companion has
// a map note at their spot ("Морриган", tag wp_camp_gen00fl_morrigan) beside the
// person herself, and the world map exit was there three times (exit note plus
// two placeables "Карта мира"). People and objects are what Enter acts on, so:
// - a map note with the name of a creature in the area goes;
// - a companion spot note (tag "gen00fl", the dog's note says "Пес" while the dog
//   is named by the player) goes when a creature stands within 6 m of it;
// - a map note or exit note goes when an object or exit of the same name is
//   within 25 m (the door "Церковь" beside the note "Церковь", "Доска певчего").
// Same-named objects all stay: the rule that dropped one within 5 m of another
// (agent v25) hid separate bodies and crates in Lothering ("Разбойник",
// "Мертвый храмовник", "Ящик", log 16:52) and missed the two world map tables,
// which stand 8 m apart.
void Dedupe(std::vector<Entry>& all) {
    std::vector<bool> drop(all.size(), false);
    // A shop note (icon ico_mm_store, "Купец" in Lothering, test 18:51: the only
    // entry of "Места") marks the merchant standing there: the person within 10 m
    // gets ", торговец" and the note goes.
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i].kind != L"note" || all[i].extra.find(L"ico_mm_store") == std::wstring::npos) continue;
        Entry* best = nullptr;
        double bestD = 10.0;
        for (Entry& o : all) {
            if (o.kind != L"person" && o.kind != L"giver") continue;
            double d = std::hypot(o.x - all[i].x, o.y - all[i].y);
            if (d < bestD) { bestD = d; best = &o; }
        }
        if (!best) continue;
        std::wstring low = Lower(best->name);
        if (low.find(L"торгов") == std::wstring::npos && low.find(L"купец") == std::wstring::npos)
            best->name += L", торговец";
        drop[i] = true;
    }
    for (size_t i = 0; i < all.size(); ++i) {
        Entry& e = all[i];
        std::wstring name = Lower(e.name);
        if (name.empty()) continue;
        bool note = e.kind == L"note" || (e.kind == L"exit" && e.extra.find(L"ico_mm_") != std::wstring::npos);
        if (note) {
            bool spot = Lower(e.extra).find(L"gen00fl") != std::wstring::npos;
            for (size_t j = 0; j < all.size() && !drop[i]; ++j) {
                const Entry& o = all[j];
                if (j == i) continue;
                double d = std::hypot(o.x - e.x, o.y - e.y);
                if (IsBeing(o) && (Lower(o.name) == name || (spot && d <= 6.0))) drop[i] = true;
                // Exit notes and plain map notes: the object (door) of that name is what
                // Enter uses (test 18:26: the player stood on the note "Церковь",
                // pressed Enter, only walked; the door object:14895 "Церковь" opens it).
                if ((o.kind == L"object" || o.kind == L"exit" || o.kind == L"loot") &&
                    o.extra.find(L"ico_mm_") == std::wstring::npos && Lower(o.name) == name && d <= 25.0)
                    drop[i] = true;
            }
        }
    }
    std::vector<Entry> out;
    std::wstring gone;
    for (size_t i = 0; i < all.size(); ++i) {
        if (drop[i]) gone += L" " + all[i].Key() + L" \"" + all[i].name + L"\"";
        else out.push_back(all[i]);
    }
    if (!gone.empty()) {
        static std::wstring s_lastGone;
        if (gone != s_lastGone) LogLine(L"scan doubles dropped:" + gone);
        s_lastGone = gone;
    }
    all.swap(out);
}

bool ReadWorld(void* movie, Player& pl, std::vector<Entry>& out) {
    std::wstring txt;
    if (!GfxCall(movie, "DAOAccess.World", {}, &txt) || txt.empty()) return false;
    for (const std::wstring& line : SplitW(txt, L'\n')) {
        if (line.empty()) continue;
        auto f = SplitW(line, L'\t');
        if (f.size() < 6) continue;
        if (f[0] == L"player") {
            pl.id = f[1]; pl.name = f[2]; pl.x = Num(f[3]); pl.y = Num(f[4]); pl.orient = Num(f[5]);
            pl.ok = !f[3].empty();
            continue;
        }
        if (f[3].empty() && f[4].empty()) continue;  // no position
        Entry e;
        e.kind = f[0]; e.id = f[1]; e.name = f[2]; e.x = Num(f[3]); e.y = Num(f[4]); e.extra = f[5];
        // Map notes with the exit icon are the area's exits (Ostagar: "Королевский
        // тракт", "Дикие земли Коркари"; "Выходы" was empty there, test 21:53).
        if (e.kind == L"note" && e.extra.find(L"ico_mm_exit") != std::wstring::npos) e.kind = L"exit";
        if (e.kind == L"quest") {
            // extra = "ActiveQuest|Tooltip"; the tooltip is the quest's name (test 17:07:
            // "Поручение отца"). Some targets have no object name (quest:218).
            size_t bar = e.extra.find(L'|');
            if (bar != std::wstring::npos) e.tag = e.extra.substr(bar + 1);
            if (e.name.empty()) e.name = L"отметка";
        }
        out.push_back(e);
    }
    // A quest marker may be a map note (test 18:04: quest:1074 = note:1074 "Главный
    // зал", with no object name): use the note's name.
    for (Entry& q : out)
        if (q.kind == L"quest" && q.name == L"отметка")
            for (const Entry& n : out)
                if ((n.kind == L"note" || n.kind == L"exit") && n.id == q.id && !n.name.empty()) q.name = n.name;
    return pl.ok;
}

double Dist(const Player& p, const Entry& e) { return std::hypot(e.x - p.x, e.y - p.y) * kMetresPerUnit; }

// People and статисты with the same name, kFoldMin or more of them, become one
// entry at the nearest of them (test 21:49, Ostagar: 28 "Солдат" and 14 "Пес
// мабари" among 91 creatures; the player paged to entry 121). End and Enter act
// on that nearest one.
void Fold(const Player& p, std::vector<Entry>& list) {
    std::unordered_map<std::wstring, int> counts;
    for (const Entry& e : list)
        if (e.kind == L"person" || e.kind == L"extra") counts[e.kind + L"|" + e.name]++;
    std::unordered_map<std::wstring, size_t> rep;  // group -> index in out
    std::vector<Entry> out;
    for (const Entry& e : list) {
        std::wstring g = e.kind + L"|" + e.name;
        auto c = counts.find(g);
        if (c == counts.end() || c->second < kFoldMin) { out.push_back(e); continue; }
        auto r = rep.find(g);
        if (r == rep.end()) {
            rep[g] = out.size();
            out.push_back(e);
            out.back().count = c->second;
        } else if (Dist(p, e) < Dist(p, out[r->second])) {
            int n = out[r->second].count;
            out[r->second] = e;
            out[r->second].count = n;
        }
    }
    list.swap(out);
}

std::wstring Direction(const Player& p, const Entry& e) {
    double dx = e.x - p.x, dy = e.y - p.y;
    if (std::hypot(dx, dy) < 1.0) return L"";
    const double kPi = 3.14159265358979;
    double rel = std::atan2(-dx, dy) * 180.0 / kPi - p.orient;
    while (rel > 180) rel -= 360;
    while (rel <= -180) rel += 360;
    static const wchar_t* const kSectors[] = {L"впереди", L"впереди слева", L"слева", L"сзади слева",
                                              L"сзади", L"сзади справа", L"справа", L"впереди справа"};
    return kSectors[(int)std::floor((rel + 22.5 + 360.0) / 45.0) % 8];
}

std::wstring KindWord(const Entry& e) {
    if (e.kind == L"quest") return e.tag.empty() ? L"цель задания, " : L"цель задания «" + e.tag + L"», ";
    if (e.kind == L"giver") return L"задание, ";
    if (e.kind == L"mention") return L"по заданию «" + e.tag + L"», ";
    if (e.kind == L"party") return L"спутник, ";
    if (e.kind == L"exit") return L"выход, ";
    if (e.kind == L"creature" && e.extra.find(L"враг") != std::wstring::npos) return L"враг, ";
    return L"";
}

std::wstring DistText(double d) {
    return d < 1.5 ? std::wstring(L"рядом") : std::to_wstring((int)std::lround(d)) + L" м";
}

void SpeakEntry(const Player& p, const wchar_t* why) {
    if (g_list.empty()) {
        LogLine(L"scan empty in category " + std::wstring(kCats[g_cat].name));
        Speak(std::wstring(kCats[g_cat].name) + L": ничего не найдено", true);
        return;
    }
    const Entry& e = g_list[g_sel];
    g_selKey = e.Key();
    std::wstring dir = Direction(p, e);
    std::wstring say = KindWord(e) + (e.name.empty() ? L"без имени" : e.name) +
                       (e.count > 1 ? L", всего " + std::to_wstring(e.count) + L", ближайший" : L"") +
                       (e.extra == L"тело" ? L", тело" : L"") + L", " + DistText(Dist(p, e)) +
                       (dir.empty() ? L"" : L", " + dir) + L", " + std::to_wstring(g_sel + 1) + L" из " +
                       std::to_wstring(g_list.size());
    wchar_t pos[160];
    swprintf_s(pos, L" | player %.2f %.2f o=%.1f entry %.2f %.2f", p.x, p.y, p.orient, e.x, e.y);
    LogLine(std::wstring(L"scan ") + why + L" " + e.Key() + L" \"" + say + L"\" extra=" + e.extra + pos);
    Speak(say, true);
}

std::wstring QuestTitle(const Quest& q);

// Quest steps the designers gave no marker (test 2026-11-18 17:08: "talk to the
// revered mother" about Sten, no marker anywhere, the player did not know where to
// look). People, objects, places and exits whose name appears in an open quest's
// text are copied into "Задания" as "по заданию «...»". Russian words change
// endings ("преподобной матерью" for "Преподобная мать"), so each word of the name
// (3 letters or more) must share its start with a word of the text: 3 letters for
// short words, up to 6 for long ones. Bodies and enemies are left out (a bandit
// quest would mark every bandit).
bool WordMatch(const std::wstring& w, const std::wstring& t) {
    size_t n = 0;
    while (n < w.size() && n < t.size() && w[n] == t[n]) ++n;
    size_t need = w.size() <= 4 ? 3 : std::min<size_t>(w.size() - 2, 6);
    return n >= need;
}

// Words of a text written with a capital letter inside a sentence (names:
// "Стэна", "Мириам"), lower-cased; sentence starts and the heading do not count.
std::vector<std::wstring> NameWords(const std::wstring& text) {
    std::vector<std::wstring> v;
    std::wstring cur;
    bool tag = false, start = true, cap = false;
    for (wchar_t c : text + L" ") {
        if (c == L'<') tag = true;
        if (!tag && Letter(c)) {
            if (cur.empty()) cap = (c >= 0x410 && c <= 0x42F) || c == 0x401 || (c >= L'A' && c <= L'Z');
            cur += c;
            continue;
        }
        if (!cur.empty()) {
            if (cap && !start) v.push_back(Lower(cur));
            start = false;
            cur.clear();
        }
        if (c == L'>') tag = false;
        if (c == L'.' || c == L'!' || c == L'?' || c == L':' || c == L'\n') start = true;
    }
    return v;
}

// Test 18:10 (Lothering): 40 "Беженец", 3 "Храмовник", "Разбойник", "Торговец",
// "Купец" were listed because the texts speak of refugees, templars, bandits. Now
// only one of a kind in the area (a name met twice is a crowd), no статисты, and a
// one-word name must stand in the text as a name (capital letter inside a
// sentence: "Стэна"); multi-word names ("Преподобная мать", "Старейшина Мириам")
// match by every word.
void AddMentions(std::vector<Entry>& all) {
    struct QW { std::wstring title; std::vector<std::wstring> words, names; };
    std::vector<QW> qs;
    for (const Quest& q : g_quests)
        if (!q.completed && !q.desc.empty()) qs.push_back({QuestTitle(q), Words(q.desc), NameWords(q.desc)});
    if (qs.empty()) return;
    std::unordered_map<std::wstring, int> seen;  // kind|lower name -> how many
    for (const Entry& e : all) seen[e.kind + L"|" + Lower(e.name)]++;
    std::unordered_set<std::wstring> added;
    std::vector<Entry> add;
    for (const Entry& e : all) {
        bool kindOk = e.kind == L"person" || e.kind == L"object" || e.kind == L"exit" || e.kind == L"note" ||
                      e.kind == L"loot";
        if (!kindOk || e.extra == L"тело") continue;
        if (seen[e.kind + L"|" + Lower(e.name)] > 1) continue;
        std::wstring base = e.name.substr(0, e.name.find(L", возле:"));
        if (added.count(Lower(base))) continue;
        std::vector<std::wstring> nw;
        bool longWord = false;
        for (const std::wstring& w : Words(base))
            if (w.size() >= 3) { nw.push_back(w); longWord = longWord || w.size() >= 4; }
        if (nw.empty() || !longWord) continue;
        for (const QW& q : qs) {
            const std::vector<std::wstring>& pool = nw.size() == 1 ? q.names : q.words;
            bool all3 = true;
            for (const std::wstring& w : nw) {
                bool hit = false;
                for (const std::wstring& t : pool)
                    if (WordMatch(w, t)) { hit = true; break; }
                if (!hit) { all3 = false; break; }
            }
            if (!all3) continue;
            added.insert(Lower(base));
            Entry m = e;
            m.origin = e.kind;
            m.kind = L"mention";
            m.tag = q.title;
            add.push_back(m);
            break;
        }
    }
    for (Entry& m : add) all.push_back(m);
}

// Fresh positions every press; the order is rebuilt only when the snapshot is
// old, the player moved, the area or the category changed, so paging is stable.
bool Refresh(void* movie, DWORD now, Player& p, bool force) {
    std::vector<Entry> all;
    double t0 = NowMs();
    if (!ReadWorld(movie, p, all)) {
        LogLine(L"scan: World() gave no player");
        return false;
    }
    double tWorld = NowMs() - t0;
    AddEngineObjects(movie, p, all);
    Dedupe(all);
    LabelDoors(all);
    AddMentions(all);
    g_all = all;
    double total = NowMs() - t0;
    if (total > 30.0) {
        wchar_t b[200];
        swprintf_s(b, L"scan timing: %.0f ms (world %.0f, objects %.0f, names %.0f for %u)", total, tWorld, g_tEnum,
                   g_tNames, (unsigned)g_tAsked);
        LogLine(b);
    }
    std::vector<Entry> fresh;
    for (const Entry& e : all)
        if (InCategory(e)) fresh.push_back(e);
    if (g_cat != kEnemyCat) Fold(p, fresh);
    std::wstring area;
    GfxCall(movie, "DAOAccess.Val", {"Area.Name"}, &area);
    bool stale = force || (int)(now - g_snapAt) >= (int)kStaleMs || (int)(now - g_snapAt) < 0 ||
                 std::hypot(p.x - g_snapX, p.y - g_snapY) > kStaleMove || area != g_area;
    if (area != g_area) {
        LogLine(L"scan area \"" + area + L"\" (was \"" + g_area + L"\")");
        g_area = area;
        g_selKey.clear();
        g_objNames.clear();
        g_unreachable.clear();
        g_dormantSeen.clear();
        g_walking = false;
        g_dumpPending = true;
        g_dumpAt = GetTickCount() + 5000;
    }
    if (stale) {
        std::sort(fresh.begin(), fresh.end(), [&](const Entry& a, const Entry& b) { return Dist(p, a) < Dist(p, b); });
        g_list.swap(fresh);
        g_snapAt = now;
        g_snapX = p.x;
        g_snapY = p.y;
        std::vector<std::wstring> lines{L"scan rebuild " + std::to_wstring(g_list.size()) + L" entries, category " +
                                        kCats[g_cat].name + L", area \"" + area + L"\""};
        for (const Entry& e : g_list)
            lines.push_back(L"  " + e.Key() + L" \"" + e.name + L"\" " + std::to_wstring((int)std::lround(Dist(p, e))) +
                            L" m extra=" + e.extra +
                            (e.act0 || e.act1 ? L" act=" + std::to_wstring(e.act0) + L"/" + std::to_wstring(e.act1) : L""));
        LogLines(lines);
    } else {
        // Same order; update positions, drop vanished entries, append newcomers.
        std::vector<Entry> kept;
        for (const Entry& old : g_list)
            for (const Entry& f : fresh)
                if (f.Key() == old.Key()) { kept.push_back(f); break; }
        for (const Entry& f : fresh) {
            bool known = false;
            for (const Entry& k : kept) known = known || k.Key() == f.Key();
            if (!known) kept.push_back(f);
        }
        g_list.swap(kept);
    }
    g_sel = 0;
    for (size_t i = 0; i < g_list.size(); ++i)
        if (g_list[i].Key() == g_selKey) { g_sel = i; break; }
    return true;
}

void StartWalk(const Player& p, const Entry& e, DWORD now) {
    unsigned id = (unsigned)_wtoi(p.id.c_str());
    bool ok = id != 0 && MoveCreatureTo(id, e.x, e.y, p.x, p.y);
    LogLine(L"walk " + std::wstring(ok ? L"issued" : L"refused") + L" to " + e.Key() + L" \"" + e.name + L"\" " +
            DistText(Dist(p, e)));
    if (!ok) {
        Speak(L"Не удалось отдать приказ", true);
        return;
    }
    Speak(L"Иду: " + e.name + L", " + DistText(Dist(p, e)), true);
    g_useOnArrive = 0;  // set again by the caller when Enter walks first
    g_walking = true;
    g_target = e;
    g_lastX = p.x;
    g_lastY = p.y;
    g_lastMoveAt = now;
    g_walkNextCheck = now + 500;
    g_walkArea = g_area;
    g_walkerId = id;
    g_walkRetries = 0;
}

// The game's own movement keys (KeyBindings.ini RunCharacterForward / Backward /
// Left / Right, _0 and _1; on this PC W, S, Q, E). Pressing one hands control
// back at once (player request 2026-11-18): before, the walk watch saw the stop
// after the player's own steps and repeated the order, pulling him on.
std::vector<int> MoveKeys() {
    static std::vector<int> s_keys;
    static bool s_read = false;
    if (s_read) return s_keys;
    s_read = true;
    wchar_t docs[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docs))) {
        std::wstring path = std::wstring(docs) + L"\\" DOCS_DIR L"\\Settings\\KeyBindings.ini";
        static const wchar_t* const kActions[] = {L"RunCharacterForward", L"RunCharacterBackward", L"RunCharacterLeft",
                                                  L"RunCharacterRight"};
        for (const wchar_t* a : kActions)
            for (int n = 0; n < 2; ++n) {
                wchar_t key[64], val[128] = {};
                swprintf_s(key, L"%s_%d", a, n);
                GetPrivateProfileStringW(L"Key Bindings", key, L"", val, 128, path.c_str());
                std::wstring v = val;
                if (v.find(L"+") != std::wstring::npos) continue;  // with a modifier: skip
                size_t b = v.find(L"Keyboard::Button_");
                if (b == std::wstring::npos) continue;
                std::wstring k = v.substr(b + 17);
                int vk = 0;
                if (k.size() == 1 && ((k[0] >= L'A' && k[0] <= L'Z') || (k[0] >= L'0' && k[0] <= L'9'))) vk = k[0];
                else if (k == L"Up") vk = VK_UP;
                else if (k == L"Down") vk = VK_DOWN;
                else if (k == L"Left") vk = VK_LEFT;
                else if (k == L"Right") vk = VK_RIGHT;
                if (vk) s_keys.push_back(vk);
            }
    }
    if (s_keys.empty()) s_keys = {'W', 'S', 'Q', 'E'};  // the game's defaults on this PC
    std::wstring list;
    for (int k : s_keys) list += L" " + std::to_wstring(k);
    LogLine(L"walk cancel keys:" + list);
    return s_keys;
}

// A movement key while walking: the walk watch lets go (no repeated orders). The
// game's own keyboard movement takes over the character (HYPOTHESIS that it also
// drops the click-to-move path; the log line "walk cancelled" with the positions
// after it shows whether the leader kept going).
bool CancelWalkByKeys() {
    if (!g_walking) return false;
    for (int vk : MoveKeys())
        if (GetAsyncKeyState(vk) & 0x8000) {
            g_walking = false;
            LogLine(L"walk cancelled by movement key " + std::to_wstring(vk) + L", target " + g_target.Key());
            Speak(L"Автоходьба выключена", true);
            return true;
        }
    return false;
}

// While walking: arrival, or a stop short of the target (repeated up to
// kWalkRetries times before it is announced).
void WatchWalk(void* movie, DWORD now) {
    if (!g_walking || (int)(now - g_walkNextCheck) < 0) return;
    g_walkNextCheck = now + 500;
    std::wstring area;
    GfxCall(movie, "DAOAccess.Val", {"Area.Name"}, &area);
    if (!area.empty() && area != g_walkArea) {
        g_walking = false;
        g_useOnArrive = 0;
        LogLine(L"walk ended by area change to \"" + area + L"\"");
        return;
    }
    Player p;
    std::vector<Entry> all;
    if (!ReadWorld(movie, p, all)) return;
    // Follow a moving target (a companion, a person). A folded group follows its
    // nearest member, whose own entry carries the same id.
    for (const Entry& e : all)
        if ((e.kind == g_target.kind && e.id == g_target.id && !e.id.empty()) || e.Key() == g_target.Key()) {
            g_target.x = e.x;
            g_target.y = e.y;
        }
    if (!g_target.id.empty() && (g_target.kind == L"person" || g_target.kind == L"extra" || g_target.kind == L"creature")) {
        RawObject o;
        if (ReadObject((unsigned)_wtoi(g_target.id.c_str()), o) && o.posOk) {
            g_target.x = o.x;
            g_target.y = o.y;
        }
    }
    double d = Dist(p, g_target);
    if (std::hypot(p.x - g_lastX, p.y - g_lastY) > 0.3) {
        g_lastX = p.x;
        g_lastY = p.y;
        g_lastMoveAt = now;
    }
    // Arrived after Enter on a far target: the default action now (talk, open...).
    auto useNow = [&]() {
        unsigned id = g_useOnArrive;
        g_useOnArrive = 0;
        if (!id) return false;
        int action = InteractWith(id, false);
        LogLine(L"use on arrival " + g_target.Key() + L" \"" + g_target.name + L"\" -> action " + std::to_wstring(action));
        if (action <= 0) return false;
        Speak(L"Действие: " + g_target.name, true);
        return true;
    };
    if (d <= kArrive) {
        g_walking = false;
        LogLine(L"walk arrived " + g_target.Key() + L" " + std::to_wstring(d));
        if (!useNow()) Speak(L"Пришли: " + g_target.name, true);
    } else if ((int)(now - g_lastMoveAt) >= (int)kStuckMs) {
        std::wstring dir = Direction(p, g_target);
        if (d <= kNear) {
            g_walking = false;
            LogLine(L"walk arrived near " + g_target.Key() + L" " + std::to_wstring(d));
            if (!useNow())
                Speak(L"Пришли: " + g_target.name + L", " + DistText(d) + (dir.empty() ? L"" : L", " + dir), true);
        } else if (g_walkRetries < kWalkRetries && g_walkerId &&
                   MoveCreatureTo(g_walkerId, g_target.x, g_target.y, p.x, p.y)) {
            ++g_walkRetries;
            g_lastMoveAt = now;
            LogLine(L"walk stopped at " + std::to_wstring(d) + L" m, order repeated (" +
                    std::to_wstring(g_walkRetries) + L")");
        } else {
            g_walking = false;
            g_useOnArrive = 0;
            LogLine(L"walk stopped short of " + g_target.Key() + L" " + std::to_wstring(d));
            Speak(L"Остановился. " + g_target.name + L", " + DistText(d) + (dir.empty() ? L"" : L", " + dir), true);
        }
    }
}

// ---------------------------------------------------------------- quests

bool ReadQuests(void* movie, std::vector<Quest>& out, std::wstring& active) {
    std::wstring txt;
    if (!GfxCall(movie, "DAOAccess.Quests", {}, &txt) || txt.empty()) return false;
    for (const std::wstring& line : SplitW(txt, L'\n')) {
        auto f = SplitW(line, L'\t');
        if (f.size() >= 2 && f[0] == L"active") { active = f[1]; continue; }
        if (f.size() < 6 || f[0].empty()) continue;
        Quest q;
        q.resref = f[0]; q.parent = f[1]; q.name = f[2];
        q.completed = f[3] == L"1" || f[3] == L"true";
        q.updated = f[4] == L"1" || f[4] == L"true";
        q.desc = f[5];
        out.push_back(q);
    }
    return true;
}

std::wstring QuestTitle(const Quest& q) {
    for (const Quest& p : g_quests)
        if (!q.parent.empty() && p.resref == q.parent && !p.name.empty() && p.name != q.name)
            return p.name + L" — " + q.name;
    return q.name;
}

std::wstring QuestText(const Quest& q) {
    std::wstring t = L"Задание «" + QuestTitle(q) + L"»";
    if (!q.desc.empty()) t += L": " + q.desc;
    std::wstring out;
    for (wchar_t c : t)
        if (!(c == L' ' && !out.empty() && out.back() == L' ')) out += c;
    // "Заголовок.. Текст" (a heading that already ended with a dot) -> one dot;
    // an ellipsis "..." stays.
    for (size_t i = 0; i + 1 < out.size(); ++i)
        if (out[i] == L'.' && out[i + 1] == L'.' && (i + 2 >= out.size() || out[i + 2] != L'.') &&
            (i == 0 || out[i - 1] != L'.'))
            out.erase(i, 1);
    return out;
}

// What ties a quest to where the player is: quest names with a marker on this
// map (PlotDestinations tooltips), and the area's module prefix, the first three
// letters of its map note tags ("bhn000mn_your_room" -> "bhn"; quest resrefs
// share it: "bhn100pt_mother"). HYPOTHESIS from the resource naming seen in tests.
struct QuestContext { std::vector<std::wstring> marked; std::wstring prefix; };

QuestContext ReadQuestContext(void* movie) {
    QuestContext c;
    Player pl;
    std::vector<Entry> all;
    ReadWorld(movie, pl, all);
    std::unordered_map<std::wstring, int> prefixes;
    for (const Entry& e : all) {
        if (e.kind == L"quest" && !e.tag.empty()) c.marked.push_back(e.tag);
        if (e.kind == L"note" || (e.kind == L"exit" && e.extra.find(L"ico_mm_exit") != std::wstring::npos)) {
            auto f = SplitW(e.extra, L'|');
            if (f.size() > 1 && f[1].size() > 3) prefixes[f[1].substr(0, 3)]++;
        }
    }
    int best = 0;
    for (auto& kv : prefixes)
        if (kv.second > best) { best = kv.second; c.prefix = kv.first; }
    for (auto& ch : c.prefix) ch = (wchar_t)towlower(ch);
    return c;
}

int Relevance(const Quest& q, const QuestContext& c) {
    int r = 0;
    std::wstring title = QuestTitle(q);
    for (const std::wstring& m : c.marked)
        if (m == q.name || title.find(m) != std::wstring::npos) r += 2;
    std::wstring rr = q.resref;
    for (auto& ch : rr) ch = (wchar_t)towlower(ch);
    if (!c.prefix.empty() && rr.compare(0, c.prefix.size(), c.prefix) == 0) r += 1;
    return r;
}

// Open quests: tied to this place first (marker here, same module), then the
// most recently changed (the journal's "updated" mark and the tracked quest count
// as changes seen before the first poll). Test 17:45: the tracked quest was the
// Shale DLC offer "Сулкеров перевал" while the player was in Castle Cousland.
std::vector<const Quest*> OpenQuests(const QuestContext& c) {
    std::vector<const Quest*> v;
    for (const Quest& q : g_quests)
        if (!q.completed && !q.name.empty()) v.push_back(&q);
    std::stable_sort(v.begin(), v.end(), [&](const Quest* a, const Quest* b) {
        int ra = Relevance(*a, c), rb = Relevance(*b, c);
        if (ra != rb) return ra > rb;
        return a->stamp > b->stamp;
    });
    return v;
}

// Every 2 s while exploring: a new or changed quest text is read out (queued, so
// it follows the game's "Задание обновлено"). The first read after loading or an
// area change is silent.
void PollQuests(void* movie, DWORD now, bool force) {
    if (!force && (int)(now - g_questPollAt) < 2000 && g_questBaseline) return;
    g_questPollAt = now;
    static std::wstring s_area;
    std::wstring area;
    GfxCall(movie, "DAOAccess.Val", {"Area.Name"}, &area);
    if (area != s_area) {  // a load or an area change: take a new silent baseline
        s_area = area;
        g_questBaseline = false;
        g_questSeen.clear();
    }
    std::vector<Quest> fresh;
    std::wstring active;
    if (!ReadQuests(movie, fresh, active)) return;
    std::vector<const Quest*> changed;
    for (Quest& q : fresh) {
        unsigned old = 0;
        for (const Quest& o : g_quests)
            if (o.resref == q.resref) old = o.stamp;
        q.stamp = old;
        if (!g_questBaseline && old == 0) q.stamp = q.updated ? 2 : (q.resref == active ? 1 : 0);
        auto it = g_questSeen.find(q.resref);
        bool isNew = it == g_questSeen.end();
        if (!isNew && it->second == q.desc) continue;
        g_questSeen[q.resref] = q.desc;
        if (!g_questBaseline) continue;
        q.stamp = g_questStampNext++;
        if (!q.completed) changed.push_back(&q);
    }
    g_quests.swap(fresh);
    if (!g_questBaseline) {
        g_questBaseline = true;
        std::vector<std::wstring> lines{L"quests baseline: " + std::to_wstring(g_quests.size()) + L" plots, active " + active};
        for (const Quest& q : g_quests)
            lines.push_back(L"  " + q.resref + L" parent=" + q.parent + L" done=" + (q.completed ? L"1" : L"0") +
                            L" upd=" + (q.updated ? L"1" : L"0") + L" \"" + q.name + L"\"");
        LogLines(lines);
        return;
    }
    for (const Quest* c : changed) {
        // pointers into the old vector are gone after the swap: find by resref
        for (const Quest& q : g_quests)
            if (q.resref == c->resref) {
                LogLine(L"quest changed " + q.resref + L" \"" + QuestText(q) + L"\"");
                Speak(QuestText(q), false);
            }
    }
}

// Delete: the most recent open quest; pressed again within 15 s, the next one.
void SpeakQuestHint(void* movie, DWORD now, const wchar_t* prefix) {
    PollQuests(movie, now, true);
    QuestContext ctx = ReadQuestContext(movie);
    auto open = OpenQuests(ctx);
    if (open.empty()) {
        LogLine(L"quest hint: no open quests");
        Speak(std::wstring(prefix) + L"Открытых заданий нет", true);
        return;
    }
    bool again = g_questKeyAt != 0 && (int)(now - g_questKeyAt) < 15000 && *prefix == 0;
    g_questCycle = again ? (g_questCycle + 1) % open.size() : 0;
    g_questKeyAt = *prefix == 0 ? now : 0;
    const Quest& q = *open[g_questCycle];
    std::wstring say = std::wstring(prefix) + QuestText(q);
    if (open.size() > 1) {
        wchar_t last = say.empty() ? L' ' : say.back();
        say += (last == L'.' || last == L'!' || last == L'?') ? L" " : L". ";
        say += std::to_wstring(g_questCycle + 1) + L" из " + std::to_wstring(open.size());
    }
    std::wstring order;
    for (const Quest* o : open) order += L" " + o->resref + L"=" + std::to_wstring(Relevance(*o, ctx));
    LogLine(L"quest hint prefix " + ctx.prefix + L", order" + order);
    LogLine(L"quest hint " + q.resref + L" \"" + say + L"\"");
    Speak(say, true);
}

// ---------------------------------------------------------------- combat
// Test 17:47: the player pressed Enter about 30 times on one soldier hearing only
// "Атакую" each time. Now the fight is narrated: its start (nearest enemy chosen,
// so Enter attacks it), the target's health in quarters, its death and the next
// nearest enemy, the leader's own health falling, and the end of the fight.
// Health = Attributes.7 Current/Total (portraits.gfx, inventory.gfx); combat =
// GameModeController.CurrentGameMode 1 (GUISystem.GM_COMBAT).

struct CombatState {
    bool in = false;
    int prevCat = 0;
    unsigned target = 0;
    std::wstring targetName;
    int targetBucket = -1, hpBucket = -1;
    DWORD pollAt = 0;
    // Our attack order (Enter) and the target's health since: no change for
    // kNoHarmMs means the target cannot be reached.
    DWORD attackAt = 0, hpChangeAt = 0;
    double lastThp = -1;
} g_cb;
constexpr DWORD kNoHarmMs = 12000, kNoHarmBlindMs = 45000;

struct CombatRead {
    bool ok = false;
    int mode = 0;
    double hp = 0, hpMax = 0, thp = 0, thpMax = 0;
    unsigned ctarget = 0;
};

CombatRead ReadCombat(void* movie, unsigned id) {
    CombatRead r;
    std::wstring txt;
    if (!GfxCall(movie, "DAOAccess.Combat", {(double)id}, &txt)) return r;
    auto f = SplitW(txt, L'\t');
    if (f.size() < 6) return r;
    r.ok = true;
    r.mode = _wtoi(f[0].c_str());
    r.hp = Num(f[1]);
    r.hpMax = Num(f[2]);
    r.ctarget = (unsigned)_wtoi(f[3].c_str());
    r.thp = Num(f[4]);
    r.thpMax = Num(f[5]);
    return r;
}

int Pct(double cur, double max) { return max > 0 ? (int)std::lround(cur * 100.0 / max) : -1; }
// 4 = above 75 %, 3, 2, 1, 0 = none left; -1 = unknown.
int Bucket(double cur, double max) {
    int p = Pct(cur, max);
    if (p < 0) return -1;
    return p <= 0 ? 0 : (int)std::ceil(p / 25.0);
}

std::wstring NameOf(unsigned id) {
    auto it = g_objNames.find(id);
    if (it == g_objNames.end()) return L"цель";
    std::wstring n = it->second;
    size_t bar = n.find(L'|');
    if (bar != std::wstring::npos) n.resize(bar);
    return n.empty() ? L"цель" : n;
}

bool IsCreature(unsigned id) {
    RawObject o;
    return id != 0 && ReadObject(id, o) && o.cls == "CCCreature";
}

// The enemy list within range, nearest first; selects the nearest.
bool NearestEnemies(void* movie, DWORD now, Player& p) {
    g_cat = kEnemyCat;
    g_selKey.clear();
    if (!Refresh(movie, now, p, true)) return false;
    while (!g_list.empty() && Dist(p, g_list.back()) > kEnemyRange) g_list.pop_back();
    g_sel = 0;
    if (!g_list.empty()) g_selKey = g_list[0].Key();
    return true;
}

std::wstring EnemyBrief(const Player& p, const Entry& e) {
    std::wstring dir = Direction(p, e);
    return e.name + L", " + DistText(Dist(p, e)) + (dir.empty() ? L"" : L", " + dir);
}

void SetTarget(unsigned id, const std::wstring& name) {
    g_cb.target = id;
    g_cb.targetName = name.empty() ? NameOf(id) : name;
    g_cb.targetBucket = -1;
    g_cb.lastThp = -1;
    g_cb.hpChangeAt = GetTickCount();
}

std::wstring SelectedName();  // the controlled character's name (party section)

void CombatTick(void* movie, DWORD now) {
    if ((int)(now - g_cb.pollAt) < 400) return;
    g_cb.pollAt = now;
    CombatRead r = ReadCombat(movie, g_cb.target);
    if (!r.ok) return;
    bool in = r.mode == 1;
    if (in && !g_cb.in) {
        g_cb.in = true;
        g_cb.prevCat = g_cat;
        g_cb.target = 0;
        g_cb.attackAt = 0;
        g_cb.hpBucket = Bucket(r.hp, r.hpMax);
        g_walking = false;
        Player p;
        std::wstring say = L"Бой!";
        if (NearestEnemies(movie, now, p) && !g_list.empty())
            say += L" Врагов рядом: " + std::to_wstring(g_list.size()) + L". Ближайший: " + EnemyBrief(p, g_list[0]);
        LogLine(L"combat start: " + say);
        Speak(say, true);
        return;
    }
    if (!in && g_cb.in) {
        g_cb.in = false;
        g_cat = g_cb.prevCat;
        g_selKey.clear();
        g_cb.target = 0;
        g_cb.attackAt = 0;
        std::wstring say = L"Бой окончен";
        int hp = Pct(r.hp, r.hpMax);
        if (hp >= 0) say += L". Здоровье " + std::to_wstring(hp) + L"%";
        LogLine(L"combat end: " + say);
        Speak(say, true);
        return;
    }
    if (!in) return;
    // The game's own target for the leader, when it reports one.
    if (r.ctarget != 0 && r.ctarget != g_cb.target && IsCreature(r.ctarget)) {
        SetTarget(r.ctarget, L"");
        LogLine(L"combat target from game " + std::to_wstring(r.ctarget) + L" " + g_cb.targetName);
        return;
    }
    int hb = Bucket(r.hp, r.hpMax);
    if (hb >= 0) {
        if (g_cb.hpBucket >= 0 && hb < g_cb.hpBucket) {
            std::wstring say;
            if (hb == 0) {
                // Test 20:58: "Моё здоровье 0%, опасно!" when the hero fell; the game
                // then hands control to a companion (spoken by the party watch).
                std::wstring who = SelectedName();
                say = (who.empty() ? std::wstring(L"Вы") : who) + L" без сознания";
            } else {
                say = L"Моё здоровье " + std::to_wstring(Pct(r.hp, r.hpMax)) + L"%";
                if (hb <= 1) say += L", опасно!";
            }
            LogLine(L"combat " + say);
            Speak(say, hb <= 1);
        }
        g_cb.hpBucket = hb;
    }
    if (!g_cb.target) return;
    // An attack that harms nothing (test 20:58: 53 attacks on a soldier whose health
    // never showed): say so and offer the next enemy.
    if (r.thp != g_cb.lastThp) {
        g_cb.lastThp = r.thp;
        g_cb.hpChangeAt = now;
    }
    // With readable health: no change for kNoHarmMs. Without it (many darkspawn,
    // test 23:32) damage cannot be seen, so only a target still standing after
    // kNoHarmBlindMs of our attacks counts.
    bool readable = r.thpMax > 0;
    bool noHarm = g_cb.attackAt &&
                  (readable ? (int)(now - g_cb.attackAt) >= (int)kNoHarmMs && (int)(now - g_cb.hpChangeAt) >= (int)kNoHarmMs
                            : (int)(now - g_cb.attackAt) >= (int)kNoHarmBlindMs);
    if (noHarm) {
        g_cb.attackAt = 0;
        std::wstring say = L"Не получается достать: " + g_cb.targetName + L".";
        unsigned stuck = g_cb.target;
        g_unreachable.insert(stuck);
        Player p;
        if (NearestEnemies(movie, now, p)) {
            for (size_t i = 0; i < g_list.size(); ++i)
                if ((unsigned)_wtoi(g_list[i].id.c_str()) != stuck) {
                    g_sel = i;
                    g_selKey = g_list[i].Key();
                    say += L" Следующий: " + EnemyBrief(p, g_list[i]);
                    break;
                }
        }
        LogLine(L"combat no harm to " + std::to_wstring(stuck) + L": " + say);
        Speak(say, true);
        return;
    }
    RawObject o;
    bool gone = !ReadObject(g_cb.target, o) || o.dead;
    if (gone) {
        std::wstring say = L"Убит: " + g_cb.targetName + L".";
        g_cb.target = 0;
        g_cb.attackAt = 0;
        Player p;
        if (NearestEnemies(movie, now, p) && !g_list.empty())
            say += L" Следующий: " + EnemyBrief(p, g_list[0]);
        else
            say += L" Врагов рядом нет";
        LogLine(L"combat kill: " + say);
        Speak(say, true);
        return;
    }
    int tb = Bucket(r.thp, r.thpMax);
    if (tb > 0) {
        if (g_cb.targetBucket >= 0 && tb < g_cb.targetBucket) {
            std::wstring say = g_cb.targetName + L": " + std::to_wstring(Pct(r.thp, r.thpMax)) + L"%";
            LogLine(L"combat target " + say);
            Speak(say, false);
        }
        g_cb.targetBucket = tb;
    }
}

// Home during a fight: own health, the target, how many enemies are close.
void SpeakCombatStatus(void* movie, DWORD now) {
    CombatRead r = ReadCombat(movie, g_cb.target);
    std::wstring say;
    int hp = Pct(r.hp, r.hpMax);
    if (hp >= 0) say = L"Моё здоровье " + std::to_wstring(hp) + L"%. ";
    Player p;
    std::vector<Entry> all;
    ReadWorld(movie, p, all);
    if (g_cb.target) {
        say += L"Цель: " + g_cb.targetName;
        int tp = Pct(r.thp, r.thpMax);
        if (tp >= 0) say += L", " + std::to_wstring(tp) + L"%";
        RawObject o;
        if (ReadObject(g_cb.target, o)) {
            Entry e;
            e.x = o.x;
            e.y = o.y;
            std::wstring dir = Direction(p, e);
            say += L", " + DistText(Dist(p, e)) + (dir.empty() ? L"" : L", " + dir);
        }
        say += L". ";
    } else {
        say += L"Цели нет. ";
    }
    size_t n = 0;
    if (g_cat == kEnemyCat)
        for (const Entry& e : g_list)
            if (Dist(p, e) <= kEnemyRange) ++n;
    say += L"Врагов рядом: " + std::to_wstring(n);
    LogLine(L"combat status: " + say);
    Speak(say, true);
}

// ---------------------------------------------------------------- quickbar
// The game's own keys 1..0 use quickbar slots 0..9 (proven with Ctrl+0 -> slot
// index 9, test 03:54). Polled every 500 ms in the world: an ability whose
// cooldown ends is announced ("Готово: ..."), a sustained one switching on or
// off, and one waiting for a target. Backspace lists the slots.

struct Slot {
    std::wstring value, name, stack;
    double cd = 0;
    bool on = false, pending = false, error = false, warning = false;
};
Slot g_slots[10];
bool g_slotsRead = false;
DWORD g_slotsPollAt = 0;
KeyState g_slotKeys[10], g_listKey;

std::wstring SlotKeyName(int i) { return std::to_wstring(i == 9 ? 0 : i + 1); }

void ReadSlotNames(void* movie) {
    std::wstring txt;
    if (!GfxCall(movie, "DAOAccess.QuickNames", {}, &txt)) return;
    auto lines = SplitW(txt, L'\n');
    for (int i = 0; i < 10 && i < (int)lines.size(); ++i) {
        auto f = SplitW(lines[i], L'\t');
        if (f.size() < 5) continue;
        g_slots[i].name = f[0];
        g_slots[i].stack = f[1];
        g_slots[i].error = f[2] == L"1" || f[2] == L"true";
        g_slots[i].warning = f[3] == L"1" || f[3] == L"true";
    }
}

bool SlotEmpty(const Slot& s) { return s.value.empty() || s.value == L"0"; }

std::wstring SlotState(const Slot& s) {
    std::wstring t;
    int n = _wtoi(s.stack.c_str());
    if (n > 0) t += L", " + std::to_wstring(n) + L" шт.";
    if (s.on) t += L", включено";
    if (s.cd > 0.001) t += L", перезарядка " + std::to_wstring((int)std::lround(s.cd * 100)) + L"%";
    else if (s.error) t += L", недоступно";
    else if (s.warning) t += L", не выполнены условия";
    else t += L", готово";
    return t;
}

void QuickbarTick(void* movie, DWORD now) {
    if ((int)(now - g_slotsPollAt) < 500) return;
    g_slotsPollAt = now;
    std::wstring txt;
    if (!GfxCall(movie, "DAOAccess.QuickState", {}, &txt) || txt.empty()) return;
    auto lines = SplitW(txt, L'\n');
    Slot old[10];
    for (int i = 0; i < 10; ++i) old[i] = g_slots[i];
    bool namesStale = !g_slotsRead;
    for (int i = 0; i < 10 && i < (int)lines.size(); ++i) {
        auto f = SplitW(lines[i], L'\t');
        if (f.size() < 4) continue;
        Slot& s = g_slots[i];
        if (s.value != f[0]) namesStale = true;
        s.value = f[0];
        s.cd = Num(f[1]);
        s.on = f[2] == L"1";
        s.pending = f[3] == L"1" || f[3] == L"true";
    }
    if (namesStale) ReadSlotNames(movie);
    if (!g_slotsRead) {  // first read: silent
        g_slotsRead = true;
        std::vector<std::wstring> log{L"quickbar first read"};
        for (int i = 0; i < 10; ++i)
            log.push_back(L"  " + SlotKeyName(i) + L" value=" + g_slots[i].value + L" \"" + g_slots[i].name + L"\"" +
                          SlotState(g_slots[i]));
        LogLines(log);
        return;
    }
    for (int i = 0; i < 10; ++i) {
        const Slot& s = g_slots[i];
        const Slot& o = old[i];
        if (SlotEmpty(s) || s.value != o.value) continue;
        std::wstring say;
        if (o.cd > 0.001 && s.cd <= 0.001) say = L"Готово: " + s.name;
        else if (s.on != o.on) say = (s.on ? L"Включено: " : L"Выключено: ") + s.name;
        else if (s.pending && !o.pending) say = s.name + L": выберите цель, Enter на враге";
        if (say.empty()) continue;
        LogLine(L"quickbar " + SlotKeyName(i) + L" \"" + say + L"\"");
        Speak(say, false);
    }
}

// The game's key 1..0 was pressed: say what it does (or why nothing happens).
void SlotKeyPressed(void* movie, int i) {
    ReadSlotNames(movie);
    const Slot& s = g_slots[i];
    std::wstring say;
    if (SlotEmpty(s)) say = L"Ячейка " + SlotKeyName(i) + L" пуста";
    else if (s.cd > 0.001) say = s.name + L": перезарядка, " + std::to_wstring((int)std::lround(s.cd * 100)) + L"%";
    else if (s.error) say = s.name + L": недоступно";
    else say = s.name;
    LogLine(L"quickbar key " + SlotKeyName(i) + L" \"" + say + L"\"");
    Speak(say, true);
}

// Backspace: every filled slot with its key and state.
void SpeakQuickbar(void* movie) {
    ReadSlotNames(movie);
    std::wstring say;
    int n = 0;
    for (int i = 0; i < 10; ++i) {
        const Slot& s = g_slots[i];
        if (SlotEmpty(s)) continue;
        say += SlotKeyName(i) + L": " + (s.name.empty() ? L"без имени" : s.name) + SlotState(s) + L". ";
        ++n;
    }
    if (n == 0) say = L"Панель быстрого доступа пуста";
    LogLine(L"quickbar list \"" + say + L"\"");
    Speak(say, true);
}

// ---------------------------------------------------------------- party
// F1..F4 select a party member (Shift adds to the selection; KeyBindings.ini
// SelectPartyMemberN / ...Inclusive). The game shows it only on the portraits,
// so the change is spoken: "Тэйрна Элеанора, здоровье 100%", or the selected
// group. In a fight a companion's health is spoken when it falls below half.

struct Member {
    std::wstring id, name;
    bool sel = false;
    double hp = 0, hpMax = 0;
    int bucket = -1;
    std::wstring injuries;  // names joined by "; " (agent A.Injuries), empty = none
};
std::vector<Member> g_party;
std::wstring g_selId, g_leaderId;
int g_selCount = -1;
bool g_partyRead = false, g_partyForce = false;
DWORD g_partyPollAt = 0, g_partyForceAt = 0;
KeyState g_fKeys[4], g_partyKey;
// Party.HoldPositions (the game's H, ToggleHoldParty in KeyBindings.ini): the
// companions stay where they are instead of following. Test 23:44: Джори stood
// 121 m away for 15 s while the player stood still. HYPOTHESIS: 1 = holding (the
// F10 dump of 02:19 read 0 while the party followed).
std::wstring g_holdRaw, g_hold;  // last raw value read / last value acted on
KeyState g_gatherKey;

std::wstring SelectedName() {
    for (const Member& m : g_party)
        if (m.id == g_selId) return m.name;
    return L"";
}

std::wstring MemberHp(const Member& m) {
    int p = Pct(m.hp, m.hpMax);
    return p < 0 ? L"" : L", здоровье " + std::to_wstring(p) + L"%";
}

bool ReadParty(void* movie, std::wstring& selId, std::wstring& selName, int& count, std::wstring& leader,
               std::vector<Member>& out) {
    std::wstring txt;
    if (!GfxCall(movie, "DAOAccess.Party", {}, &txt) || txt.empty()) return false;
    auto lines = SplitW(txt, L'\n');
    auto h = SplitW(lines[0], L'\t');
    if (h.size() < 4) return false;
    g_holdRaw = h.size() > 4 ? h[4] : L"";
    selId = h[0];
    selName = h[1];
    count = _wtoi(h[2].c_str());
    leader = h[3];
    for (size_t i = 1; i < lines.size(); ++i) {
        auto f = SplitW(lines[i], L'\t');
        if (f.size() < 5 || f[0].empty() || f[0] == L"0") continue;
        Member m;
        m.id = f[0];
        m.name = f[1];
        m.sel = f[2] == L"1" || f[2] == L"true";
        m.hp = Num(f[3]);
        m.hpMax = Num(f[4]);
        m.bucket = Bucket(m.hp, m.hpMax);
        if (f.size() > 5) m.injuries = f[5];
        out.push_back(m);
    }
    return true;
}

void PartyTick(void* movie, DWORD now) {
    if (g_partyForce) {
        if ((int)(now - g_partyForceAt) < 250) return;  // let the game apply the key first
    } else if ((int)(now - g_partyPollAt) < 400) {
        return;
    }
    g_partyPollAt = now;
    std::wstring selId, selName, leader;
    int count = 0;
    std::vector<Member> fresh;
    if (!ReadParty(movie, selId, selName, count, leader, fresh)) return;
    // Companions' health in a fight: below half, at each quarter lost.
    for (const Member& m : fresh) {
        for (const Member& o : g_party) {
            if (o.id != m.id || !g_cb.in || m.id == selId || o.bucket < 0 || m.bucket < 0) continue;
            if (m.bucket < o.bucket && m.bucket <= 2) {
                std::wstring say = m.bucket == 0 ? m.name + L" без сознания" : m.name + MemberHp(m);
                LogLine(L"party health \"" + say + L"\"");
                Speak(say, false);
            }
        }
    }
    // Injuries (player request 2026-11-18): a new one is said when it comes, the
    // healing when the last one goes; after a load, the injured are listed once.
    {
        std::wstring news;
        for (const Member& m : fresh) {
            const Member* old = nullptr;
            for (const Member& o : g_party)
                if (o.id == m.id) old = &o;
            if (!g_partyRead || !old) {
                if (!g_partyRead && !m.injuries.empty()) news += m.name + L": " + m.injuries + L". ";
                continue;
            }
            if (m.injuries == old->injuries) continue;
            if (m.injuries.empty()) { news += m.name + L": травмы вылечены. "; continue; }
            std::wstring added;
            for (const std::wstring& t : SplitW(m.injuries, L';')) {
                std::wstring n = (!t.empty() && t[0] == L' ') ? t.substr(1) : t;
                if (old->injuries.find(n) == std::wstring::npos) added += (added.empty() ? L"" : L", ") + n;
            }
            if (!added.empty()) news += m.name + L" получает травму: " + added + L". ";
            else news += m.name + L": одна из травм вылечена, остались: " + m.injuries + L". ";
        }
        if (!news.empty()) {
            if (!g_partyRead) news = L"Травмы. " + news;
            LogLine(L"party injuries \"" + news + L"\"");
            Speak(news, false);
        }
    }
    bool changed = g_partyRead && (selId != g_selId || count != g_selCount);
    if (g_partyRead && (changed || g_partyForce)) {
        std::vector<std::wstring> names;
        std::wstring say;
        for (const Member& m : fresh)
            if (m.sel) names.push_back(m.name);
        if (names.size() > 1) {
            if (names.size() == fresh.size()) say = L"Выбран весь отряд";
            else {
                say = L"Выбраны: ";
                for (size_t i = 0; i < names.size(); ++i) say += (i ? L", " : L"") + names[i];
            }
            say += L". Управление: " + selName;
        } else {
            say = selName;
            for (const Member& m : fresh)
                if (m.id == selId) say += MemberHp(m);
        }
        LogLine(L"party select id " + selId + L" leader " + leader + L" count " + std::to_wstring(count) + L" \"" +
                say + L"\"");
        Speak(say, true);
        if (selId != g_selId) {
            g_slotsRead = false;    // the quickbar is now this character's: silent re-read
            g_cb.hpBucket = -1;     // "Моё здоровье" now follows this character
            g_walking = false;
        }
    }
    if (!g_partyRead)
        LogLine(L"party first read: selected " + selId + L" " + selName + L", leader " + leader + L", hold \"" +
                g_holdRaw + L"\"");
    // Hold position: said when it changes, and once after loading when it is on.
    if (!g_holdRaw.empty() && g_holdRaw != g_hold) {
        bool on = g_holdRaw != L"0";
        if (!g_hold.empty() || on) {
            std::wstring say = on ? L"Отряд держит позицию и не идёт за вами. Клавиша H отменяет"
                                  : L"Отряд снова следует за вами";
            LogLine(L"party hold " + g_holdRaw + L" \"" + say + L"\"");
            Speak(say, false);
        }
        g_hold = g_holdRaw;
    }
    g_partyRead = true;
    g_partyForce = false;
    g_selId = selId;
    g_selCount = count;
    g_leaderId = leader;
    g_party.swap(fresh);
}

// Shift+Home: everyone's health.
void SpeakParty(void* movie) {
    std::wstring selId, selName, leader;
    int count = 0;
    std::vector<Member> list;
    if (!ReadParty(movie, selId, selName, count, leader, list) || list.empty()) {
        Speak(L"Отряд недоступен", true);
        return;
    }
    RawObject me;
    bool meOk = ReadObject((unsigned)_wtoi(selId.c_str()), me);
    std::wstring say = L"Отряд: ";
    for (size_t i = 0; i < list.size(); ++i) {
        say += (i ? L"; " : L"") + list[i].name + MemberHp(list[i]);
        if (!list[i].injuries.empty()) say += L", травмы: " + list[i].injuries;
        RawObject o;
        if (list[i].id == selId) say += L", управление";
        else if (meOk && ReadObject((unsigned)_wtoi(list[i].id.c_str()), o))
            say += L", " + DistText(std::hypot(o.x - me.x, o.y - me.y));
    }
    if (g_hold != L"" && g_hold != L"0") say += L". Отряд держит позицию";
    LogLine(L"party list \"" + say + L"\"");
    Speak(say, true);
}

// Shift+End: every companion farther than 4 m walks to the controlled character
// (the same move order as End, one per companion, spread around the leader).
void GatherParty(void* movie) {
    std::wstring selId, selName, leader;
    int count = 0;
    std::vector<Member> list;
    RawObject me;
    if (!ReadParty(movie, selId, selName, count, leader, list) || !ReadObject((unsigned)_wtoi(selId.c_str()), me)) {
        Speak(L"Отряд недоступен", true);
        return;
    }
    std::wstring called;
    int k = 0;
    for (const Member& m : list) {
        if (m.id == selId) continue;
        RawObject o;
        unsigned id = (unsigned)_wtoi(m.id.c_str());
        if (!ReadObject(id, o) || o.dead) continue;
        double d = std::hypot(o.x - me.x, o.y - me.y);
        if (d <= 4.0) continue;
        double a = 2.0944 * k++ + 2.5;  // 120 degrees apart, behind-ish
        bool ok = MoveCreatureTo(id, me.x + 2.0 * std::cos(a), me.y + 2.0 * std::sin(a), o.x, o.y);
        LogLine(L"gather " + m.name + L" " + std::to_wstring(d) + L" m -> " + (ok ? L"ordered" : L"refused"));
        if (ok) called += (called.empty() ? L"" : L", ") + m.name + L" " + DistText(d);
    }
    std::wstring say = called.empty() ? L"Отряд уже рядом" : L"Ко мне: " + called;
    if (g_hold != L"" && g_hold != L"0") say += L". Отряд держит позицию, клавиша H отменяет";
    Speak(say, true);
}

// Shift+Delete: how each companion feels about the hero, from the game's own
// approval list (Party.Approval.N.{ObjectID, Approval, Desc}: inventory.gfx
// CreatureUtils.GetApprovalInfo, the bar's tooltip "Одобрение: +12 (Тёплое)").
// HYPOTHESIS: the list holds every companion met, not only the active party; the
// log line "approval ..." shows which.
void SpeakApproval(void* movie) {
    std::wstring txt;
    if (!GfxCall(movie, "DAOAccess.Approval", {}, &txt)) {
        Speak(L"Одобрение недоступно", true);
        return;
    }
    LogLine(L"approval " + txt);
    std::wstring say;
    for (const std::wstring& line : SplitW(txt, L'\n')) {
        auto f = SplitW(line, L'\t');
        if (f.size() < 4 || f[0].empty()) continue;
        int r = _wtoi(f[2].c_str());
        std::wstring name = f[1].empty() ? L"спутник " + f[0] : f[1];
        say += name + L": " + (r > 0 ? L"+" : L"") + std::to_wstring(r) + (f[3].empty() ? L"" : L", " + f[3]) + L". ";
    }
    Speak(say.empty() ? L"Спутников с одобрением нет" : L"Одобрение. " + say, true);
}

}  // namespace

void ScannerTick(void* movie, DWORD now, bool menuOpen) {
    HWND fg = GetForegroundWindow();
    DWORD pid = 0;
    if (fg) GetWindowThreadProcessId(fg, &pid);
    bool usable = pid == GetCurrentProcessId() && !menuOpen;

    bool next = Pressed(g_next, VK_NEXT), prev = Pressed(g_prev, VK_PRIOR);
    bool rep = Pressed(g_repeat, VK_HOME), walk = Pressed(g_walk, VK_END), use = Pressed(g_use, VK_RETURN);
    bool enemyNext = Pressed(g_enemyNext, VK_OEM_6), enemyPrev = Pressed(g_enemyPrev, VK_OEM_4);
    bool questKey = Pressed(g_questKey, VK_DELETE);
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

    if (menuOpen) g_walking = false;  // a conversation or menu ends the watch quietly
    else {
        if (usable) CancelWalkByKeys();
        WatchWalk(movie, now);
        PollQuests(movie, now, false);
        CombatTick(movie, now);
        QuickbarTick(movie, now);
        PartyTick(movie, now);
    }
    for (int f = 0; f < 4; ++f)
        if (Pressed(g_fKeys[f], VK_F1 + f) && usable) {
            g_partyForce = true;  // say the selection even when it did not change
            g_partyForceAt = now;
        }
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    // Quickbar keys: the game's own 1..0 (Ctrl+digit belongs to the menu reader)
    // and Backspace for the list.
    int slotKey = -1;
    for (int d = 0; d < 10; ++d)
        if (Pressed(g_slotKeys[d], d == 9 ? '0' : '1' + d) && !ctrl) slotKey = d;
    bool listKey = Pressed(g_listKey, VK_BACK);
    if (usable && slotKey >= 0) { SlotKeyPressed(movie, slotKey); return; }
    if (usable && listKey) { SpeakQuickbar(movie); return; }

    if (!usable || !(next || prev || rep || walk || use || enemyNext || enemyPrev || questKey)) return;

    if (questKey && shiftDown) {
        SpeakApproval(movie);
        return;
    }
    if (questKey) {
        SpeakQuestHint(movie, now, L"");
        return;
    }
    if (walk && shiftDown) {
        GatherParty(movie);
        return;
    }

    // ] / [ : nearest live enemies, re-sorted on every press (they move).
    if (enemyNext || enemyPrev) {
        bool switching = g_cat != kEnemyCat;
        g_cat = kEnemyCat;
        if (switching) g_selKey.clear();
        std::wstring keep = g_selKey;
        Player p;
        if (!Refresh(movie, now, p, true)) { Speak(L"Сканер недоступен", true); return; }
        // Refresh(force) sorted by distance: keep the close ones only.
        size_t total = g_list.size();
        while (!g_list.empty() && Dist(p, g_list.back()) > kEnemyRange) g_list.pop_back();
        if (g_list.empty()) {
            LogLine(L"scan enemies: none within range, " + std::to_wstring(total) + L" in the area");
            Speak(L"Врагов поблизости нет", true);
            g_selKey.clear();
            return;
        }
        size_t n = g_list.size(), cur = n;
        for (size_t i = 0; i < n; ++i)
            if (g_list[i].Key() == keep) cur = i;
        if (cur == n) g_sel = enemyNext ? 0 : n - 1;  // nothing chosen yet, or it died
        else g_sel = enemyNext ? (cur + 1) % n : (cur + n - 1) % n;
        SpeakEntry(p, enemyNext ? L"enemy-next" : L"enemy-prev");
        return;
    }

    if (ctrl && (next || prev)) {
        g_cat = (g_cat + (next ? 1 : kCatCount - 1)) % kCatCount;
        g_selKey.clear();
        Player p;
        if (!Refresh(movie, now, p, true)) { Speak(L"Сканер недоступен", true); return; }
        LogLine(L"scan category " + std::wstring(kCats[g_cat].name) + L" " + std::to_wstring(g_list.size()));
        if (g_cat == kQuestCat && g_list.empty()) {
            // No marker on the map: say what the journal asks instead.
            SpeakQuestHint(movie, now, L"Задания: отметок на карте нет. ");
            return;
        }
        Speak(std::wstring(kCats[g_cat].name) + L", " + std::to_wstring(g_list.size()), true);
        return;
    }

    Player p;
    if (!Refresh(movie, now, p, false)) {
        Speak(L"Сканер недоступен", true);
        return;
    }
    if (g_list.empty() && g_cat == kQuestCat) {
        SpeakQuestHint(movie, now, L"Отметок на карте нет. ");
        return;
    }
    if (g_list.empty()) { SpeakEntry(p, L"empty"); return; }
    size_t n = g_list.size();
    if (next) {
        if (!g_selKey.empty()) g_sel = (g_sel + 1) % n;
        SpeakEntry(p, L"next");
    } else if (prev) {
        g_sel = g_selKey.empty() ? n - 1 : (g_sel + n - 1) % n;
        SpeakEntry(p, L"prev");
    } else if (rep) {
        if (shiftDown) SpeakParty(movie);
        else if (g_cb.in) SpeakCombatStatus(movie, now);
        else SpeakEntry(p, L"repeat");
    } else if (walk) {
        if (g_selKey.empty()) { Speak(L"Сначала выберите объект клавишами PageUp и PageDown", true); return; }
        StartWalk(p, g_list[g_sel], now);
    } else if (use) {
        // The game's own default action (talk, open, use, pick up, attack), walking
        // there first, as a click on the object. Map notes are places: walk.
        if (g_selKey.empty()) { Speak(L"Сначала выберите объект клавишами PageUp и PageDown", true); return; }
        const Entry& e = g_list[g_sel];
        unsigned id = (unsigned)_wtoi(e.id.c_str());
        if (e.kind == L"note" || e.origin == L"note") {
            // A place on the map is only a point; when a door or an exit stands within
            // 10 m of it (a building's entrance), Enter uses that instead.
            const Entry* door = nullptr;
            double best = 10.0;
            for (const Entry& o : g_all) {
                if (o.kind != L"exit" && o.kind != L"object") continue;
                if (o.extra.find(L"ico_mm_") != std::wstring::npos || o.extra == L"тело") continue;
                std::wstring low = Lower(o.name + L" " + o.tag);
                bool doorish = o.kind == L"exit" || low.find(L"двер") != std::wstring::npos ||
                               low.find(L"ворот") != std::wstring::npos || low.find(L"door") != std::wstring::npos ||
                               low.find(L"gate") != std::wstring::npos || low.find(L"trans") != std::wstring::npos;
                double d = std::hypot(o.x - e.x, o.y - e.y);
                if (doorish && d < best) { best = d; door = &o; }
            }
            if (!door) { StartWalk(p, e, now); return; }
            id = (unsigned)_wtoi(door->id.c_str());
            LogLine(L"note " + e.Key() + L" uses nearby " + door->Key() + L" \"" + door->name + L"\" " +
                    std::to_wstring(best) + L" m");
            Speak(L"Вход: " + door->name, true);
        }
        if (id == 0) { StartWalk(p, e, now); return; }
        std::wstring inCombat;
        GfxCall(movie, "DAOAccess.Val", {"GUI.SelectedChar.InCombat"}, &inCombat);
        if (inCombat.empty()) GfxCall(movie, "DAOAccess.Val", {"Party.Leader.InCombat"}, &inCombat);
        bool fighting = inCombat == L"1" || g_cb.in;
        // Far away and not fighting: walk there, act on arrival (kUseFar).
        if (!fighting && Dist(p, e) > kUseFar) {
            LogLine(L"use " + e.Key() + L" \"" + e.name + L"\" far (" + DistText(Dist(p, e)) + L"): walk first");
            StartWalk(p, e, now);
            if (g_walking) g_useOnArrive = id;
            return;
        }
        // In a fight the combat mode gives the order (DA2: the catalogue has no
        // InCombat field, so every attack of test 19:55 went through explore mode).
        int action = InteractWith(id, fighting);
        LogLine(L"use " + e.Key() + L" \"" + e.name + L"\" -> action " + std::to_wstring(action));
        // An ability waiting for its target (test 19:23: Enter on the enemy after
        // key 2 used Удар щитом, its cooldown started).
        int pend = -1;
        for (int i = 0; i < 10; ++i)
            if (g_slots[i].pending && !SlotEmpty(g_slots[i])) pend = i;
        if (action == 4) {
            // The "no harm" clock starts with the first order on this target; the
            // player presses Enter every few seconds, which must not restart it.
            bool newTarget = id != g_cb.target;
            if (newTarget) SetTarget(id, e.name);
            if (newTarget || !g_cb.attackAt) g_cb.attackAt = now;
            CombatRead r = ReadCombat(movie, id);
            int tp = Pct(r.thp, r.thpMax);
            g_cb.targetBucket = Bucket(r.thp, r.thpMax);
            // DA2 fights are fast: the player presses Enter for every blow (test
            // 19:55: up to 25 presses on one enemy, each said "Атакую"). Only a
            // new target is spoken; further blows are silent, the target's health
            // is still told in quarters by CombatTick, its death by the kill line.
            bool again = !newTarget && pend < 0;
            if (pend >= 0) {
                Speak(g_slots[pend].name + L": " + e.name + (tp >= 0 ? L", " + std::to_wstring(tp) + L"%" : L""), true);
                g_slots[pend].pending = false;
            } else if (!again) {
                Speak(L"Атакую: " + e.name + (tp >= 0 ? L", " + std::to_wstring(tp) + L"%" : L""), true);
            }
            g_walking = false;
        } else if (action > 0) {
                Speak(L"Действие: " + e.name, true);
            g_walking = false;
        } else if (action == 0) {
            Speak(L"Нет действия. Иду к объекту", true);
            StartWalk(p, e, now);
        } else {
            Speak(L"Не удалось. Иду к объекту", true);
            StartWalk(p, e, now);
        }
    }
}

void ScannerDumpObjects(void* movie) {
    std::vector<RawObject> objs;
    DWORD t0 = GetTickCount();
    EnumObjects(objs, kMaxObjectId);
    DWORD t1 = GetTickCount();
    std::unordered_map<std::string, int> counts;
    for (const RawObject& o : objs) counts[o.cls + "/" + std::to_string(o.type)]++;
    std::vector<std::wstring> lines{L"objects: " + std::to_wstring(objs.size()) + L" in " + std::to_wstring(t1 - t0) +
                                    L" ms; explore mode " + (ExploreMode() ? L"captured" : L"not captured") +
                                    L", combat mode " + (CombatMode() ? L"captured" : L"not captured")};
    for (auto& c : counts) lines.push_back(L"  class " + Widen(c.first.c_str()) + L" x" + std::to_wstring(c.second));
    std::wstring ask;
    size_t shown = 0;
    for (const RawObject& o : objs) {
        if (shown++ >= 150) break;
        ask += (ask.empty() ? L"" : L",") + std::to_wstring(o.id);
    }
    std::wstring txt;
    GfxCall(movie, "DAOAccess.Names", {Utf8(ask)}, &txt);
    auto names = SplitW(txt, L'\n');
    for (size_t i = 0; i < objs.size() && i < 150; ++i) {
        const RawObject& o = objs[i];
        wchar_t b[200];
        swprintf_s(b, L"  id %u %S type %u pos %.2f %.2f %.2f usable %d f7f0 %d talk %d | ", o.id, o.cls.c_str(),
                   o.type, o.x, o.y, o.z, (int)o.usable, o.flag7f0, (int)o.talk);
        lines.push_back(std::wstring(b) + L"mem name \"" + o.name + L"\" tag \"" + o.tag + L"\" | catalogue " +
                        (i < names.size() ? names[i] : std::wstring()));
    }
    LogLines(lines);
}
