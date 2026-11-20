# Notebook: Dragon Age 2

> Since 2026-11-19 this game is part of the merged project DragonAgeAccess (shared
> core in core\, this game in games\da2\). Shared decisions, the installer and the
> publishing rules are in the root NOTEBOOK.md. Older paths in this notebook such as
> src\scanner.cpp, src\agent\daoaccess.as or tools\x.py now mean core\src\scanner.cpp,
> core\agent\daoaccess.as or games\da2\tools\x.py (shared tools stay in tools\).

## Game

- Name: Dragon Age II: Ultimate Edition (patch 1.04 with the DLC)
- Store and install folder (since 2026-11-20): Steam app 1238040 (build id
  10351532), F:\SteamLibrary\steamapps\common\Dragon Age II. It runs through the
  EA app (EAStore.ini StoreName=Steam, __Installer, bin_ship\Core\Activation.dll).
  The earlier copy in F:\games is deleted.
- Main executable: ...\Dragon Age II\bin_ship\DragonAge2.exe (12 172 664 bytes,
  SHA-256 478357e492859402618e2225df25d6d78a8105ea6f30f548c19602483474ed0a).
  bin_ship\DragonAge2Config.exe is the settings tool; no launcher exe at the root.
  One exe for DX9 and DX11 (DragonAge.ini UseDirectX11Renderer=1).
- Steam build: PE stamp 0x4f1e9fb5 (same as the earlier copy), SizeOfImage
  0xbd2000, no LAA (Characteristics 0x102), signed. In kProvenBuilds since
  release v0.3.0. EA's DRM wrapper: a last,
  unnamed section at RVA 0xBD0000 holds the entry point 0xBD0830 and an import
  table of SHLWAPI / KERNEL32 / USER32 only; .text is encrypted on disk (entropy
  8.0), .rdata / .data are plain. The game's own imports (DINPUT8 among them)
  are resolved by the wrapper after decryption, so dinput8.dll is loaded from
  bin_ship later than in the earlier copy. With UAL's default
  (DontLoadFromDllMain=1, plugins load from hooked kernel32 calls of the exe) the
  mod never started (test 12:56: game ran, no DA2Access.log). Fix, proven
  2026-11-20 13:03: games\da2\loader\global.ini, copied to
  bin_ship\scripts\global.ini by deploy.ps1, make_release.ps1 and so the
  installer: [GlobalSets] LoadPlugins=1, LoadFromScriptsOnly=1,
  DontLoadFromDllMain=0 (key names are strings in UAL 9.7.4's dinput8.dll). Log:
  "loader: dinput8.dll=...\bin_ship\DINPUT8.dll (9.7.4.0)", vtable hooks on,
  start phrase Success, menus read.
- Memory without LAA (Steam, log 13:03 at the title screen): used 883 of 2047 MB
  address space, largest free block 416 MB. Watch in long sessions.
- Earlier build proven on: DragonAge2.exe FileVersion 1.04.8524.0, PE stamp
  0x4f1e9fb5, SizeOfImage 0xbd5000, SHA-256
  253617d98b68457f3c249431da4e888f020210f888f001319881731c1e4bb506 (imports in an
  extra section ".theta"), with the LAA flag set from 2026-11-19.
- 32 or 64 bit: 32-bit (PE Machine 0x14c).
- Engine family (one of: unity-mono, unity-il2cpp, unreal, godot, gamemaker,
  rpgmaker-web-vn, dotnet-custom, native-custom, script-extender-engines,
  emulation, scripted, other): native-custom
- Engine and version: family native-custom, BioWare Eclipse engine (DA2 branch)
  1.04.8524.0. UI is Scaleform GFx 3.x with ActionScript 2 movies (SWF 8). Audio
  is Wwise (AK exports), not FMOD as in Origins. Middleware: PhysX, Umbra, Bink.
- Anti-cheat or DRM: no anti-cheat files. EA's DRM wrapper on the Steam exe (see
  above); the exe is not changed (no LAA), since changing a wrapped exe may stop
  it from starting. The exe carries ".stab/.stabstr" debug sections from
  TransGaming's libcross (the Mac port wrapper); harmless. The game tries EA online features
  (BLAZE login prompt) at start; single-player works without it.

## Stages

Status is one of: to do, in progress, done, blocked (with the reason).

- Recon (game-recon): done 2026-11-18. Verdict Hard but much cheaper than Origins:
  same engine family, same Flash menu system, same dinput8 route. See Decisions.
- Injection (inject): done 2026-11-19 (launched by Claude while the player was
  away; log line "DA2Access probe loaded", see Proven facts).
- Speech (speech): done 2026-11-19 (Prism NVDA backend, "Dragon Age 2 Access
  загружен." -> Success).
- Screen text (screen-text): in progress. The Origins menu reader and Flash agent
  run unchanged on DA2 (agent ver 28 = Origins agent v28). Proven 2026-11-19 by
  Claude's own test runs (keys sent with tools\keys.ps1): press-any-key screen,
  "connecting" text, the EA login screen (all 12 items), the error popup, the
  start menu (Новая игра, Загрузить (disabled: no saves), Настройки,
  Дополнительно, Загружаемый контент, Войдите в сеть, Выход), New Game, the
  intro text, and the first chargen step "КЛАСС И ПОЛ" (PartyPicker movie).
  Hero choice on that step (3D models picked with the mouse): mod item "Выбор
  героя"; Left/Right first sweep the Windows cursor over the screen (about 7 s,
  4 rows x 40 points) and keep the centre of each highlighted character, then
  move the cursor from one to the next; the game itself then shows class and
  description, which the reader speaks. Proven 00:15: "Найдено персонажей: 6"
  (ids 308, 313, 286, 291, 296, 302 left to right), "Воин" + description,
  "Разбойник" + description. Player test 00:17 proved browsing all 6 (Маг,
  Магесса, Воин x2, Разбойник, Разбойница) with class descriptions, but Enter
  (PartyPicker.DoAction 11) did nothing: DoAction needs the gamepad button clips,
  which do not exist on PC. Build 00:27 (deployed, not tested): Enter on "Выбор
  героя" = a real left click (SendInput) with the cursor on the highlighted
  character; in confirm mode Enter sends "GameModeController.ChargenConfirm"
  (what PartyPicker.ConfirmChargen sends). Not tested yet: the rest of chargen
  (chargen_stage2.gfx), options, conversations, the game world.
  Chargen step 2 (CharGen_Stage2 "НАСТРОЙКА", accordion: Внешность, Портрет,
  Имя, События "Начало", НАЧАТЬ ИГРУ): accordion buttons are non-sticky
  (ElementContainer sets m_bStickyButtonEnabled false); enabled =
  ElementContainer.IsEnabled(), Enter = onPressHitArea + onReleaseHitArea.
  Proven by the player 21:36 (build 21:12): history choices read with their
  descriptions, НАЧАТЬ ИГРУ -> difficulty dialog -> game started. Appearance
  sliders (ListEntry_Slider) are read but not adjustable yet.
  Difficulty dialog (PopupLayer DiagPop2_pc_mc.Picker_mc, SharedLibrary.Picker of
  popuplayer.gfx): the picker read "Средне, список, Средне" and its two arrow
  buttons PickerArrowLeft_mc / PickerArrowRight_mc read "без подписи" (log
  21:36-21:37; pressing them did change the level). Party portraits on the
  Character Record (Portraits_mc.Portrait1-3_mc, SharedLibrary.Portrait, token
  Party.Members.N) read "без подписи" (21:41). Fix, agent v29 (deployed
  2026-11-19 ~22:00, NOT tested): arrows no longer listed; Left / Right on the
  picker call Picker.SelectPrevEntry / SelectNextEntry ("Начало списка" /
  "Конец списка" at the ends), spoken "список, Средне, 2 из 3"; portraits read
  the member's Name, ", выбран" for GUI.SelectedChar, ", можно повысить
  уровень" from .PortraitLevelUp (the token the game's own Portrait watches).
  Also read fine in that test: inventory (items, equip with Enter), tutorial
  popups, pause menu, save screen and its name popup.
  Noise filter (build 00:27, not tested): start-menu advert ticker
  (NewContentText), build version (tf_BuildVersion) and unlocalised "?(...)"
  texts are dropped from the item list.
  Shared agent v35 (built 2026-11-20 for the Origins codex, deployed the same evening, NOT tested here): long
  texts with line breaks are read one paragraph per item, a text under two field
  names in one clip is listed once, the pause sign is read from Client.Paused when
  the game answers it (else the old PauseIndicator_mc scan; whether this movie
  has that value is unknown), money alerts of an Alert with Type 5 in coins. Check
  in the next Dragon Age II test: a long text (privacy policy, codex), the pause.
- Scanner (scanner): in progress. Origins scanner.cpp ported unchanged except: runs
  in the CombinedHUD movie (DA2 has no Minimap movie), KeyBindings path "Dragon
  Age 2", position offset found at run time (CalibratePos on the player, log
  "objects: position of object N ... found at +0x.."). New objects.cpp and
  move.cpp with DA2 addresses (see Proven facts "World objects"). Deployed
  2026-11-19 ~00:55, NOT tested. Keys as in Origins: PageDown / PageUp next /
  previous, Home repeat, End walk, Enter interact (talk, open, attack),
  Ctrl+PageDown / PageUp category, ] / [ next / previous enemy, Delete quest
  text, Backspace quickbar, F1-F4 party, Shift+Home party health, F10 dump.
  Note: the game also binds PageDown / PageUp to camera zoom (ZoomIn_1 /
  ZoomOut_1), not unbound here (Origins: unbound with the player's OK).
  Player tests 19:55-20:00 and 21:32-21:41 (prologue fights): scanner, enemy
  list ] [, combat narration, kills, walking (End) work. Quickbar reads
  .AbilityTemplate.ID and .Cooldown.CooldownPercentage: proven 21:32 ("Могучий
  удар", "Коса", "Вихрь", "Готово: Могучий удар"). Attacks in a fight go through
  GameModeCombat; only the first Enter on a new enemy says "Атакую: <name>, N%":
  proven 21:32. Small issue: every ability says "выберите цель, Enter на враге",
  also self-centred ones (Вихрь).
- Autowalk (autowalk): in progress. End walks to the scanner entry (proven 19:57,
  "Иду: Маг" -> "Пришли: Маг").
- Game world so far (player test 00:34-00:36): conversations read (subtitle
  lines, VO English), tutorial popups read ("Бой. Щелкните правой кнопкой...",
  Закрыть). HUD movies: CombinedHUD, Quickbar, FloatyLayer. Before this build there
  was no scanner and no way to fight.

## Resources

Accessibility search, 2026-11-18 (nothing found):

- Molitvan/blind-accessible-games-list README (GitHub API): no Dragon Age entry.
- GitHub repository search (REST API) "dragon age 2" + accessibility / blind /
  screen reader, "dragon age II nvda", "DragonAge2 hook": 0 results each.
- Nexus Mods GraphQL, domain dragonage2, names with access, blind, screen,
  speech, narrator, tts, reader: only cosmetic mods and a load-screen fix
  (4590). extender, hook, dll, loader: 0. "asi" matches unrelated names
  (ThedasIO 4760 is a Blender mesh add-on).
- Web search English and Russian: none. The June 2024 r/dragonage thread about
  accessibility mods for the DA games found none; only a font mod (Nexus 4544
  "Easier to Read") for low vision.
- forum.audiogames.net through web search: no Dragon Age page indexed.

Tools and sister project:

- Sister game: Dragon Age: Origins (games\dao). Its loader, speech, GFx hook,
  Flash agent and menu reader were the base of this mod; since 2026-11-19 they are
  the shared core (core\). Object offsets differ and must be found again in each
  exe. The shared tools (FFDec in vendor\ffdec-26.5.0, tools\as2dis.py,
  tools\methods.py) work on DA2 movies too (both are AS2 SWF 8).
- tools\erf3.py (ours): reads ERF V3.0 archives (DA2 format; Origins used V2.0).
  File names are partly stored only as a 64-bit FNV-1 hash of the lowercase
  name; work\names.json maps 145 of them, recovered by hashing every word found in
  the exe and in the movies.
- tools\exe.py, find_vt.py, slots.py, disf.py (ours): address conversion, string
  xrefs, call graph and vtable listing over DragonAge2.exe (capstone).
- No official DA2 toolset exists (BioWare released one only for Origins);
  community uses the override folder Documents\BioWare\Dragon Age
  2\packages\core\override (exists, empty).

## Decisions

Each decision with its reason, so it is not reopened by accident.

- 2026-11-19: publishing on GitHub, the installer and the merge with the other
  Dragon Age game into one project: see the root NOTEBOOK.md (Decisions).

- 2026-11-20 evening (player: "нельзя это как-то исправить на постоянной основе?"):
  the mod closes the EA app after the game exits. Reason: Steam starts the game
  through the EA app; EASteamLauncher creates DragonAge2.exe suspended and EADesktop
  lets it run. After a game session EADesktop stays alive and stops answering, so
  every later start ends in ProxiedLaunchTimeout 15 s on (EASteamLauncher.log
  13:48 and 20:42: "Game process created suspended", then the timeout, EaAppError)
  and the game never runs. Closing EASteamLauncher and EASteamAuthHelper alone did
  not help (20:42); closing EADesktop and EALocalHostSvc too did (20:45, the game
  started). How: main.cpp StartExitWatcher starts scripts\DA2Access\DA2AccessWatch.exe
  (core\watcher\watcher.cpp; built only for a game.h with kCloseAfterExit), which
  waits for the game process, then CLOSE_AFTER_EXIT_DELAY (30 s, time for EA's
  cloud saves), and closes the four processes of kCloseAfterExit unless the game
  was started again; EABackgroundService (a service) is left alone. Lines
  "exit watch: ..." in DA2Access.log. Cost: an EA app kept open for other games
  closes after Dragon Age II.

- 2026-11-19 (player): the dormant-enemy filter also serves Origins, which uses its
  own proven flag (CCObject +0x2C5 "active"); since the merge both games share it
  as RawObject::inPlay. (The earlier decision to run the two games as parallel
  projects is superseded by the merge, root NOTEBOOK.md.)

- 2026-11-19 (player): no automatic repeat of attacks. DA2 combat is fast and
  action-like: the player presses Enter (attack) and 1-0 (abilities) for every
  blow, as the game intends. The mod only stops saying "Атакую" on every press.

- 2026-11-18: project folder %USERPROFILE%\Documents\DragonAge2Access. The player
  asked to do everything without asking while away; profile (non-coder,
  Russian, NVDA) taken from the Origins project.
- 2026-11-18 recon verdict: Hard (native 32-bit engine, no community framework),
  but the Origins mod transfers: same engine family, same Scaleform AS2 menus
  with the same BioWare AS2 library names (GUISystem, Localization,
  ExternalCommands), same SmartData catalogue, same GameModeExplore /
  CCCreature classes. Main differences: GFx 3.x instead of 2.x (other vtable
  slots, other GFxValue layout), ERF V3 archives, Wwise audio, console
  "GamepadNavigator" in the menus.
- 2026-11-19: reuse the Origins code instead of starting from the kit templates
  (same engine family, same Flash library); only the GFx layer was rewritten.
- 2026-11-19: hero / companion choice on the PartyPicker screen goes through the
  Windows cursor (sweep, then jump between the found characters), because the
  console's own target messages need a current target and the default-selection
  message crashes.
- 2026-11-19 (player): never ask who tests. The player runs the in-game tests;
  Claude drives the game only when told to work fully alone.
- 2026-11-19: the player asked to work without questions while away, so Claude
  installed the mod (additions only) and ran the game itself to test.
- 2026-11-18 injection route: dinput8.dll proxy in bin_ship (Ultimate ASI Loader
  9.7.4 x86, as in Origins). DragonAge2.exe imports dinput8.dll by name; bin_ship
  has no dinput8.dll.

## Proven facts

Each fact with how it was proven (file and line, log line, in-game test).

- Steam copy (2026-11-20): appmanifest_1238040.acf in F:\SteamLibrary\steamapps
  (installdir "Dragon Age II"); the installer's /find finds it through the Steam
  libraries.
- Steam exe addresses, proven 2026-11-20 from the file's plain .rdata / .data
  (the code is encrypted, so no call check as in Origins): GFx movie vtable
  0xC2E514 slots 22 / 28 / 43 = 0x97B2F0 / 0x986550 / 0x988AB0 as in gfx.cpp;
  GameModeExplore 0xBE13FC and GameModeCombat 0xBEEE44 hold code pointers in the
  same pattern; 0xD54494 and 0xD4FA0E lie in .data. Same PE stamp and section
  layout up to .reloc as the earlier copy. The in-game test is the proof for the
  code addresses (move.cpp, objects.cpp).
- Vtable lengths (Steam .rdata, 2026-11-20): GameModeExplore 0xBE13FC 36 function
  slots, GameModeCombat 0xBEEE44 36, then the UTF-16 string "InCombatMode". The mod
  hooked 42 combat slots until 2026-11-20 (fixed to 36).
- DLC in the Steam copy need the EA sign-in (2026-11-20, game log
  Documents\BioWare\Dragon Age 2\Logs\DragonAge_1.log, session 13:03): all 18
  add-ins "Disabled AddIn" (DA2_PRC_DRK, DA2_PRO_FUT01..11, DA2_PRO_COL,
  DA2_PRC_SEB, DA2_PRC_ONE, DA2_MTX_*, DA2_PRC_EYE); the in-game DLC list
  (PRCContentManager) marks each "(Неавторизованный)"; the game connects to BLAZE
  and fetches its config, then waits for a login. A save made with DLC says it
  cannot load without them. Proven 2026-11-20 14:52: signing in with the
  player's EA account in the game (and accepting the policy and user agreement)
  enables all 18; a first try may fail with createPersona cf0001, a second works.
- Level-up (2026-11-20, FFDec exports characterrecord / abilities in games\da2\work\as):
  Character Record (pause menu "УРОВЕНЬ ПОВЫШЕН" / CharacterRecord_mc) reads
  Характеристики, Способности, Тактика, Устойчивость, portraits "можно повысить
  уровень", "Авто. уровень" (AutoLevelButton_mc -> GUICharGen.AutoLevelUp), Назад
  (log 14:53). Attributes: rows AttributesEntry (m_sEntryName, m_nAttributeData,
  m_sMatchingStatName/Data) with StatPlus_mc / StatMinus_mc shown only under the
  mouse, calling CharacterRecord.ChangeAttribute(ListItemIndex - 1, +1 / -1)
  (GUI.SelectedChar.BuyAttribute); a minus only for points added this time
  (AttributeHasPointsSpent); points left CreatureUtils.GetCharacterAttributePoints();
  ConfirmButton_mc (string 374937) shows only when every point is spent
  (IsAttributeConfirmEnabled) and calls ConfirmAttributeSpending; leaving refunds
  unconfirmed points. Agent v33: such a row is a "spin" ("Сила, 15, Урон 12,
  свободных очков 2"), Left / Right add / take back ("Нет свободных очков",
  "Сюда в этот раз очки не добавлялись"). Abilities (abilities.gfx): icons
  BookAbilityIcon with m_oAbility (nDisplayState 1 have, 2 available,
  3 unlockable), OnButtonAction learns / unlearns (Abilities.Learn ->
  GUI.SelectedChar.BuyAbility), Confirm_mc (ConfirmSpentAbilities) appears after a
  purchase; the agent already reads these icons as "ability" (Origins code) and
  presses them with SimulateButtonPush. Built and deployed, NOT tested.
- EA sign-in screen (mod log 13:04): start menu "ВОЙДИТЕ В СЕТЬ"
  (StartMenuButtons_mc.SignInButton) opens movie "login", title "УПРАВЛЕНИЕ
  УЧЕТНОЙ ЗАПИСЬЮ", e-mail box _level0.AccountNameInput_mc.TextField, password box
  _level0.PasswordInput_mc.TextField, "Забыли пароль" (Forgotpw_Link), "Войти"
  (Login_mc). Enter anywhere on it sends the form (an Enter that only started
  typing in the e-mail box gave "wrong user name or password" at once). The game
  focuses the e-mail box when the screen opens.
- After a sign-in with a correct password comes LoginViewTOS (FFDec export of
  patch\loginviewtos.gfx to games\da2\work\as\loginviewtos, class LoginViewTOS):
  two stages in one movie, privacy policy (GUILogin.PPText) then terms of service
  (GUILogin.TOSText), each in ViewText_mc.Scrollable_mc; buttons Accept_mc
  (AcceptAction: stage 1 -> stage 2, stage 2 -> GUILogin.AcceptTOS) and Cancel_mc
  (a confirm dialog, then GUILogin.CancelTOS). Leaving it any other way sends
  CancelTOS (FinishScene).
- Language (2026-11-20): DragonAge.ini [GameOptions] Locale=ru-ru (text),
  VOLanguage=en-us (voice); HKLM\SOFTWARE\WOW6432Node\BioWare\Dragon Age 2
  language=ru; appmanifest_1238040.acf language "russian". Voice folders exist
  only for en-us, de-de, fr-fr; the Steam store API lists Russian as text only
  (full audio: English, French, German).
- Exe identity of the earlier copy: pefile on bin_ship\DragonAge2.exe (Machine 0x14c, no CLR header,
  version resource 1.04.8524.0, sections .text .rdata .data .tls .stab .stabstr
  .rsrc .reloc .theta). Imports by name include dinput8.dll, d3d9.dll,
  binkw32.dll, winmm.dll, version.dll, xinput1_3.dll.
- Engine signatures: 34 "Eclipse" strings, 447 "SmartData", 65
  "GameModeExplore", 29 "CCCreature" in the exe; 2027 RTTI names; RTTI classes
  GameModeExplore, GameModeCombat, GameModeConversation, GameModePartyPicker,
  GUI::ScaleformManager, GFxExternalInterface, GFxFSCommandHandler.
- Scaleform 3.x: exe string "incompatible GFX file, version 3-4.x expected". No
  RTTI for GFxMovieRoot (found through its error strings instead, below).
- Game settings: Documents\BioWare\Dragon Age 2\Settings\DragonAge.ini (ASCII):
  Locale=ru-ru (Russian text), VOLanguage=en-us (English voice: this install has
  no Russian voice), SubtitlesLevel=2, FullScreen=1, 1024x768, DX11.
- Game log Documents\BioWare\Dragon Age 2\Logs\DragonAge_1.log (UTF-16): one
  launch 2026-11-18 23:47 (by the player), all 16 add-ins enabled, BLAZE login
  shown and cancelled, clean shutdown.
- Text: packages\core\data\talktables\core_ru-ru.tlk (+ _f), patch
  packages\core\patch\core_ru-ru_p.tlk.
- Archives are ERF V3.0 (UTF-16 magic "ERF V3.0", header 0x30 bytes: string
  table size, file count, flags (bits 29-31 compression, 1 = zlib), module id,
  digest; then the name string table, then 28-byte entries name offset, FNV-1
  64 name hash, type hash, offset, packed, unpacked). tools\erf3.py extracted
  every file of guiexport.erf and patch\patch.erf.
- Menus: packages\core\data\guiexport.erf holds 71 Flash movies (SWF version 8,
  ActionScript 2), patch\patch.erf replaces 68 of them (the game uses the patch
  copies). Names recovered: startmenu, startmenu2, prestartmenu, mainmenu,
  pausemenu, optionsmenu, chargen, chargen_stage2, conversation, inventory,
  abilities, characterrecord, journal, map, worldmap, store, crafting,
  partypicker, popuplayer, tooltips, floatylayer, combinedhud, quickbar,
  radialquickbar, tactics, saveload, deathscreen, login*, purchaseconfirm,
  itemupgrade, codex-like bookback/bookfront and more. Extracted copies in
  work\gui and work\patch.
- The movies carry the BioWare AS2 library also used by Origins (GUISystem,
  Localization, ExternalCommands, TokenManager, MovieUtils,
  SharedLibrary.SharedMap) plus console navigation classes GamepadNavigator,
  Gamepad, Scene (class names in the SWF __Packages tags).
- GFx movie vtable 0xC2E514 (79 slots), found by error-string call graph
  (tools\find_vt.py) and ret sizes (tools\slots.py), 2026-11-18:
  - slot 21 SetVariable 0x9881D0 (ret 0xC, holds "NULL pathToVar passed to
    SetVariable")
  - slot 22 GetVariable(GFxValue*, path) 0x97B2F0 (ret 8, reaches "GetVariable
    failed")
  - slot 27 Invoke(name, result, fmt, ...) cdecl 0x985640
  - slot 28 Invoke(name, result, const GFxValue* args, UInt n) 0x986550 (ret
    0x10, forwards to 0x9854C0)
  - slot 29 InvokeArgs 0x985720 (ret 0x10, reaches InvokeParsed)
  - slot 43 Advance(Float dt, UInt catchUp) -> Float 0x988AB0 (ret 8, returns
    1/framerate, fld1/fdivrp)
  The object keeps its level-0 movie at [this+0x30].
- GFxValue layout (GFx 3.x, converter 0x9336A0 called by GetVariable):
  +0 object interface pointer, +4 type, +8 value (16 bytes). Types 0 undefined,
  1 null, 2 bool (byte), 3 number (double), 4 string, 6 object, 8 display
  object; 0x40 = managed (string value is a GASStringNode*, its first field the
  char*; the object interface must release it: 0x9331A0 thiscall on the
  interface, args (GFxValue*, data)); 0x80 on input = convert to the low type.
  HYPOTHESIS until the first in-game read: reading the char* through the node.

- Ported from Origins (2026-11-19): src\log, speech, main (probe, crash filter,
  memory line), gfx (rewritten for GFx 3.x), menu (reader), agent
  src\agent\daoaccess.as (compiled to build\da2access.gfx; internal names
  DAOAccess.* kept). Not ported: objects, scanner, move (DAOrigins.exe
  addresses); src\world.h holds stubs. Build: build.ps1; install: deploy.ps1.
- Proof of injection, speech and hook (launch 2026-11-19 00:01, build 00:00:49):
  "DA2Access probe loaded | bits=32 | exe=...DragonAge2.exe (1.4.8524.0,
  ts=0x4f1e9fb5 size=0xbd5000)", "loader: dinput8.dll=...bin_ship\dinput8.dll
  (9.7.4.0)", "gfx: GFx movie vtable checked, Advance hook installed (slot 43)",
  "speech: backend NVDA", speak -> Success, window "Dragon Age II" with
  C:\WINDOWS\SYSTEM32\d3d9.dll loaded. So the GFx 3 vtable slots and the GFxValue
  layout (with the managed-string read) are proven: every screen text came out
  right.
- The Flash agent loads by bare name "da2access.gfx" from the override folder,
  ready 0-63 ms after loadMovie (log "agent: ready in startmenu (ver 28) via
  da2access.gfx").
- Movies seen (_root.Filename): Tooltips, PopupLayer, PreStartMenu,
  LoginConnecting, login, startmenu, BookBack, load_MovieOverlay, PartyPicker.
- Data catalogue (F10 dump in PopupLayer, test 00:04): same SmartData roots as
  Origins (Party, SelectedChar, GameModeExplore, Journal, Area, PartyPicker,
  CharGenCreature, GUICharGen, ClientOptions...). PartyPicker.{PartyPickerMode
  (0 = chargen), HighlightedPartyMember (id, 4294967295 = none),
  ChargenConfirmMode, SelectedPartyMember.0-3, LockedPartyMembers};
  Party.PartyPickerList.N.{State, Tag, Name, ObjectID, Creature} (12 entries in
  chargen). Hero in chargen: CharGenCreature id 136, Name "Гаррет".
- PartyPicker highlight follows the Windows cursor: SetCursorPos from outside the
  game changed PartyPicker.HighlightedPartyMember (test 00:12). The exe imports
  GetCursorPos, ScreenToClient, TrackMouseEvent (user32).
- partypicker.gfx (FFDec): DoAction(button, pressed, repeat) with Gamepad ids
  BottomAction 11 (select under cursor, or ChargenConfirm in confirm mode),
  RightAction 12 (back), LeftAction 13 (confirm party), TopAction 14 (clear).
  Class and description come from 2DA 374 by the highlighted creature's tag.
- GameModePartyPicker message handlers (RTTI, registration at 0x475155):
  SelectRightPartyPickerTarget -> 0x46D070, returns at once while [mode+0xCC]
  is null (no current target), else sets [mode+0xD4]. Sending it from Flash did
  nothing (test 00:07). MakeDefaultSelection sent from Flash crashed inside the
  game (access violation caught by our SEH, reader off; test 00:10): never send.
- login.gfx: its own _root.onKeyDown presses Login_mc on Enter and Cancel_mc on
  Escape. So Enter on any login item also tries to log in (error popup "Не
  удается подключиться..."); Escape closes the login screen and opens the start
  menu (test 00:03).

- Chargen step 1 proven by the player (test 00:32-00:34): sweep found 6, Enter
  clicked (confirm mode 1), "Подтвердить" pressed, the game went on to the
  prologue conversation. No further chargen screens appeared before the
  prologue.
- World objects (disassembly 2026-11-19, tools\handlers2.py, disf.py):
  - client app pointer at 0xD54494.
  - 0x5A70C0 thiscall(app, id) ret 4: object by id (0 when free).
  - 0x459D20 cdecl(obj): cursor action. byte +0x14 type (6 creature, 7 item, 10
    placeable); AsCreature vtable slot 4, AsPlaceable slot 9; creature vtable
    0x1D8 == 1 dead, then 0x5433B0 lootable -> 8; byte +0x404 hostile -> 4;
    +0x791 & 4 no action; +0x791 & 0x40 talk -> 3; item -> 7; placeable +0xEB & 8
    usable, vtable 0x104 (0/1) its two actions -> 8.
  - PerformDefaultActionOnTargetMessage handler 0x4964E0 (registered 0x49EBC5):
    target id from [0xD1D550], action 0x459D20, for actions 2/4/6 writes the
    target id to [[app+0x100]+0xB0], then 0x48D190 thiscall(mode, obj, action,
    0, 0) ret 0x10.
  - MoveCharacterToLocationMessage handler 0x46AD90 (registered 0x474DD5):
    PreMove byte 0xD4FA0E, selected creature 0x5A7220(app), dead check vtable
    0x1D8, point [this+0xFC] valid while byte [this+0x100]; calls 0x459C30
    thiscall(mode, id, &point16 aligned, -1) ret 0xC.
  - Other handlers: PreMoveCharacterToLocation 0x42BEF0, PerformUseObject
    0x480440, SelectNextCreatureTarget 0x46AC90, CenterOnObject 0x42C320.
  - GameModeExplore vtable 0xBE13FC (36 slots, offset 0), GameModeCombat
    0xBEEE44 (36 slots, offset 0; 42 before 2026-11-20 overwrote "InCombatMode").
  - Catalogue attribute ids (_PROPERTIES.as in partypicker.gfx): health 7 (as
    Origins), mana / stamina 10 (Origins 8), level 36 (Origins 15).
  - HYPOTHESIS: object names / tags as VC++ wstrings in the object head (first
    Latin = tag, first Cyrillic = name, 0x20..0x300); the F10 dump shows them.

- DA2 quickbar (quickbar.gfx of DA2, FFDec 2026-11-19): slot token
  GUI.SelectedChar.Stats.QuickSlots.N as in Origins, but the node itself reads ""
  (log "quickbar first read", test 19:55); QuickBarIconSlot.ShowIcon tests
  .AbilityTemplate.ID; cooldown .Cooldown.CooldownPercentage (0..1,
  PieCooldown); .Status 1 = sustained on; .PendingActivation; .IsError,
  .IsWarning; .SourceItemName. Keys: InputMappings.Mappings.ActivateQuickslotN.
- Catalogue: <char>.CombatTarget reads empty in DA2 (no "combat target from
  game" line in test 19:55). Objects.<id>.Attributes.7 health readable for some
  enemies only (164 yes, 182 no).
- Enemies without catalogue health = dormant creatures (objdump of 19:55:28 in
  "Моровые земли", compared with which ids later had readable health or died):
  only 5 of 55 hurlocks were awake then (160, 162, 166, 213, 216); the rest are
  the prologue's reserve waves, placed in piles (50,-68 / 38,-55 / 50,-26) from
  the start, plus the chargen fake heroes 286-313, fake Carver 211 and the frame
  story's Cassandra / Varric. Every enemy attacked without health (182, 186, 198,
  206, 219, 222, fake mage 308) was dormant at the dump; reserves attacked later
  with health (174, 196, 200...) had woken up by then. Awake vs dormant bytes:
  +0x2E7 bit 0x80 clear / set, +0xA50 0 / 1, +0x308 1 / 0, +0x30C 3 / 1, dword
  +0x82C small number / -1. Creatures with +0x2E7 bit 0x80 are left out of the
  scanner, Враги, ] [ and "Следующий", unless the catalogue shows their health
  (logged as "flag contradicted by health"). PROVEN 2026-11-19 21:32-21:35: 65
  left out at the start, then "scan woke:" lines exactly as the waves came
  (174, 176, then 168-172, then ten at once incl. 225 "Гарлок-луч", 326 "Огр");
  after the level change 94 left out, 7 woke when that fight began. Only the
  chargen fake heroes 286.. were "contradicted by health" (harmless).
  What the field is: the dword CCObject +0x2E4 (bit 31 = byte +0x2E7 bit 0x80),
  set to 0x80000000 by the CCObject constructor 0x5DC610 (vtable 0xBF6D4C);
  awake creatures hold a small number there. Its meaning is unproven.
- DA2 tutorial texts (test 19:55): "Щелкните правой кнопкой мыши по врагу, чтобы
  атаковать его"; abilities "воздействуют на текущую цель".

## Game folder changes

- Steam folder F:\SteamLibrary\steamapps\common\Dragon Age II, added 2026-11-20
  by deploy.ps1 (player approved): bin_ship\dinput8.dll (Ultimate ASI Loader
  9.7.4 x86) and bin_ship\scripts\ with DA2Access.asi and DA2Access\ (prism.dll,
  da2access.gfx, third-party\prism\mpl-2.0.txt, NOTICE); DA2Access.log appears at
  the first start; bin_ship\scripts\global.ini (loader settings, 2026-11-20 12:59);
  bin_ship\scripts\DA2Access\DA2AccessWatch.exe (exit watcher, 2026-11-20 20:57).
  The mod files and the LAA exe of the earlier copy went away with its folder.
- Left from the earlier copy and used by the Steam one too (same Documents folder;
  the game ran there 2026-11-20): %USERPROFILE%\Documents\BioWare\Dragon Age
  2\packages\core\override\da2access.gfx (an old agent, replaced by the next
  install) and Settings\KeyBindings.ini with the zoom keys freed (backup
  KeyBindings.ini.before-access).

## Pitfalls

What went wrong and how to avoid it.

- The login screen reacts to Enter by itself (see Proven facts): use Escape there,
  or teach the reader to skip the game's own Enter.
- Test runs by Claude drive the real screen and mouse. During the 00:15 run key
  presses Claude did not send appeared in the log (up, down, reread between the
  scripted Right presses): the player may have been at the PC. Ask before
  driving the game when the player could be present.

- ERF V3 name hashes are FNV-1 (multiply, then xor), not FNV-1a.
- Python scripts that need capstone run without -I (it lives in the user site).

## Open questions

- Crash on exit 20:03:28 (C0000005 in ntdll at 77DA8F56, after the game log's
  "Shutdown - Utilities"): the player closed the game from chargen. The exit at
  21:42 (from the in-game menu) logged no crash. Watch further exits.

- Memory: before LAA the game used 1593-1705 of 2047 MB address space already at
  the title screen and chargen (largest free block 125 MB). LAA set 2026-11-19
  (player approved). Proven 2026-11-19 00:28: "memory: used 1175 of 4095 MB
  address space, largest free 2046 MB".
- Load time varies: one launch showed the first menu 44 s after the probe line
  (other launches 5 s). Fixed sleeps in tools\keys.ps1 then send keys too early.
- The start menu's ticker (LoginAndNewContent_mc NewContentText_mc) changes by
  itself; it was spoken once as "text changed". Watch whether it becomes noise.

- Does GamepadNavigator drive the PC menus (like FocusManager in Origins), and
  can the console UI mode be forced? Settle by reading startmenu / mainmenu AS2.

## Test log

One line per in-game test: date, what was tested, result.

- 2026-11-19 00:01-00:04 (Claude, player away): injection, speech, start screens,
  login, start menu, New Game, chargen step 1 read. All proven. Enter on login
  "Закрыть" also triggered the game's own login (popup).
- 2026-11-19 00:07 (Claude): SelectRight/LeftPartyPickerTarget messages: no effect.
- 2026-11-19 00:10 (Claude): MakeDefaultSelection: access violation inside the
  game, caught; reader off for that session. Removed from the code.
- 2026-11-19 00:12 (Claude): SetCursorPos sweep from PowerShell: highlight follows
  the cursor (5 of 6 ids).
- 2026-11-19 00:15 (Claude): in-mod sweep found all 6 characters; Right moved to the
  next, class and description spoken. Foreign key presses seen in the log; Claude
  stopped driving the game and left it running.
- 2026-11-19 00:17-00:19 (player): browsed the 6 heroes with descriptions (works);
  Enter on "Выбор героя" did nothing (DoAction 11); Back returned to the start
  menu; Extras and the DLC content manager read. Ticker text was noise.
- 2026-11-19 00:32-00:36 (player, build 00:27): hero chosen with arrows + Enter +
  Подтвердить; prologue conversations and the combat tutorial popup read. No
  scanner, no fighting possible (not ported yet then).
- 2026-11-19 19:53-20:03 (player, build 00:42): chargen step 1 again, prologue
  fights (scanner, enemies, kills, End walk work), quickbar always "пуста",
  "Атакую" on every Enter, chargen step 2 read but "НАЧАТЬ ИГРУ, недоступно" and
  no way to start; crash on exit.
- 2026-11-19 21:30-21:42 (player, build 21:12): new game to the end of chargen
  (step 2, history, difficulty), prologue fights with quickbar and single
  "Атакую", dormant waves waking, inventory, Character Record, save. Unlabeled:
  difficulty picker arrows, party portraits (fixed in agent v29, not tested).
- 2026-11-19 00:28 (Claude, build 00:27): LAA proven (4095 MB). Test of the click
  pick interrupted: the player was pressing keys at the same time; Claude stopped.
- 2026-11-20 (Claude, no game run): Steam copy checked from files: same PE stamp
  under EA's DRM wrapper, the GFx vtable slots match gfx.cpp; combat vtable slot
  count fixed to 36. In-game test of the Steam copy pending.
- 2026-11-20 12:56 (player): first start of the Steam copy with the mod: game ran,
  mod silent, no DA2Access.log. Loader settings global.ini added for the next test.
- 2026-11-20 13:03 (player): Steam copy with global.ini: mod loaded (start phrase,
  ts=0x4f1e9fb5 size=0xbd2000), menus read: PreStartMenu, DLC list
  (PRCContentManager, 28 items, details text), start menu, quit dialog. Not yet
  checked: scanner, walking, combat (needs a save in play).
- 2026-11-20 13:04-13:05 (player): EA sign-in screen (movie "login"): read, but
  typing went wrong: Enter to start typing also sent the empty form ("wrong user
  name or password"); the e-mail box kept the game focus, so the password went
  into it and into the log (masked since). Fixed in the shared core (typing follows
  the cursor, password fields, nothing typed is logged); new build deployed, test
  pending. A save from the earlier copy would not load: its DLC is not installed.
- 2026-11-20 13:48-13:50 (player): EA sign-in with the new typing: e-mail and
  password typed (log shows only lengths, password said as stars), Далее sent the
  form. Then the privacy policy screen (movie LoginViewTOS) froze the game for
  45 s ("worst call 44969 ms in LoginViewTOS": the agent's Clean built its
  long text one character at a time), the game dropped back to the start menu
  and the policy could not be accepted. Fixed in agent v32 (Clean with
  split/join, texts over 3000 characters cut); deployed, test pending.
- 2026-11-20 13:57-13:58 (player): the game would not start. EA log
  (%LOCALAPPDATA%\Electronic Arts\EA Desktop\Logs\EASteamLauncher.log): the game
  process was created suspended, no ResumeGameRequest came from EADesktop (in the
  good launches it comes within 10 ms), ProxiedLaunchTimeout after 15 s, error
  "Непредвиденная ошибка" not shown (EA app window hidden). Steam then refused new
  starts (WaitingPrevProcess, AppError_16) since EASteamLauncher stayed alive.
  EADesktop had run since 13:48, i.e. through the 45 s freeze and the sign-in.
  Claude stopped EASteamLauncher, EASteamAuthHelper, EADesktop, EALocalHostSvc
  at about 14:10; not the mod (the game never reached dinput8.dll). Retry pending.
- 2026-11-20 14:50-14:54 (player): the game started again after the EA processes
  were stopped. Sign-in: LoginViewTOS read (header, intro, the policy text cut,
  Принять / Отмена), Принять twice (policy, then user agreement). Game log: BLAZE
  acceptTos, createPersona failed (onCreatePersonaError cf0001, "Не удалось
  соединиться с сервером Dragon Age"); a second sign-in at 14:52 went through
  (loginPersona, OnSignedIn, entitlements) and all 18 add-ins switched to
  Enabled; the DLC list lost its "*" / "(Неавторизованный)" marks, the start menu
  grew to 10 items, and the old save loaded (Lothering intro, character record).
  Seen in passing: the loading text spoken three times; "Травмы. Гаррет: травма
  200000" (a raw number). EA sign-in and DLC proven.
- 2026-11-20 15:33-15:35 (player): level-up. Attributes proven: rows read
  "Ловкость, 10, Вероятн. критич. удара 0%, свободных очков 3, 3 из 6", Right added
  points (Ловкость, Сила, Хитрость), "Подтвердить" (ConfirmButton_mc) appeared at 0
  and confirmed; the game then opened the abilities screen itself. Abilities not yet
  usable: the 7 trees (AbilityClassContainer) read "без подписи", and after Enter on
  one the screen listed no ability icons (the tree, a widget, hid them). Fixed in
  agent v33 build 15:5x: a tree is a button named m_sClassName while
  AlmightyContainer.InChainSelection() is true, otherwise the agent walks into it.
  A training tutorial (TrainingTutorialTopRight_mc, "Закрыть", text on ability icon
  shapes) took the keys until closed; a tester's stuck camera tutorial turned out to be
  in Origins (games\dao\NOTEBOOK.md), not here. Fix (menu.cpp TutorialOnly): a PopupLayer whose only
  widgets lie under TrainingTutorial* is read passively and never takes the keys
  (Origins tutorials sit under TutorialContainer_mc and are not affected). Both
  built and deployed, NOT tested.
- 2026-11-20 20:32-20:45 (player): after quitting the game, Steam starts failed:
  20:32 and 20:42 EASteamLauncher made the game process suspended, then
  ProxiedLaunchTimeout (EADesktop alive since 15:33, not answering). Claude closed
  EASteamLauncher and EASteamAuthHelper: still failed; then EADesktop and
  EALocalHostSvc: the game started (player 20:46). Exit watcher built (Decisions),
  checked with stand-in processes (closed the leftover after the delay; closed
  nothing when the "game" was started again), deployed 20:57. PROVEN 21:07-21:11
  (player: "вторая часть теперь запускается"): two quits, each 30 s later "exit
  watch: closed EADesktop.exe EALocalHostSvc.exe" (EASteamLauncher had already
  gone), and the next start from Steam worked both times.
- 2026-11-20 20:51-20:52 (player, build 18:59): level-up "ужасно работает".
  Attributes: Up from Сила gave "list go 0 of 6 -> missing", Down from Сила воли
  "Конец списка", Телосложение never reached. Cause (FFDec characterrecord.gfx
  SharedLibrary.OptimizedList): in Dragon Age II m_nNumVisibleEntries counts only
  the rows on screen, NumEntries is the full count after the list's Filter, and
  with a Filter ListItemIndex is the unfiltered data index (Сила = 1) while
  VisibleListIndex is the place on screen; the agent read the Origins meaning.
  Fix agent v36 (A.ListCount, A.ListPos, ListGo by place, pitch ItemHeight +
  Spacing). Abilities: inside a tree Up/Down said only "Конец/Начало списка"
  (every icon of a tree sits in one AbilityChainContainer_mc, one chain); Left/
  Right worked. Fix menu.cpp MoveInSection: with one chain on the screen Up/Down
  step through it. Both built and deployed 21:2x, NOT tested. Check: attributes
  list 1 из 6 to 6 из 6 with Телосложение, points still added to the right row;
  in a tree Up/Down go icon by icon; Origins inventory still counts "N из M".
