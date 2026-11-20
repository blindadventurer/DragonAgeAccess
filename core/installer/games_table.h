// Both games' settings for the one installer, taken from games\<game>\game.h so the
// names stay in one place. Each game.h is included in its own namespace and its
// macros are undefined afterwards (both define the same macro names).
#pragma once
#include <string>
#include <vector>

struct GameBuild { unsigned long ts, size; std::wstring name; };
struct GameReg { std::wstring key, value; };
struct GameInfo {
    std::wstring key;       // "dao", "da2" (/gamekey: on the command line)
    std::wstring title;     // "Dragon Age: Origins"
    std::wstring modTitle;  // "DAO Access"
    std::wstring mod;       // "DAOAccess": scripts\<mod>.asi, release asset <mod>-<version>.zip
    std::wstring exe;       // inside bin_ship
    std::wstring docs;      // inside Documents
    std::wstring agent;     // the Flash agent file in the override folder
    std::wstring start;     // what the mod says at start
    std::vector<std::wstring> processes;
    std::vector<GameBuild> builds;
    std::vector<GameReg> regs;
    std::string unwrapFrom, unwrapResult;  // SHA-256 of the Steam exe and of its 4 GB copy
    std::wstring ruIni, ruPage;            // Russian text: settings file in Documents\<docs>, translation page
    std::vector<std::wstring> ruFiles;     // talk tables copied into the install folder
};

#define GAME_INFO_BODY(keyText)                                                    \
    inline GameInfo Info() {                                                       \
        GameInfo g;                                                                \
        g.key = keyText;                                                           \
        g.title = GAME_TITLE;                                                      \
        g.modTitle = MOD_TITLE;                                                    \
        g.mod = MOD_NAME;                                                          \
        g.exe = GAME_EXE;                                                          \
        g.docs = DOCS_DIR;                                                         \
        g.agent = AGENT_FILE;                                                      \
        g.start = START_PHRASE;                                                    \
        for (const wchar_t* p : kProcesses) g.processes.push_back(p);              \
        for (const auto& b : kProvenBuilds) g.builds.push_back({b.ts, b.size, b.name}); \
        for (const auto& r : kRegPaths) g.regs.push_back({r.key, r.value});        \
        if (kSteamUnwrap.wrappedSha) g.unwrapFrom = kSteamUnwrap.wrappedSha;       \
        if (kSteamUnwrap.resultSha) g.unwrapResult = kSteamUnwrap.resultSha;       \
        if (kRuText.ini) g.ruIni = kRuText.ini;                                    \
        if (kRuText.page) g.ruPage = kRuText.page;                                 \
        for (const wchar_t* f : kRuText.files) if (f) g.ruFiles.push_back(f);      \
        return g;                                                              \
    }

namespace dao_game {
#include "../../games/dao/game.h"
GAME_INFO_BODY(L"dao")
}
#undef GAME_DAO
#undef MOD_TITLE
#undef MOD_NAME
#undef GAME_TITLE
#undef GAME_EXE
#undef DOCS_DIR
#undef AGENT_FILE
#undef START_PHRASE
#undef PARTY_PICKER_LIST

namespace da2_game {
#include "../../games/da2/game.h"
GAME_INFO_BODY(L"da2")
}
#undef GAME_DA2
#undef CLOSE_AFTER_EXIT_DELAY
#undef MOD_TITLE
#undef MOD_NAME
#undef GAME_TITLE
#undef GAME_EXE
#undef DOCS_DIR
#undef AGENT_FILE
#undef START_PHRASE

inline std::vector<GameInfo> AllGames() { return {dao_game::Info(), da2_game::Info()}; }
