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
  - core\watcher\watcher.cpp: <Mod>Watch.exe, closes programs left behind after the
    game exits (Dragon Age II: the EA app; games\da2\NOTEBOOK.md, Decisions).
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

- 2026-11-22 late, release v0.4.0 (player: fix the Dragon Age II level-up, then a
  release with all new changes; chose "release at once" over testing first): agent
  v41 (Dragon Age II attribute values, ability upgrades in their chains, tree opens on
  a learnable ability; games\da2\NOTEBOOK.md test log 2026-11-22 20:38) on top of the
  Origins Fixes v9-v11 and the engine object list. Deployed to both games the same
  evening. DAOAccess-0.4.0.zip (SHA-256 836a6036...e1f6), DA2Access-0.4.0.zip
  (ce853da5...bdd8), installer 1.4 rebuilt (8a895f12...9645), normal release, title
  "0.4.0 (тестовая версия)". Released while v41 and Fixes v11 are NOT tested in a game, and the shared
  v39/v40 changes NOT tested in Dragon Age II.
- 2026-11-22: player asked for links to the most popular content and improvement
  mods of both games (no UI, no textures), to download and then install with
  Claude: MODS.md (Nexus API v2 GraphQL, top 320 per game by endorsements, name
  filter op WILDCARD finds Russian translations). Not installed yet. Install one
  group at a time and check the access mod after each (gfx overrides, Russian
  text in override\enpy against Qwinn's Fixpack, party picker with Extra Dog
  Slot, autowalk with Faster Runspeed).
- 2026-11-22 evening: Origins Fixes v11 deployed with the player's OK, NOT tested: names of received
  items through a hook on the game's item-added handler, no repeated "выберите
  цель" after a cast (games\dao\NOTEBOOK.md, Fixes v9). Shared change: objects.h
  TakeReceivedItems (Dragon Age II stub returns nothing), menu.cpp items alert.
- 2026-11-22 17:30: Origins Fixes v10 deployed and PROVEN, not committed: Shock
  and other aimed abilities go through the game's own click handler (test
  16:19-16:25: the v9 route faulted every time; 17:30 Shock and Lightning hit;
  games\dao\NOTEBOOK.md, Stages, Scanner, Fixes v9 and v10). The engine object list, Враги at a fight's start,
  object verbs and floating texts of Fixes v9 work in that test. Shared code
  changed since 0.3.1 (scanner.cpp, menu.cpp, objects.h, agent v40): Dragon Age
  II built but not deployed; check there when next played: "Предметы получены"
  (falls back to the alert's text if its lists differ), Враги at a fight's
  start, the pause stopping "Не получается достать", object states (act= is
  Origins-only, so likely silent there), floating texts (log "floaty: no
  FloatyLayer class" if its layer differs).

- 2026-11-21, release v0.3.1 (player asked whether everything was pushed for the
  testers, then chose commit, push and a new release): commit 1e254b2 (agent v37
  and v38, menu.cpp), DAOAccess-0.3.1.zip (SHA-256 ca627cd7...3ebe),
  DA2Access-0.3.1.zip (224f6bc1...3a86), installer 1.4 rebuilt (884efef8...7927),
  normal release, title "0.3.1 (тестовая версия)". Proven the same evening:
  releases/latest redirects to v0.3.1 and the three downloaded assets match these
  hashes. Released while v37/v38 are still NOT tested in a game (as with 0.3.0).
- 2026-11-20 late: agent v38 (Dragon Age II ability trees in chains, see
  games\da2\NOTEBOOK.md test log) deployed to both games on top of v37, NOT tested.
- 2026-11-20 ~22:40: agent v37 and menu.cpp built for both games and deployed to
  both (player approved), NOT tested. Dragon Age II level-up and journal fixes
  (games\da2\NOTEBOOK.md, test log 22:01). Shared changes to check in Origins too:
  a short text with line breaks is read as sentences ("A. B") instead of run
  together; an icon button with no visible text reads its hidden button text
  (ButtonBase.GetButtonText) before "без подписи"; on screens named CharacterRecord
  or abilities the selected character's name is said when no text above the first
  widget holds it.

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
  unchanged, so no new mod version), then by installer 1.3 (Origins Russian text: a
  translation in override\ or one folder below it holding core_ru-ru.tlk is found,
  the missing talk tables of kRuText (games\dao\game.h) are copied into the game
  folder, never over a file, and Locale=ru-ru set in DragonAge.ini with a
  .before-access backup; record lines rufile= / locale=<old>, kept across updates;
  removal offers the former language back; a game with its own different
  core_ru-ru.tlk is left alone; with no translation, on a Russian Windows only, it
  offers to open ENPY's page). Released while the agent v33-v35 changes were still
  untested in a game (player's choice: testers and the player test the release).
  Published as a normal release (commit 3df691d, title "0.3.0 (тестовая
  версия)"); proven the same evening: the installer test build found v0.3.0 through
  releases/latest and downloaded and unpacked both zips (/check:DAOAccess,
  /check:DA2Access). Later that night (player: "Да, и опубликовать") the asset was
  replaced by installer 1.4 (Origins Steam launch option, see Decisions; 373 760
  bytes, SHA-256 77aa1491...6a1, the downloaded asset checked against it), release
  notes and README updated. Next: fixes from tester reports and the player's play.
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

## Reverse-engineering tools (since 2026-11-22)

- Ghidra through the MCP server "ghidra": this project's own Ghidra project is
  C:\tools\ghidra-projects\DragonAgeAccess.gpr (player's rule: one Ghidra project
  per access project, global CLAUDE.md). Holds since 2026-11-22 the Origins exe
  games\dao\work\DAOrigins.unpacked.exe (the Steamless copy, code readable), as
  program "/DAOrigins.unpacked.exe-531cd1"; functions named so far: the object
  manager ones (games\dao\NOTEBOOK.md, Proven facts, Object manager). Its first
  analysis takes several minutes in the background. Dragon Age II has no readable
  copy on disk (EA wrapper), so its decrypted image must first be dumped from the
  running game.
- Ghidra without the MCP server: tools\ghidra_cli.py (2026-11-22), run with
  C:\tools\mcp\ghidra\Scripts\python.exe: str (strings and their users), dec
  (decompile), callers, rename (saved in the project). About a minute a call.
  Proven 2026-11-22 (Shock research). Needed when the MCP server cannot open the
  project: 2026-11-22 13:35 this session's server failed (CONNECTION_CLOSED)
  because another Claude session of this project, started 13:10, still held
  DragonAgeAccess.gpr through its own pyghidra-mcp; with the player's OK that
  server (pyghidra-mcp and its python children) was stopped. A session cannot
  reconnect a user-config MCP server itself; a new session gets it again.
- Quick disassembly without Ghidra: Python capstone (pip --user, 2026-11-22) with
  pefile on the unpacked exe, a few lines in the scratchpad (map VA - ImageBase
  with pe.get_data).
- Frida 17.27 (MCP server "frida" and the Python package): proven 2026-11-22 to see
  the running DAOrigins.exe (process list only, no attach yet). Attaching to the
  player's running game needs their OK each time.
- Ideas offered 2026-11-22: the scanner taking the engine's own object list
  instead of walking ids with gaps (CHOSEN by the player the same day; Origins
  built and deployed, NOT tested, games\dao\NOTEBOOK.md Stages, Scanner, Object
  source; Dragon Age II would need its own proof in its own exe); the game's
  pathfinder for "reachable / metres by path"; the decrypted Dragon Age II dump.
- SafetyHook 0.7.0 with Zydis bundled, in lib\safetyhook (SOURCE.md there: source,
  sha256, licenses BSL-1.0 and MIT). Gives what our hand-made vtable patches cannot:
  hooks on ordinary (non-virtual) game functions and mid-function hooks that read
  and change registers; installs with the game's other threads frozen. NOT used by
  the mod yet (player's choice 2026-11-22: prepare now, adopt when a feature needs a
  non-virtual function; working vtable hooks stay). Proven 2026-11-22: a test
  program (inline hook + mid hook on its own function) built with our MSVC and
  passed in both x86 and x64; both mods also build with /std:c++latest (only the
  old Prism warning C4309, same as with C++17). To adopt: build.ps1 to
  /std:c++latest, add lib\safetyhook\safetyhook.cpp, compile Zydis.c as C (cl /c /TC
  /I lib\safetyhook), ship both license files. Untested risk (HYPOTHESIS): whether
  the EA wrapper of Dragon Age II checks its code for changes; the first inline hook
  there needs one in-game test.
- Dragon Age: Inquisition (asked 2026-11-22, not installed on this PC, web sources
  only): Frostbite 3, 64-bit exe, Denuvo at launch (licence text, 2014 news), EA app
  needed. Modding: Frosty Tool Suite (assets, reads Frostbite's own type info);
  FBTools (IDA plugin naming typeinfo and vtables). No screen-reader mod found.
  Our route would still fit: an injected 64-bit DLL, SafetyHook x64, Ghidra, Frida;
  the unknowns are whether Denuvo is still in the current build and how hard it
  makes reading the code.
- Other games for the same work (asked 2026-11-22, web sources): BioWare's Eclipse
  engine ran only Origins, Awakening and Dragon Age II (Lycium = the DA2 branch).
  Awakening is in the player's Origins install (addins\dao_prc_ep_1) and runs in
  the same DAOrigins.exe: HYPOTHESIS that the Origins mod already works there,
  never tested; its new areas, items and dialogues are the things to check.
  Older BioWare engines: Odyssey (KOTOR 1 and 2, Jade Empire base) and Aurora
  (Neverwinter Nights). KOTOR 1 already has a blind mod, Voice of the Old Republic
  (github.com/JeanStiletto/voice-of-the-old-republic, uses Prism too); none found
  for KOTOR 2 or Mass Effect. What carries over to other games is the method and
  tools (injected DLL, Prism, Ghidra, Frida, SafetyHook) and, for games with
  Scaleform GFx AS2 menus, the Flash agent idea; game code does not.

## Puzzle plan (idea, 2026-11-20, not started)

The player worries that puzzles could stop a blind playthrough. Where they are:
the Puzzle map section of each game notebook (Origins has most; the Dragon Age II
main campaign has none, only the DLC). The plan discussed, not yet chosen by the
player (the question was left open):

- Prepare in advance from game files, no tests needed: extract the puzzle area
  (.are) and its scripts, list the portals, plates and levers with positions and
  what switches them; find the solution (in scripts, else a walkthrough on the
  web); write a short spoken hint the mod says on entering the area.
- General improvements that help everywhere: the scanner says an object's state
  (lever down, plate pressed); in the Circle Tower Fade, a "current form" line.
- Check on the spot only what files cannot show: whether the scanner sees these
  objects in the live game and under which names, how walking copes (walls, the
  Gauntlet bridge). Ask the player to save just before the puzzle so a test is one
  or two short runs.
- Reaching a puzzle early, both HYPOTHESIS: a save downloaded from the web (ask
  first), or the Origins developer console with BioWare's own test scripts
  (resources zz_tr_tour_gauntlet_puzzles.utt, zz_tr_tour_gauntlet_riddles.utt).
- Order: Circle Tower Fade first (first in the story and the hardest), then the
  Gauntlet, then the rest as the player gets near them.

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
- Changelog (player, 2026-11-21: "когда выпускаем релиз писать небольшой чейнджлог"):
  every release first gets a section at the top of CHANGELOG.md (Russian, newest
  first, short player-facing bullets: what changed or got better, in which game), and
  the same bullets go into the release notes under "Что нового". make_release.ps1
  packs CHANGELOG.md into both zips next to README.md (the installer copies only
  bin_ship\ and override\, so testers read it on GitHub or in the zip).
- 2026-11-19/20 (player: "под ключ", "один установщик с выбором игры", "установщик
  проверяет GitHub и обновляет мод"): one installer for both games,
  core\installer\setup.cpp (games_table.h takes both games' names from their game.h),
  built by build_setup.ps1 into DragonAgeAccess-Setup.exe (installer version
  kSetupVersion, 1.4 since 2026-11-20 late evening). It holds no mod files and is rebuilt only when its code
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
  .before-laa; Russian text (DONE in installer 1.3, setup.cpp RussianText) by finding the ENPY files already on the PC
  (Documents\...\override\enpy) and copying the six talk tables into the game folder
  plus Locale=ru-ru (ENPY files are not ours to ship: enpy.net/license/ forbids
  placing them anywhere without the editors' consent; the player chose 2026-11-20
  not to ask for now; without them the installer offers to open
  https://enpy.net/projects/localizations/games/dragon_age_origins/ in the browser
  to download the translation, and to run the installer again afterwards); (3) Steam builds in kProvenBuilds, release, README.
- 2026-11-20 (player: "автоматический пропуск ланчера в первой части"): installer 1.4
  writes the Origins Steam launch option itself (setup.cpp SteamLaunchOption, app id
  in games\dao\game.h kSteamLaunch; Dragon Age II has none). Only for a game in
  <library>\steamapps\common with appmanifest_47810.acf. With a question, it sets
  LaunchOptions = "<game>\bin_ship\DAOrigins.exe" %command% (the player's own options
  kept after it) in userdata\<account>\config\localconfig.vdf, block
  UserLocalConfigStore\Software\Valve\Steam\apps\47810 (made when missing), backup
  localconfig.vdf.before-access. Account: ActiveUser while Steam runs, else accounts
  whose file holds the game, else the newest login in config\loginusers.vdf. Steam
  writes that file when it exits, so a running Steam is closed (steam.exe -shutdown,
  up to 45 s, then "close it yourself, Retry") and started again; both through
  Explorer (ShellRunUnelevated), so Steam never runs as administrator. Record line
  steamlaunch=<account>|<options before>; removal puts the old options back (in the
  "вернуть настройки" question) only where they still start the game exe.
  Proven on the player's PC 2026-11-20 21:50 (test log): Steam closed and reopened,
  option written, game started without the launcher. Published as the installer
  asset of v0.3.0 the same night.
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
- python -I ignores PYTHONIOENCODING and the user site-packages (capstone is
  installed there): use python -I -X utf8 for logs with Russian text, plain python
  for capstone scripts. Never name a scratch script like a standard module (dis.py
  broke capstone's import of inspect, 2026-11-22).
- windows.h defines near and far as macros: a C++ variable named near fails with
  C2513 (scanner.cpp, 2026-11-22).
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
- Never run the test installer from Git Bash: it rewrites arguments that start with
  "/" (/auto becomes a path), so the installer ran without /auto and showed real
  dialogs on the player's screen (2026-11-20 21:40). Run it from PowerShell
  (Start-Process -ArgumentList) and check that the log's first line ends in "auto".

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
- 2026-11-20 ~21:00 (Claude, scratch folder): installer 1.3 test build, Origins copy
  with the six ENPY talk tables in docs\...\override\enpy and DragonAge.ini at
  Locale=en-us: six files copied into the game folder, Locale=ru-ru (VOLanguage
  kept), rufile= x6 and locale=en-us recorded; reinstall asked nothing and kept the
  record; /uninstall deleted the six files and wrote Locale=en-us back (the ini then
  byte-identical to its backup). The no-translation question (Russian Windows, not
  /auto) not seen yet; no dialog tried with NVDA.
- 2026-11-20 ~21:50 (Claude, scratch folder, PowerShell): installer 1.4 test build,
  /auto with /steam:<copy> on three copies of Steam settings: (1) the player's
  localconfig.vdf before the hand edit: one LaunchOptions line added in apps\47810,
  exactly like the hand edit; (2) no account with the game: newest login chosen,
  block "47810" made; (3) LaunchOptions "-novid \"x\"": became "<exe>" %command%
  -novid "x". Reinstall: "уже", nothing written. /uninstall: old options back
  ("" in 1 and 2, the line stays empty). Steam closing / reopening not exercised.
  An earlier run from Git Bash lost /auto and showed dialogs to the player; real
  files checked afterwards: unchanged.
- 2026-11-20 21:50-21:52 (player, real Steam, NVDA): installer 1.4 (admin build,
  /gamekey:dao /source: a copy of the mod files already in the game) with Steam open
  and the Origins launch option removed beforehand by Claude: update chosen, launch
  option question answered Yes, Steam closed through Explorer (-shutdown) and started
  again (21:50:31), LaunchOptions written (localconfig.vdf 21:51:39 after Steam's own
  write), record steamlaunch=1588692629|. Game started from Steam without the
  launcher: mod log 21:51:03 cmdline "<exe>" "<launcher>", "Dragon Age Access
  загружен", 4095 MB address space. Player: "все работает".
