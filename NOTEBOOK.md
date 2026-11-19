# Notebook: Dragon Age Access (shared core)

This notebook holds what both games share. Each game has its own notebook with its
exe, addresses, stages and tests: games\dao\NOTEBOOK.md (Dragon Age: Origins) and
games\da2\NOTEBOOK.md (Dragon Age II).

## Project

- One folder and one repository for both mods since 2026-11-19 (see Decisions).
  Before that they were two folders, DragonAgeOriginsAccess and DragonAge2Access
  (kept on the player's PC as a backup until the merged build is proven in both
  games; paths like src\scanner.cpp or tools\erf3.py in the game notebooks mean
  core\src\... or games\<game>\tools\... now).
- Layout:
  - core\src: main (probe, crash filter, memory line), log, speech (Prism), menu
    reader, scanner (with walking, quests, combat, quickbar, party), the shared
    interfaces objects.h and move.h.
  - core\agent\daoaccess.as: the Flash agent (AS2), the same for both games; its
    global is still called DAOAccess in both.
  - core\installer: setup.cpp and build_setup.ps1 (the installer).
  - games\<game>\game.h: names, folders, process names, proven builds, registry keys;
    read by the mod and the installer. Origins also sets PARTY_PICKER_LIST.
  - games\<game>\src: gfx.cpp / gfx.h (Scaleform GFx 2.x in Origins, 3.x in Dragon
    Age II), objects.cpp (world objects at that exe's addresses), move.cpp.
  - games\<game>\tools: that game's research scripts; tools\: shared scripts
    (build_agent.py, as2dis.py, methods.py) and games.ps1 (names for the scripts).
- Build: build.ps1 [-Game dao|da2|all] -> build\<game>\<Mod>.asi and the agent .gfx;
  deploy.ps1 copies into the player's games (paths at its top); make_release.ps1
  -Version X -> release\DAOAccess-X.zip, release\DA2Access-X.zip and
  release\DragonAgeAccess-Setup.exe (the one installer, no mod files inside).
- Names per game: Origins DAOAccess / daoaccess.gfx / Documents\BioWare\Dragon Age;
  Dragon Age II DA2Access / da2access.gfx / Documents\BioWare\Dragon Age 2.

## Status

- Merge done 2026-11-19 and built for both games; NOT tested in either game yet.
  Dragon Age II code is unchanged apart from names (diff review of every file);
  Origins now also runs what was added for Dragon Age II. Check in the first test of
  each game:
  - Both: start phrase, menus, scanner (PageDown), walking (End), a fight (Enter on
    an enemy, ] [), quickbar (Backspace, 1-0). Log lines renamed: "scan not in play
    left out: N | flag contradicted by health | not in play objects kept" replaces
    "scan inactive left out" (Origins) and "scan dormant left out" (Dragon Age II).
  - Origins, new from Dragon Age II: Left / Right on a picker (options, difficulty)
    steps its entries when the picker has SelectNextEntry, else switches sections as
    before; a picker without a label reads "список, <value>"; unlabeled party
    portraits read the member's name; quickbar slots are read through
    .AbilityTemplate.ID and .Cooldown.CooldownPercentage first (falling back to the
    old fields); the combat target through .CombatTarget.ID first; in a fight Enter
    goes through the combat mode also when the narration (not the catalogue) says
    combat; a repeated Enter on the same target is silent (no second "Атакую");
    a creature not in play is kept when the catalogue shows its health.
  - Origins party selection at camp still uses its own candidate list
    (PARTY_PICKER_LIST); Dragon Age II keeps the cursor chooser.
  - Both, new 2026-11-20: FreeZoomKeysOnce at start (main.cpp); with install.txt
    freekeys=1 and no KeyBindings.ini.before-access it frees ZoomIn_1 / ZoomOut_1 once
    (log "keys: ..."). Not yet seen in a game.
- Published 2026-11-20: https://github.com/blindadventurer/DragonAgeAccess (public,
  branch main) and the normal release v0.2.0 (title "тестовая версия") with
  DragonAgeAccess-Setup.exe, DAOAccess-0.2.0.zip, DA2Access-0.2.0.zip (player's choice:
  publish, then test, since the installer needs a release to download). Proven the
  same night: the test build found v0.2.0 through releases/latest and downloaded and
  unpacked both zips (/check:DAOAccess, /check:DA2Access). Waiting for the player's
  test of the installer with NVDA and of both games. After it passes: archive
  blindadventurer/DragonAgeOriginsAccess and blindadventurer/DragonAge2Access with a
  link to the new repository (README note, then gh repo archive).

## Decisions

- 2026-11-19 (player, on a friend's advice): one project and one repository for both
  games with a shared core, instead of two parallel projects. Reason: about 90% of
  the code was the same (log, speech, installer identical; main, scanner, agent a few
  dozen lines apart), so every fix had to be ported by hand. Supersedes the earlier
  decision to port fixes between the two projects only on request.
- 2026-11-19 (player): the new public repository DragonAgeAccess replaces
  blindadventurer/DragonAgeOriginsAccess and blindadventurer/DragonAge2Access, which
  are archived (read-only) with a link to the new one, not deleted.
- 2026-11-19 (player): installers are released only after the merge, from the new
  repository; the player tries an installer with NVDA before the release.
- Publishing rules (player, 2026-11-19, from the Age of the Ring project): public
  repository; MIT licence (LICENSE); README.md in Russian then English; notebooks and
  CLAUDE.md published; commits use the GitHub noreply address
  301905384+blindadventurer@users.noreply.github.com, never the player's mail; no
  Windows user name in published files (%USERPROFILE% instead); nothing about how a
  game copy was obtained. Not in git (.gitignore): build, release, games\*\work (game
  extracts), FFDec, zips, edited copies of game tables. core.autocrlf false.
- Releases: one tag vX per release with DAOAccess-X.zip, DA2Access-X.zip and
  DragonAgeAccess-Setup.exe (versions move together, the core is shared). Published
  as a NORMAL release, not a prerelease (the title may say "тестовая версия"): the
  installer reads github.com/<repo>/releases/latest, and a prerelease is never
  "latest". The zip name must be <Mod>-X.zip for tag vX. Steps: make_release.ps1
  -Version X, then gh release create vX with the three files of release\.
- 2026-11-19/20 (player: "под ключ", "один установщик с выбором игры", "установщик
  проверяет GitHub и обновляет мод"): one installer for both games,
  core\installer\setup.cpp (games_table.h takes both games' names from their game.h),
  built by build_setup.ps1 into DragonAgeAccess-Setup.exe (installer version
  kSetupVersion, 1.0). It holds no mod files and is rebuilt only when its code
  changes. Steps: finds both games (installer folder, HKLM BioWare keys, GOG keys,
  Steam libraries, EA / Origin / GOG default folders) and asks in a task dialog with
  named command buttons: each game (with its folder or "not found"), both, or a
  folder by hand (the game is told by its exe); waits while the game runs; finds the
  newest release (the 302 of github.com/<repo>/releases/latest gives the tag, the zip
  is .../releases/download/vX/<Mod>-X.zip; fallback: REST API releases list) and
  downloads and unpacks it (WinHTTP, tar.exe) under a progress dialog; when the mod
  is installed it offers "Обновить до версии X" / "Переустановить" and "Удалить мод";
  warns when the exe stamp and size are not a proven build; replaces a foreign
  dinput8.dll only with consent (backup .before-access); copies bin_ship and override,
  then reads every file back (an antivirus removing dinput8.dll is reported with the
  way out); offers LAA (backup <exe>.before-laa, CheckSumMappedFile); offers freeing
  ZoomIn_1 / ZoomOut_1 (backup .before-access), or when the game never wrote
  KeyBindings.ini yet records freekeys=1 for the mod. Writes
  bin_ship\scripts\<Mod>\install.txt (version, setup, source, dinput8 SHA-256, files,
  freekeys). Removal deletes the recorded files (an install without install.txt: the
  known names; dinput8.dll only when its hash is ours or UAL 9.7.4's), restores a
  foreign dinput8.dll, offers to put back the exe (only a backup of the same build) and
  the zoom keys. Runs as administrator (manifest). Log %TEMP%\DragonAgeAccess-setup.log
  and bin_ship\scripts\<Mod>.setup.log. Test switches: /find, /auto with
  /gamekey:dao|da2|all, /uninstall, /game:, /docs:, /source:<zip or folder>,
  /repo:<owner/name>, /check:<Mod>; build_setup.ps1 -AsInvoker makes a test build
  without the UAC window.
- Nothing else is needed on a tester's PC: the mod and prism.dll import only Windows
  DLLs (pefile 2026-11-20: no Visual C++ runtime; Prism delay-loads the PC-Talker,
  ZDSR and BoyPC client DLLs only for those readers).
- Both copies of the agent ship (bin_ship\scripts\<Mod>\ and the override folder)
  because only the override folder is proven; loading from the mod folder is a
  HYPOTHESIS.

## Proven facts

- Downloading, proven 2026-11-20 with the one installer (/repo:blindadventurer/BG3Access
  /check:BG3Access): github.com/<repo>/releases/latest answers 302 to
  .../releases/tag/v0.6.0 without the API, and .../releases/download/v0.6.0/
  BG3Access-0.6.0.zip downloaded (1 374 834 bytes) and unpacked with tar.exe. The REST
  API answered 403 "rate limit exceeded" (60 an hour per address, used up) the same
  night: the reason the web page comes first.
- Installer, proven 2026-11-19/20 by Claude on copies in a scratch folder (no game
  run), both games, per-game installers and the one installer (/source with the
  release zips): fresh install with freekeys=1 when KeyBindings.ini is missing,
  "Переустановить" for the same version, "Обновить до версии 0.2.0" from 0.1.0, removal
  with the exe restored to the original; earlier runs: install, update and removal; the LAA exe is
  byte-identical to the one tools\set_laa.py (pefile) made on the player's PC;
  the freed keys file matches the player's hand edit (Origins); /find found Dragon
  Age II through HKLM\SOFTWARE\WOW6432Node\BioWare\Dragon Age 2\Path and Origins
  through the GOG key; unknown build and missing key file messages. NOT tested: the
  dialogs with NVDA, the folder picker, a Steam or EA app install, a standard user
  elevating with another account (Documents would then be that account's).
- KeyBindings.ini of both games (Documents\BioWare\<game>\Settings, ASCII, CRLF):
  ZoomIn_1=Keyboard::Button_Next and ZoomOut_1=Keyboard::Button_Prior by default
  (both files read 2026-11-19).
- Merged build: both games compile from core + games\<game>; the merged sources were
  compared line by line with both old projects (only the changes listed under Status).

## Pitfalls

- Bash heredocs here lose backslashes inside Python string literals; write such
  scripts to a file with the Write tool and run them from there.
- Compress-Archive of PowerShell 5.1 writes zip entries with backslashes, which some
  extractors do not take as folders: make_release.ps1 uses tar.exe -a instead.
- The installer needs administrator rights; a test run from a non-elevated shell
  fails with ERROR_ELEVATION_REQUIRED, so tests use build_setup.ps1 -AsInvoker.

## Test log

One line per test of the shared parts; in-game tests go in the game's notebook.

- 2026-11-19 (Claude, scratch folder): installers of both games, install / update /
  remove / find / unknown build / missing key file, before and after the merge. All
  as expected.
- 2026-11-20 (Claude, scratch folder): the one installer: real download of a GitHub
  release (BG3Access), install / same version / update from 0.1.0 / removal for both
  games from the 0.2.0 zips, freekeys=1 recorded. All as expected. A bug found and
  fixed: a failed download listed files under an empty path, i.e. the drive root;
  ListFiles and DeleteTree now refuse paths that are not absolute or are a root.
