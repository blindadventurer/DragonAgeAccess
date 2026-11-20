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
- testers logs\ (since 2026-11-20, git-ignored: other people's play, names and
  typed text): mod logs the player collects from testers. The game notebooks' test
  logs say which tester log a finding came from.

## Status

- 2026-11-20 evening: agent v35 (shared core\agent and menu.cpp) built for both
  games, deployed to both (deploy.ps1, 2026-11-20 evening), NOT tested: Origins codex readable (games\dao\NOTEBOOK.md,
  Stages, Codex, money and pause v1), money alerts in coins, pause from the game's
  state. Also in Dragon Age II, check there: long texts (300+ characters with
  line breaks) now come one paragraph per item, a text shown twice in one clip is
  listed once, the pause line (DAOAccess.PauseLine falls back to the old sign scan
  when Client.Paused cannot be read).

- 2026-11-20 14:55: both Steam games work with the mod on the player's PC (menus,
  scanner, conversations in Origins; menus, EA sign-in, DLC, a save loaded in
  Dragon Age II), Origins with LAA (Steamless exe) and Russian text (ENPY talk
  tables). Done 2026-11-20 evening: the Steam builds in kProvenBuilds (Origins
  0xa49000 wrapped / 0x9ed000 Steamless, Dragon Age II 0xbd2000, re-read from the
  player's exes with pe.py that evening), README notes for Steam players, release
  v0.3.0 (next item).
- 2026-11-20 evening, release v0.3.0 (player: "собрать новый релиз и новый
  установщик ... запушить, дальше дорабатывать по отзывам тестировщиков"): installer
  1.1 (Steam builds proven, no LAA on wrapped exes), DAOAccess-0.3.0.zip and
  DA2Access-0.3.0.zip with agent v35 and everything under Status above, global.ini
  in the Dragon Age II zip. The same evening the installer asset of v0.3.0 was
  replaced by installer 1.2 (Steam Origins 4 GB through Steamless; the mod zips
  unchanged, so no new mod version). Released while the agent v33-v35 changes were still
  untested in a game (player's choice: testers and the player test the release).
  Published as a normal release (commit 3df691d, title "0.3.0 (тестовая
  версия)"); proven the same evening: the installer test build found v0.3.0 through
  releases/latest and downloaded and unpacked both zips (/check:DAOAccess,
  /check:DA2Access). Next: fixes from tester reports and the player's play.
- 2026-11-20: the player switched both games to Steam (F:\SteamLibrary; the old GOG
  Origins and the old Dragon Age II folders in F:\games are deleted, together with
  the mod files in them). Both Steam exes are the same builds (same PE stamp) wrapped
  by store DRM; the mod's addresses were re-proven from the files (game notebooks,
  Game and Proven facts). Changes for Steam, built, NOT yet tested in a game: the
  installer leaves a DRM-wrapped exe alone (no LAA offer), deploy.ps1 and the
  research tools point to the Steam folders. The new build is in both Steam games
  (deploy.ps1, 2026-11-20 12:53). First test 12:55: neither mod started. Origins:
  the Steam launcher hides its Play button from OCR, so the game never ran; way
  round it is a Steam launch option (games\dao\NOTEBOOK.md, Game), set for the
  player 13:07, Origins test pending. Dragon Age II: the game ran, the mod did not
  load; with bin_ship\scripts\global.ini (DontLoadFromDllMain=0, Dragon Age II only,
  games\da2\loader\global.ini, shipped by deploy.ps1 and make_release.ps1) it
  loaded and read the menus (13:03). The README needs the Steam launch option for
  Origins players once it is proven. Found on the way and fixed in both games:
  the mode vtable hooks patched past the end of the vtables (see Pitfalls). The
  Steam builds go into kProvenBuilds only after the in-game test (player's choice).
  The release v0.2.0 still has the old installer and the old slot counts: a new
  release follows the test.
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
  kSetupVersion, 1.2 since 2026-11-20 evening). It holds no mod files and is rebuilt only when its code
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
- 2026-11-20 (player): the installer does not offer the 4 GB (LAA) flag for an exe
  wrapped by store DRM (entry point outside the first section: PeInfo.wrapped in
  setup.cpp); the report says why. Reason: the code of both Steam exes is encrypted
  on disk, and players report Steam Origins no longer starting (error code 51) after
  the 4GB patch (Steam community / Nexus posts found 2026-11-20). For Origins on
  the player's PC the exe was unwrapped with Steamless and given LAA by hand
  (games\dao\tools\make_laa_exe.py, games\dao\NOTEBOOK.md Game folder changes). Since
  installer 1.2 (2026-11-20 evening, player: "Сделать сейчас") the installer does
  the same for the Steam Origins exe, with a question: setup.cpp SteamUnwrapWork
  downloads Steamless 3.1.0.5 from its GitHub release (zip SHA-256 pinned,
  kSteamlessSha256; CC BY-NC-ND, so never shipped by us), unwraps a copy with
  Steamless.CLI.exe --quiet, clears the security directory past the end, sets LAA
  and the checksum, and writes it only when its SHA-256 is the proven one
  (kSteamUnwrap in games\dao\game.h: Steam exe 400c2c9e...47bd, result
  b74dca7a...8f45); the original stays as .before-laa and removal puts it back.
  Any other wrapped exe (Dragon Age II, a changed Steam build) gets no LAA.
- 2026-11-20 (after a password reached the log): what is typed into input fields is
  never logged. SpeakPrivate (speech.cpp) speaks but logs "<скрыто>"; item lines of
  a non-empty "edit" field log "<скрыто>"; edit start / text / end log only the
  length. Password fields (TextField.password, agent v31 kind "password") are never
  spoken either: "звёздочка" per character, the field says "N симв.". Typing focus
  follows the cursor (menu.cpp StartTyping / StopTyping): arriving on a field
  focuses it with the caret at the end, Up / Down / Tab leave it, Enter finishes
  (and the game gets that Enter), Enter on a finished field types again with the
  old text selected. Built, deployed 2026-11-20, NOT tested in a game yet; check in
  Origins too (character name, save name).
- 2026-11-20 (player), for the next release, in this order: (1) Dragon Age II
  level-up made accessible (attribute points, abilities; "Авто. уровень" is the
  stopgap); (2) installer, Origins on Steam, each with a question: 4 GB memory (DONE
  in installer 1.2, see the LAA decision above) by
  downloading Steamless 3.1.0.5 from github.com/atom0s/Steamless (pinned version and
  SHA-256; CC BY-NC-ND, so never shipped by us) to unwrap a copy of the exe on the
  player's PC, then LAA (as games\dao\tools\make_laa_exe.py), original kept as
  .before-laa; Russian text by finding the ENPY files already on the PC
  (Documents\...\override\enpy) and copying the six talk tables into the game folder
  plus Locale=ru-ru (ENPY files are not ours to ship: enpy.net/license/ forbids
  placing them anywhere without the editors' consent; the player chose 2026-11-20
  not to ask for now; without them the installer offers to open
  https://enpy.net/projects/localizations/games/dragon_age_origins/ in the browser
  to download the translation, and to run the installer again afterwards); (3) Steam builds in kProvenBuilds, release, README.
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
- Installer and Steam, proven 2026-11-20 (test build /find and /auto on copies of the
  Steam exes in a scratch folder): /find finds Steam Origins through
  HKLM\SOFTWARE\BioWare\Dragon Age\Path (32-bit view; the key the Steam install
  writes) and Steam Dragon Age II through the Steam libraries
  (F:\SteamLibrary\steamapps\common\Dragon Age II); both exes are logged as
  "wrapped (DRM stub)", the unknown-build question comes up, LAA is not offered and
  the exe stays byte-identical.
- Log size (core\src\log.cpp, scanner.cpp; checked 2026-11-20): at game start a
  log over 20 MB becomes <Mod>.old.log (replacing the older one), so at most two
  logs stay; the objdump file is capped at 6 MB. Growth seen: about 1.1-1.6 MB for
  5 hours of play (player and tester logs 2026-11-20), mostly the perf line every
  10 s and the memory line every 30 s.
- KeyBindings.ini of both games (Documents\BioWare\<game>\Settings, ASCII, CRLF):
  ZoomIn_1=Keyboard::Button_Next and ZoomOut_1=Keyboard::Button_Prior by default
  (both files read 2026-11-19).
- Merged build: both games compile from core + games\<game>; the merged sources were
  compared line by line with both old projects (only the changes listed under Status).

## Pitfalls

- Bash heredocs here lose backslashes inside Python string literals; write such
  scripts to a file with the Write tool and run them from there.
- The mod logs every spoken line and every item it reads: anything private shown
  or typed on screen ends up in the log unless it goes through SpeakPrivate and
  the masked item line. 2026-11-20 13:05 an EA password typed into the e-mail box
  was logged in plain text; Claude masked the typed values in DA2Access.log the
  same day (scratchpad scrub.py) and told the player.
- Scaleform's ActionScript is slow at building strings one character at a time
  (each += copies the string): the agent's Clean did that and froze Dragon Age II
  for 45 s on its privacy policy (2026-11-20). Use split/join and substring in the
  agent, never a character loop over screen text; texts over A.maxText (3000) are
  cut. The perf line ("worst call N ms in <movie>") is where such stalls show.
- sed -i in Git Bash rewrites CRLF files with LF (the whole file shows as changed in
  git diff), and heredoc appends add LF lines. The repository mixes both (the game
  notebooks and objects.cpp are CRLF; NOTEBOOK.md, menu.cpp, speech.cpp, the agent
  are LF): keep each file's own endings. Prefer the Edit tool.
- Compress-Archive of PowerShell 5.1 writes zip entries with backslashes, which some
  extractors do not take as folders: make_release.ps1 uses tar.exe -a instead.
- Vtable slot counts (both games' objects.cpp, PatchVtable): a vtable in .rdata is
  followed by other data, often UTF-16 strings, whose dwords (0x6E0049 for "In")
  fall inside the exe's address range, so the InExe check passes them. A count that
  is too high silently overwrites game strings. Count the slots by reading the
  vtable up to the first entry that is not a function (proven counts: Origins
  explore 32, combat 32, party picker 30; Dragon Age II explore 36, combat 36; the
  old 39 / 39 / 42 overwrote "Target Required", "LockedPartyMembers", "InCombatMode").
- Store copies of both games encrypt the exe's code on disk (Steam SteamStub in
  Origins, EA's wrapper in Dragon Age II): reading .text from the file gives noise
  (entropy 8.0). .rdata and .data are plain. Origins can be decrypted with Steamless
  (games\dao\work\DAOrigins.unpacked.exe); Dragon Age II has no decrypted copy.
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
- 2026-11-20 (Claude, scratch folder): installer test build on copies of the Steam
  exes of both games: found, "wrapped (DRM stub)", LAA not offered, exe unchanged.
  As expected.
- 2026-11-20 19:36 (Claude, scratch folder): installer 1.1 test build, /auto with
  /source:DA2Access-0.3.0.zip on a copy of the Steam DragonAge2.exe: no unknown-build
  question (Steam build now proven), global.ini copied, LAA not offered, freekeys
  recorded. The Origins run stopped at "game running: DAOrigins.exe" (the player
  was playing): the running-game check works; the Origins copy was not installed.
- 2026-11-20 ~20:10 (Claude, scratch folder): installer 1.2 test build on a copy of
  the Steam DAOrigins.exe (from .before-laa): real download of Steamless 3.1.0.5
  (610 646 bytes, hash as pinned), Steamless exit 0, security directory
  0xa19000+0x1760 cleared, result 10 217 312 bytes, SHA-256 b74dca7a...8f45 = the
  exe proven in the game; report "4 ГБ включён". Second run: "уже был включён".
  /uninstall: Steam exe restored byte for byte (400c2c9e...47bd). Dragon Age II
  copy: LAA not offered, as before. All as expected.
