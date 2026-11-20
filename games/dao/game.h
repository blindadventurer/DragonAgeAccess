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

// The Steam exe the installer may unwrap to give it the 4 GB flag (core\installer
// setup.cpp SteamUnwrapWork): SHA-256 of DAOrigins.exe as Steam ships it, and of the
// result of Steamless 3.1.0.5 + security directory cleared + LAA + checksum, i.e.
// what games\dao\tools\make_laa_exe.py made (NOTEBOOK.md, Game folder changes; the
// result proven in the game 2026-11-20 13:43). Both hashes read from the player's
// files 2026-11-20. Nothing else is ever written over the exe.
struct SteamUnwrap { const char* wrappedSha; const char* resultSha; };
static const SteamUnwrap kSteamUnwrap = {
    "400c2c9e97bb1a534121553be66f202429ef6f7320c93c86f9b7a491864647bd",
    "b74dca7a1b1b69f9242d94afb47b2f67e8c655df89914352cca7d31f76dd8f45"};

// Steam starts DAOriginsLauncher.exe, whose window gave the player's screen reader no
// Play button (test 2026-11-20 12:55). The launch option
// "<game>\bin_ship\DAOrigins.exe" %command% makes Steam start the game exe at once
// (proven 2026-11-20 13:12, NOTEBOOK.md Game). The installer writes it into Steam's
// localconfig.vdf (setup.cpp SteamLaunchOption) for this Steam app id
// (steamapps\appmanifest_47810.acf, "appid" "47810", read 2026-11-20).
struct SteamLaunch { const wchar_t* appId; };
static const SteamLaunch kSteamLaunch = {L"47810"};

// Russian text (installer, setup.cpp RussianText). A translation in the override
// folder (ENPY's lies in override\enpy) is not enough: the game falls back to English
// unless its install folder holds packages\core\data\talktables\core_<Locale>.tlk
// (strings of the decrypted exe; Locale=ru-ru alone was written back to en-us,
// 2026-11-20 14:01). Copying these six talk tables from the translation into the
// install folder and Locale=ru-ru in DragonAge.ini [GameOptions] gave Russian text
// (proven 2026-11-20 14:54, NOTEBOOK.md). The files are the player's own; we never ship
// them (ENPY terms). files[0] is the one the game checks.
struct RuText { const wchar_t* ini; const wchar_t* page; const wchar_t* files[6]; };
static const RuText kRuText = {
    L"Settings\\DragonAge.ini",
    L"https://enpy.net/projects/localizations/games/dragon_age_origins/",
    {L"packages\\core\\data\\talktables\\core_ru-ru.tlk",
     L"packages\\core\\data\\talktables\\core_ru-ru_f.tlk",
     L"packages\\core_ep1\\data\\talktables\\core_ep1_ru-ru.tlk",
     L"packages\\core_ep1\\data\\talktables\\core_ep1_ru-ru_f.tlk",
     L"modules\\single player\\data\\talktables\\singleplayer_ru-ru.tlk",
     L"modules\\single player\\data\\talktables\\singleplayer_ru-ru_f.tlk"}};

// HKLM keys (32-bit view) with the install folder. GOG games are found through the
// GOG registry keys in the installer (proven on the player's PC). This BioWare key is
// a HYPOTHESIS (the Dragon Age 2 installer writes BioWare\Dragon Age 2\Path); a key
// that does not exist is simply skipped.
struct RegPath { const wchar_t* key; const wchar_t* value; };
static const RegPath kRegPaths[] = {
    {L"SOFTWARE\\BioWare\\Dragon Age", L"Path"}};
