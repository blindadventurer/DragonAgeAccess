// Dragon Age II settings of the shared code (core\src, core\installer). Everything
// that differs between the games and is only a name, a path or a proven build lives
// here; code that differs lives in games\da2\src.
#pragma once

#define GAME_DA2 1
#define MOD_TITLE    L"DA2 Access"
#define MOD_NAME     L"DA2Access"              // bin_ship\scripts\DA2Access.asi, its folder and log
#define GAME_TITLE   L"Dragon Age II"
#define GAME_EXE     L"DragonAge2.exe"         // inside bin_ship
#define DOCS_DIR     L"BioWare\\Dragon Age 2"  // inside Documents (override, Settings)
#define AGENT_FILE   L"da2access.gfx"          // the Flash agent (core\agent), override folder
#define START_PHRASE L"Dragon Age 2 Access загружен"

// Processes that must be closed before files are replaced (installer, deploy.ps1).
static const wchar_t* const kProcesses[] = {
    L"DragonAge2.exe", L"DragonAge2Launcher.exe", L"DragonAge2Config.exe"};

// Builds the mod is proven on: PE TimeDateStamp and SizeOfImage (NOTEBOOK.md, Game).
// The Large Address Aware flag does not change these two.
struct ProvenBuild { unsigned long ts, size; const wchar_t* name; };
static const ProvenBuild kProvenBuilds[] = {
    {0x4f1e9fb5, 0xbd5000, L"DragonAge2.exe 1.04.8524.0"},
    // Steam copy under EA's wrapper (NOTEBOOK.md, Game; in-game tests 2026-11-20
    // 13:03 menus, 14:50 EA sign-in and a save loaded, 15:33 level-up).
    {0x4f1e9fb5, 0xbd2000, L"DragonAge2.exe 1.04.8524.0 (Steam)"}};

// No unwrap route for the 4 GB flag: EA's wrapper has no Steamless counterpart.
struct SteamUnwrap { const char* wrappedSha; const char* resultSha; };
static const SteamUnwrap kSteamUnwrap = {nullptr, nullptr};

// HKLM keys (32-bit view) with the install folder. Proven on the player's PC
// 2026-11-19: SOFTWARE\WOW6432Node\BioWare\Dragon Age 2, value Path.
struct RegPath { const wchar_t* key; const wchar_t* value; };
static const RegPath kRegPaths[] = {
    {L"SOFTWARE\\BioWare\\Dragon Age 2", L"Path"}};
