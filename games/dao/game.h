// Dragon Age: Origins settings of the shared code (core\src, core\installer).
// Everything that differs between the games and is only a name, a path or a proven
// build lives here; code that differs lives in games\dao\src.
#pragma once

#define GAME_DAO 1
#define MOD_TITLE    L"DAO Access"
#define MOD_NAME     L"DAOAccess"              // bin_ship\scripts\DAOAccess.asi, its folder and log
#define GAME_TITLE   L"Dragon Age: Origins"
#define GAME_EXE     L"DAOrigins.exe"          // inside bin_ship
#define DOCS_DIR     L"BioWare\\Dragon Age"    // inside Documents (override, Settings)
#define AGENT_FILE   L"daoaccess.gfx"          // the Flash agent (core\agent), override folder
#define START_PHRASE L"Dragon Age Access загружен"
// The party screen lists the game mode's candidates (objects.cpp PartyCandidates);
// without it the menu reader picks characters with the cursor (Dragon Age II).
#define PARTY_PICKER_LIST 1

// Processes that must be closed before files are replaced (installer, deploy.ps1).
static const wchar_t* const kProcesses[] = {
    L"DAOrigins.exe", L"DAOriginsLauncher.exe", L"DAOriginsConfig.exe"};

// Builds the mod is proven on: PE TimeDateStamp and SizeOfImage (NOTEBOOK.md, Game).
// The Large Address Aware flag does not change these two.
struct ProvenBuild { unsigned long ts, size; const wchar_t* name; };
static const ProvenBuild kProvenBuilds[] = {
    {0x4eb19d53, 0x9ec0a2, L"DAOrigins.exe 1.05.13263.0 (GOG)"},
    // Steam copy, same code (NOTEBOOK.md, Game): SteamStub-wrapped as Steam ships it
    // (in-game test 2026-11-20 13:12), and unwrapped by Steamless with LAA
    // (in-game tests 2026-11-20 13:43 and 14:54).
    {0x4eb19d53, 0xa49000, L"DAOrigins.exe 1.05.13263.0 (Steam)"},
    {0x4eb19d53, 0x9ed000, L"DAOrigins.exe 1.05.13263.0 (Steam, Steamless)"}};

// HKLM keys (32-bit view) with the install folder. GOG games are found through the
// GOG registry keys in the installer (proven on the player's PC). This BioWare key is
// a HYPOTHESIS (the Dragon Age 2 installer writes BioWare\Dragon Age 2\Path); a key
// that does not exist is simply skipped.
struct RegPath { const wchar_t* key; const wchar_t* value; };
static const RegPath kRegPaths[] = {
    {L"SOFTWARE\\BioWare\\Dragon Age", L"Path"}};
