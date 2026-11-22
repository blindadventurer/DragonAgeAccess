# Notebook: Dragon Age: Origins

> Since 2026-11-19 this game is part of the merged project DragonAgeAccess (shared
> core in core\, this game in games\dao\). Shared decisions, the installer and the
> publishing rules are in the root NOTEBOOK.md. Older paths in this notebook such as
> src\scanner.cpp, src\agent\daoaccess.as or tools\x.py now mean core\src\scanner.cpp,
> core\agent\daoaccess.as or games\dao\tools\x.py (shared tools stay in tools\).

## Game

- Name: Dragon Age: Origins - Ultimate Edition
- Store and install folder (since 2026-11-20): Steam app 47810 (build id
  21245176), F:\SteamLibrary\steamapps\common\Dragon Age Ultimate Edition. The
  earlier GOG copy (id 1949616134, F:\games\Dragon Age - Origins) is deleted.
- Main executable: ...\Dragon Age Ultimate Edition\bin_ship\DAOrigins.exe
  (10 594 144 bytes, SHA-256
  400c2c9e97bb1a534121553be66f202429ef6f7320c93c86f9b7a491864647bd).
  DAOriginsLauncher.exe at the root is the launcher ("Launcher Application"
  1.1.0.14, native, does not import or mention dinput8, so the mod cannot touch
  it); Steam starts it. Test 2026-11-20: the player's OCR found no Play button in
  it, and the game did not start (DragonAge.ini not rewritten after 12:33). Way
  round it, from Steam community posts: Steam game properties, launch options
  "F:\SteamLibrary\steamapps\common\Dragon Age Ultimate Edition\bin_ship\DAOrigins.exe" %command%
  (Steam then runs the game exe, the launcher path becomes an argument;
  proven 2026-11-20 13:12: the mod log shows that command line). Set 2026-11-20 13:07 by Claude with the player's
  permission (see Game folder changes, settings). DAOriginsConfig.exe the settings tool; bin_ship\DAUpdater.exe registers the DLC (it ran 2026-11-20,
  Documents\DAO Ultimate Addins Updater.log).
- Steam build: same code as the GOG build, wrapped by SteamStub variant 2.1
  (section .bind, entry point 0x9ED2EE in it, .text encrypted on disk). PE stamp
  0x4eb19d53 (same), SizeOfImage 0xa49000, no LAA (Characteristics 0x102).
  In kProvenBuilds since release v0.3.0, together with the Steamless + LAA copy
  (SizeOfImage 0x9ed000, Characteristics 0x122; both read from the player's files
  2026-11-20 evening).
  Proven same layout 2026-11-20: its sections .text .rdata .data .tls .rsrc .reloc
  have the GOG addresses (.reloc ends at 0x9EC0A2 = the GOG SizeOfImage); the
  Steamless 3.1.0.5 copy (games\dao\work\DAOrigins.unpacked.exe, not in git, SHA-256
  df7af157...e186) is exactly 10 217 312 bytes like the GOG exe (its hash differs,
  header fields only presumably; the GOG file is gone, no byte diff possible); every
  address of the mod checked in it (Proven facts, Steam).
- Earlier build proven on (GOG): DAOrigins.exe FileVersion 1.05.13263.0, PE
  TimeDateStamp 0x4eb19d53, SizeOfImage 0x9ec0a2, SHA-256
  2346caee611002d4dff6ecef68eb315b63a37004dd98b39c11a9c26c0b45bfb3
  (GOG build 2.1.1.5), with the LAA flag set from 2026-11-17.
- 32 or 64 bit: 32-bit (PE Machine 0x14c).
- Engine family (one of: unity-mono, unity-il2cpp, unreal, godot, gamemaker,
  rpgmaker-web-vn, dotnet-custom, native-custom, script-extender-engines,
  emulation, scripted, other): native-custom
- Engine and version: family native-custom, BioWare Eclipse engine 1.05.13263.0.
  UI is Scaleform GFx. Middleware: FMOD Ex, PhysX, SpeedTree, Umbra, Bink.
- Anti-cheat or DRM: no anti-cheat, single-player. The Steam exe carries SteamStub
  (see above); changing the exe (LAA) is reported to stop it starting (error 51),
  so the Steam copy runs without the 4 GB flag. The GOG build was DRM-free.

## Stages

Status is one of: to do, in progress, done, blocked (with the reason).

- Recon (game-recon): done 2026-11-16. Verdict Hard, route dinput8.dll proxy.
- Injection (inject): done 2026-11-16 (one launch through the player's usual
  path; the next test launch counts as the second check).
- Speech (speech): done 2026-11-16 (Prism NVDA backend, both lines Success in
  the log; bundled with the injection test).
- Screen text (screen-text): in progress. Main menu fully read through the
  FocusManager (test 23:45: Russian labels, top-to-bottom order, Enter works).
  Flash agent proven (tests 2026-11-17 00:05 and 00:25). Covered and proven:
  main menu, Options (tabs, pickers with their lists, toggles, sliders,
  OK/Cancel, Restore defaults), New Game campaign choice. Agent v4 deployed
  2026-11-17 00:45, not tested: off-screen test on the bounds centre (drops the
  slid-out login button), plain text fields and input boxes listed as items
  ("text", "edit"), labels from oTarget.sName (chargen icons), disabled sticky
  buttons. Reader: on a new screen it reads the texts above the first widget
  (dialog messages, titles) then that widget; Tab / Shift+Tab jump between
  widgets; after Enter it reads texts that changed (description panels).
  Proven in test 01:07: chargen RaceGender fully (names via GetTlkString,
  choices, descriptions read after Enter, Next), appearance scene reachable
  (voice set choices work), attributes scene reached. Agent v6 + reader
  deployed 2026-11-17 01:25, not tested:
  - "spin" kind (clip with Minus_mc/Plus_mc: AttributeAdjuster), Left/Right;
  - the game's own hover on every cursor move (DAOAccess.Hover calls
    onRollOverHitArea / onRollOutHitArea), texts it changes read after the item;
  - text input: Enter on an "edit" item calls Selection.setFocus + select all,
    typing echoed from the field's text, Enter or Tab ends (Unfocus);
  - order follows the clip tree (containers stay together, so the appearance
    scene's two columns no longer interleave);
  - a scene change inside a movie (current item gone and over half the paths
    new) is announced like a new screen;
  - Scrollbar* clips skipped.
  Proven in test 01:23: scene announcement, appearance columns kept apart,
  hover-driven attribute descriptions, spin items "Сила, 15", name typing.
  Agent v7 deployed 01:40, not tested: the spin +/- press now really in
  DAOAccess.Adjust (v6 put it in Press by mistake, with an undefined dir, so
  Enter pressed Minus and Left/Right did nothing); Enter on a spin or slider
  says to use Left/Right; a changed bare number is read with the text before
  it ("Расходуемых очков, 4").
  Proven in test 01:35: the whole new-game path is playable by ear, from the
  main menu through every chargen step (attribute points spent, skills and
  talents chosen, difficulty popup with its picker) into the first
  conversations (responses read and chosen).
  Agent v8 proven in test 01:56: "Продолжить" loads the last save, ability
  icons named with state ("Силач, изучено", "Запугивание, требования не
  выполнены") and their descriptions read on hover, pause spoken ("Пауза",
  "Пауза снята"), HUD movies no longer take the arrows, in-game Skills
  (abilities.gfx) and Character Record screens read.
  Paused 2026-11-17 by the player's choice to work on navigation first (menus
  are usable; the rest is polish done gradually). Not done: save/load list
  screen, inventory and other in-game screens checked one by one, NPC lines
  (voiced in Russian; subtitles off on this PC), Options two-column order. Options are read row by row (no column
  containers there), so its two columns still interleave.
- Scanner (scanner): in progress since 2026-11-17. Game shape: 3D world,
  party-based, real time with pause. F10 catalogue dump proven (test 02:19).
  Scanner v1 proven in test 02:31: src/scanner.cpp, runs in the Minimap movie's
  Advance, sources from agent DAOAccess.World() (map notes, quest
  destinations, quest givers, party members); keys ] next, [ previous, Home
  repeat. Speech "<kind> <name>, N м, <direction>, k из n": 11 entries in
  Castle Cousland (e.g. "Главный зал, 15 м, сзади", "цель задания, Фергюс,
  119 м, сзади слева"). Not covered: ordinary NPCs, doors, containers, items
  (need engine object lists).
  Walk proven in test 02:42 (src/move.cpp, MoveCreatureTo): the leader walked
  to "Главный зал" (9 m in 2 s, then "рядом") and then 105 m to Fergus, where
  his conversation started. Scanner v2 deployed 03:25, not tested: keys moved
  to PageDown next, PageUp previous, Home repeat, End walk, Ctrl+PageDown /
  Ctrl+PageUp category (Все, Места, Задания, Отряд); walk watch says "Пришли"
  within 2.5 m, or "Остановился" with distance and direction after 1.5 s
  without movement.
  Scanner v3 deployed 2026-11-17 04:30, not tested: engine objects added
  (src/objects.cpp: every id 1..8000 through the game's lookup; CCCreature as
  people, usable CCPlaceable as objects, CCTriggerAreaTransition as exits,
  CCItem on the ground as items); names from the catalogue
  "Objects.<id>.Name" (HYPOTHESIS), else from the object's Name attribute in
  memory, else a word from its tag (door, chest...), else the kind. Categories
  Все, Люди, Объекты, Выходы, Места, Задания, Отряд. Enter = the game's default
  action on the selected object (talk, open, use, pick up, attack; the game
  walks there itself), places fall back to walking. F10 now also dumps every
  engine object with class, type, position, usable flag, memory name and tag,
  and the catalogue answer. Proven in tests 03:30 and 03:53: names come from
  the catalogue "Objects.<id>.Name", hostility from "Objects.<id>.IsHostile"
  ("враг, Гигантская крыса"); "IsDead" gave nothing, so dead rats stayed in the
  list (player report).
  Scanner v4 (deployed 04:40, not tested): dead creatures (CCCreature vtable
  slot 0x1D0 = health <= 0) leave the list, lootable bodies (0x6AB100) stay
  as objects "<name>, тело"; category Враги (live hostiles); ] / [ jump to
  the next / previous nearest enemy, re-sorted on every press; Enter on an
  enemy attacks through the combat mode instance when Party.Leader.InCombat
  is 1 ("Атакую: ...").
  Scanner v4 tested 2026-11-17 16:55 (see test log). Scanner v5 deployed
  2026-11-17 17:45, not tested (player: "unclear what to interact with to move
  on"):
  - Delete reads the current quest's journal text ("Задание «Name»: text";
    child plots as "Parent — Child"), again within 15 s the next open quest;
    order = most recently changed first (before any change: journal Updated
    mark, then Journal.ActivePlot). Agent v15 DAOAccess.Quests.
  - While exploring the journal is polled every 2 s; a new or changed open
    quest text is spoken (queued). Silent baseline at load and area change.
  - Empty "Задания" category speaks "отметок на карте нет" plus the quest text.
  - Quest targets carry their quest's name: "цель задания «Поручение отца»,
    Фергюс"; nameless targets are "отметка".
  - Doors (and exits) get the nearest map note within 12 m: "Дверь, возле:
    Комната Фергюса" (HYPOTHESIS that notes sit in their rooms).
  - ] / [ only cycle enemies within 40 m ("Врагов поблизости нет").
  Proven in test 17:45: Delete, quest change read-out ("Задание «Предательство
  Хоу»: ..." after the conversation), door labels ("Дверь, возле: Ваша
  комната"), enemies within 40 m. Flaws fixed in scanner v6.
  Scanner v6 deployed 2026-11-17 18:13, not tested:
  - Quest order for Delete: quests with a marker on this map first, then those
    whose resref starts with the area's module prefix (first 3 letters of the
    map note tags, "bhn"), then most recently changed. Reason: the tracked
    Journal.ActivePlot was the Shale DLC offer "Сулкеров перевал"
    (shl000pt_main, always Updated) while in Castle Cousland. The order and
    prefix are logged ("quest hint prefix ..."); the baseline logs every plot.
  - A quest marker that is a map note takes the note's name (quest:1074 =
    note:1074 "Главный зал").
  - Creature hostility re-read every 4 s (Howe's soldiers turned hostile while
    the cache still said friendly).
  - Combat narration (agent v16 DAOAccess.Combat): "Бой! Врагов рядом: N.
    Ближайший: ..." with the category set to Враги and the nearest selected;
    the target's health spoken at each quarter lost; "Убит: X. Следующий: Y"
    (selected, Enter attacks); own health at each quarter lost ("опасно" at 25%);
    "Бой окончен. Здоровье N%"; Home in a fight = status (own health, target,
    enemies near); Enter attack says the target's health. Enemy health via
    Objects.<id>.Attributes.7 is a HYPOTHESIS (log shows whether a % is spoken).
  Quickbar voice (agent v17, deployed 2026-11-17 18:34, not tested; asked by the
  player): Backspace lists slots 1..0 ("1: Удар щитом, готово. 2: ...,
  перезарядка 40%"); the game's own 1..0 are echoed (name, or "перезарядка N%",
  "недоступно", "ячейка пуста"); polled every 500 ms: "Готово: X" when a
  cooldown ends, "Включено/Выключено: X" for sustained abilities, "X: выберите
  цель, Enter на враге" when the game waits for a target (HYPOTHESIS that our
  Enter on an enemy then uses the pending ability). The first read is logged
  ("quickbar first read").
  Proven in test 19:21: combat narration (start with nearest enemy, enemy
  health 72/43/15%, "Убит: ... Следующий: ...", own health, end), so
  Objects.<id>.Attributes.7 works for enemies; quickbar keys echoed, "Готово",
  "Включено/Выключено" for Расчетливый удар; key 2 Удар щитом went pending and
  Enter on the enemy used it (cooldown 45% right after). Selecting a companion
  (F1..F4) was silent: the quickbar then showed Рык (dog) and Быстрая стрельба
  (Eleanor, "недоступно" without a bow).
  Party v1 (agent v18, deployed 2026-11-17 19:29, not tested): F1..F4 /
  Shift+F1..F4 followed by the selected name and health ("Тэйрна Элеанора,
  здоровье 100%", "Выбраны: ...", "Выбран весь отряд"); the quickbar re-reads
  silently for the new character; Shift+Home = everyone's health; in a fight a
  companion's health is spoken below half ("пупсик, здоровье 40%", "без
  сознания"); Enter on an enemy while an ability waits says "Удар щитом:
  Солдат Хоу, 80%" instead of "Атакую". The log line "party select id ...
  leader ..." shows whether Party.Leader follows the selection (the scanner
  measures from Party.Leader).
  Menus: a pressed button that turns disabled right after (chosen
  conversation reply) no longer says ", недоступно".
  Scanner v7 (2026-11-17 23:40, not tested; from the review of the logs of
  20:47-22:50, player chose "all of it" and asked to hide статисты):
  - categories Все, Персонажи, Статисты, Объекты, Выходы, Места, Задания,
    Отряд, Враги. Friendly creatures split by the engine's own cursor code
    (0x70CAD0, disassembly): +0x7F0 non-zero = no action; byte +0x860
    non-zero = talk. Talkable = Персонажи, the rest = Статисты (not in Все).
    Runtime proof pending: F10 dump now prints "talk" per object.
  - People / статисты with the same name 3+ times fold into one entry at the
    nearest ("Солдат, всего 28, ближайший, 5 м"); End/Enter act on it.
    Ostagar before: Все 114-128, 28 "Солдат", 14 "Пес мабари".
  - Map notes with icon ico_mm_exit are exits (Выходы was empty 5 times).
  - Walk: ends silently on an area change; a stop within 5 m is "Пришли";
    a stop farther repeats the order up to 2 times ("order repeated" in the
    log) before "Остановился".
  - Combat: "Не получается достать: X. Следующий: Y" when an attacked target's
    readable health does not change for 12 s, or, when its health cannot be
    read, it still stands 45 s after our first attack; such enemies leave
    Враги, ] [ and "Следующий" until the area changes (test 20:58: creature
    1714 attacked 53 times). The controlled character at 0% is "<name> без
    сознания". Leaving out every enemy without catalogue health was wrong:
    test 23:32 in the Korcari Wilds had 58 such darkspawn and the fight began
    with a bare "Бой!" (fixed 00:30).
  Proven in test 23:30-00:01 (Korcari Wilds): categories Персонажи 9,
  Статисты, Все shorter (55-105); walks arrived (Умирающий солдат, Генлок 27
  m); menus open in 0-80 ms (agent ready after 0-31 ms, identified after
  62-78 ms).
  Party v2 (agent v22, 2026-11-18 00:30, not tested; player: companions do not
  follow, often alone in fights): the game's click-to-move handler 0x70C9A0
  does exactly what move.cpp does (0x70BE80 for the selected creature, then a
  sound), so following is the companions' own behaviour; the log of 23:44
  shows Джори still at 121 m for 15 s while the player stood, i.e. not
  following at all. Proven in test 00:08: "party first read ... hold "1""
  at load, the player pressed H (hold 0), and companions then stayed within
  1-7 m for the whole session (46-51 m only during the scripted battle).
  Party.HoldPositions 1 = holding (proven).
  Now Party.HoldPositions is read with the party (logged in "party first
  read ... hold") and spoken on change and once after loading when on;
  Shift+Home adds each companion's distance; Shift+End orders every
  companion farther than 4 m to walk to the controlled character.
  - Quest givers: the game marks none in Ostagar or the castle (no giver
    entries in any log; Journal.PlotGivers empty), so no "gives quests"
    category is possible from that data.
- List screens v1 (2026-11-17 19:53, not tested; player: inventory and skills
  lists are inconvenient, chose "everything in one long list" and "noise" as
  the problems and sections on Left/Right as the fix): in inventory (also
  containers), abilities, CharacterRecord, LevelUp and store, Left/Right and
  PageUp/PageDown switch sections (Рюкзак, Экипировка, Характеристики,
  Фильтры, Персонаж, Атрибуты, Описание, skill classes such as "Воин", Прочее),
  landing on the item last visited there and reading the section's texts
  ("Рюкзак 9 / 70", "Персонаж, Айдан, Деньги, 0"); Up/Down stay in the section
  and add "N из M"; Home/End = first/last of the section. Skill icons
  everywhere (chargen too): Up/Down chain to chain keeping the step, Left/Right
  along the chain, "2 из 4". Left/Right on spins and sliders still adjust.
  Noise removed: paperdoll HitArea_mc "без подписи", "Опыт" caption, "Наведите
  мышь..." hint, faded clips (_alpha 0: "Инвентарь заполнен"), HTML in labels;
  a bare number joins the text before it in the same clip ("Сила, 17",
  "Уровень, 2"); slots in body order. The skills screen opens on the first
  skill; a one-time hint "Разделы: стрелки влево и вправо".
  Proven in test 20:03: inventory sections (Рюкзак, Прочее, Персонаж, Фильтры,
  Экипировка in body order, Характеристики) with "N из M", character record
  sections ("Сила, 17"), skills screen chains, level-up attributes spent with
  Right, talents learned; F1..F4 announce the selection and companions' health
  in fights ("Тэйрна Элеанора без сознания"). Party.Leader stays 130 (the hero)
  whatever is selected. Player: switching branches on the level-up screen is
  awkward (Down stopped at the last chain of "Воин", the player used Tab).
  Fixed 20:35, not tested (agent v20): on list screens Up/Down on skills run
  through every chain of the screen and name the branch when crossing
  ("Парное оружие"), "Конец списка" / "Начало списка" at the ends, "Конец
  цепочки" / "Начало цепочки" on Left/Right; the level-up talents scene, which
  is announced before its icons exist, jumps to the first skill within 3 s
  saying the points ("Умения, 1"). The scanner, walk, Enter and combat health
  now use the controlled character (A.Me = GUI.SelectedChar, else the leader).
- Companions and lists v1 (agent v25, deployed 2026-11-18 ~16:00; tested 16:19-17:37,
  see test log; player asked about gifts, approval, the double Морриган in the camp and
  missing list entries):
  - Shift+Delete in the world: every Party.Approval entry spoken ("Одобрение.
    Морриган: +12, Нейтральное. ..."); log line "approval ...".
  - Inventory and record: a companion's approval bar read as a text line
    "Одобрение: +12, Тёплое".
  - Gifts read as "<name>, подарок"; Enter on one in the backpack with a
    companion selected gives it ("Подарено: X, Морриган"), with the hero
    selected says how to pick the companion.
  - Level-up specializations selectable (kind "spec": "Берсерк, специализация,
    можно выбрать / выбрана / не открыта... / недоступна"), Space = description.
  - Store rows are items with price ("цена 2 зол. 5 сер.", "не хватает
    денег", "продать за ..."), Enter buys or sells.
  - Scanner doubles (Dedupe in scanner.cpp): a map note named like a creature,
    a companion camp spot note (tag gen00fl) with a creature within 6 m, and an
    exit note with a same-named object within 25 m are dropped; log "scan doubles
    dropped: ...". Proven 16:19 (camp: 12 entries, notes Пес/Алистер/Морриган and
    the exit note "Карта мира" dropped). The v25 rule "same-named object within
    5 m" hid separate bodies and crates in Lothering and was removed in v26.
  - Proven 16:19-16:39: Shift+Delete "Одобрение. пупсик: +100, Дружелюбно.
    Алистер: +4, Нейтрально. Морриган: +28, Интерес" (Party.Approval holds every
    hired companion, not only the active party); the inventory line "Одобрение:
    +100, Дружелюбно". Store prices, buying and selling proven 17:35-17:36.
    Gifts proven 20:55-21:27 (Enter gave the puppet to Алистер, the Alistair doll
    to Морриган, the lute to Лелиана, +50 each); specializations not tried yet.
  - Feastday keepsakes (agent v28, deployed 2026-11-18 ~21:55, not tested; player:
    "gifts already given are still read in my backpack"): a backpack item whose
    tag starts with val_im_ / val_arm_ (not val_im_gift_) and is no longer base
    109 reads ", праздничный подарок, уже вручён" (+ ", можно применить" for
    val_im_qck_ with an ability). See the proven fact "Feastday gifts".
- Store and quests v2 (agent v26, deployed 2026-11-18 17:50, not tested; player:
  the store is one long list, Sten's quest gives no clue where the revered mother
  is, the camp scanner lists emissaries not there yet, Flemeth listed as enemy):
  - Store sections with Left/Right: "Купить" (StorePage_mc, StoreName_mc),
    "Товары купца по видам" (CategoryTabs_store_mc), "Продать" (PartyItemWindow_mc,
    ProgressBar_mc), "Свои вещи по видам" (CategoryTabs_mc), "Прочее" (money,
    close, sort). Cause: SectionMode checked "store", the movie is "Store".
  - Money fields as words ("Деньги, 3 зол. 29 сер. 21 мед."; popup "25 сер.").
  - The sale/buy quantity popup's Slider_mc is a spin "Количество, 1 из 2",
    Left/Right press its ArrowDown_mc / ArrowUp_mc.
  - Hidden creatures: creatures at a negative position more than 1000 m away
    are left out (Flemeth at -58, -18). The v26 filter on byte +0x23C was
    removed: DISPROVEN, see Proven facts.
  - Inactive creatures (ported from Dragon Age 2's dormant-enemy filter, build
    2026-11-19 ~22:10, deployed, NOT tested in game): living creatures whose
    CCObject byte +0x2C5 ("active", see Proven facts) is 0 are left out of every
    scanner list (Все, categories, Враги, ] [, Следующий), unless the catalogue
    lists them (party). Expected: Lothering's ambush refugees, bears, wolves,
    hurlocks and the outdoor Leliana gone until their scene; companions parked
    outside the party gone; camp emissaries gone until they arrive. Log lines
    "scan inactive left out: N: ids | inactive objects kept: N: ids" (inactive
    usable placeables and exits are only logged, still listed) and "scan woke:
    id:name" when a creature seen inactive becomes active.
  - Raw memory of every live creature once per area per session goes to
    bin_ship\scripts\DAOAccess.objdump.txt (0xC00 bytes each, cap 6 MB).
  - "Задания" also lists people, objects, places and exits whose name matches a
    word of an open quest's text ("по заданию «Узник кунари», Стэн"): each name
    word of 3+ letters must share its first 3-6 letters with a text word; bodies
    and enemies excluded. Enter / End act as on the original entry.
  - Walk cancel (player request 17:55, deployed 18:00): a movement key
    (KeyBindings.ini [Key Bindings] RunCharacterForward/Backward/Left/Right _0/_1,
    here W S Q E; A/D only turn the camera) while the walk watch runs ends it,
    "Автоходьба выключена", log "walk cancelled by movement key ...". Before, the
    watch saw the stop after the player's own steps and repeated the order.
    HYPOTHESIS: the game's keyboard movement drops the click-to-move path itself.
  - Fixes after test 18:10-18:27 (deployed 18:40, not tested):
    - Mentions only for people, objects, exits and notes whose name is unique in
      the area (per kind); a one-word name must appear capitalised inside a
      sentence of the quest text (a name, "Стэна"); multi-word names match every
      word; one entry per name. Before: 40 "Беженец", 3 "Храмовник", "Разбойник",
      "Торговец", "Купец" were listed.
    - A map note goes when an object or exit of the same name is within 25 m
      (note:14813 "Церковь" beside the door object:14895 "Церковь", "Доска
      певчего"); before only exit notes did.
    - Enter on a map note uses the nearest door or exit within 10 m of it ("Вход:
      <name>", log "note ... uses nearby ..."); no door: walk as before.
  - Categories v8 (player 18:55: church found "somewhere", "Купец" in Места,
    loot scattered; deployed ~19:10, not tested): Все, Персонажи, Статисты,
    Добыча (new), Объекты, Выходы, Места, Задания, Отряд, Враги. Usable
    placeables sorted by the game's own action (CCPlaceable slot 0x100 with 0/1,
    see Proven facts): 15 Area Transition -> Выходы; 11 Loot, or 1 Open / 3
    Unlock with a container word in name or tag (сундук, ящик, бочка, мешок,
    chest, crate, barrel, sack...) -> Добыча; lootable bodies -> Добыча; the rest
    Объекты. Rebuild log lines show "act=a/b" per placeable. A shop note
    (ico_mm_store) gives the person within 10 m ", торговец" and goes. Lothering
    18:51 before: Объекты 44 (crates, herbs, the church door "Церковь", "Убежище
    Дейна", body bags "Разбойник", "Мабари"), Места 1 ("Купец"), Выходы 1.
  - Object source (2026-11-22, player's choice "Сканер из списка игры"):
    EnumObjects (games\dao\src\objects.cpp) takes the ids from the engine's own
    object manager (hash map, Proven facts "Object manager") and reads each one
    through the proven lookup 0xA58190; the old id walk (1 upward until 30000
    free ids in a row; 4000 had missed the Lothering chantry's people at
    16927..17018, 8000 the Ostagar darkspawn) is only the fallback, used when the
    code bytes differ (log "engine object list code differs") or the list cannot
    be read ("engine object list unreadable"). Until proven, once a minute both
    run and the log compares them: "objects: list N ids, M read in X ms; walk K
    in Y ms; only in list: ... | only in walk: ...". PROVEN in test 2026-11-22
    15:54-16:25 (Ostagar): "engine object list on", 10 comparisons, list and
    walk read the same objects every time (814..978), "only in" both empty.
    NEXT: drop the once-a-minute walk comparison. Raw dumps wait 5 s after an
    area change.
  - Fixes v9 (player's report 2026-11-22 on the play of 2026-11-21 22:14 - 03:21,
    causes read in that log; player chose "all in one build"; agent v39, built
    and deployed 2026-11-22 14:50 together with the object source above, NOT
    tested):
    - Shock never cast (key 4 20+ times 02:24-02:28, no "выберите цель", Enter
      attacked): Shock (ABI_base 14000, TargetType 126, AOE_Type 2 cone) enters
      the game's targeting state and waits for a mouse click (Proven facts,
      Ability targeting). Now the quickbar poll also reads that state
      (PendingAbility, 0xC35FA0) and says "Шок: выберите цель, Enter на враге";
      Enter on a creature while an ability waits casts it through the game's own
      click code (CastPendingOn: valid-target check 0x70D550, then 0x711680),
      else at the creature's place; speech "Шок: <target>, N%" or "Нельзя
      применить на эту цель". Log "cast pending: ability A on id I mode M -> r".
      Dragon Age II: stubs (returns 0), unchanged behaviour.
      Test 2026-11-22 16:19-16:25 FAILED: "выберите цель" was spoken, but all 24
      Enter presses gave "cast pending ... mode 1A9D0090 (the combat mode) -> -1"
      (a fault caught by __try), the ability stayed pending; Shock 14000,
      Lightning 14001 and 200254 alike; the player heard "Нельзя применить на эту
      цель". Fixes v10 (built 16:36, NOT deployed): read again, the handler
      0x711A00 takes its object with 0x5D5BE0([0xC06794]) and passes target
      [obj+0x10] (0x53CC40 on obj+4) to 0x711680, not the id. The creature case
      now sets 0xC06794 = id, calls mode->0x711A00 itself (bytes checked) and puts
      0xC06794 back; -4 if the ability is still pending afterwards ("Игра не
      приняла цель"). A fault now logs "step S; fault CODE at ADDR, stack <exe
      return addresses>" and says "Сбой мода при применении". GameModeCombat
      derives from GameModeExplore at offset 0 (RTTI, so the explore functions
      are valid on it). The cause of the v9 fault is unknown (HYPOTHESIS: the raw
      id instead of [obj+0x10]). Fixes v10 deployed 16:43 (player's OK) and
      PROVEN 2026-11-22 17:30: "cast pending: ability 14000 on id 4448 (combat)
      -> 1 step 6", "Шок: Гарлок, 100%", floaty "Шок", the Hurlock 60% then 28%
      and killed; Lightning 14001 the same ("Молния: Гарлок, 72%" -> 44%). Left:
      0.2 s after the cast the quickbar poll still said "Шок: выберите цель,
      Enter на враге" once; fixed in scanner.cpp (g_castValue: the cast ability
      is not "pending" for 1.5 s), built 2026-11-22, NOT deployed (game running).
      The player confirmed "шок работает" the same day. Frida could
      not attach to this game (TransportError timeout at 17:28, twice), not
      needed in the end.
    - Enemies missing from Враги: (1) the "Не получается достать" clocks stop
      while Client.Paused (agent Combat 7th field) or when polls are over 2 s
      apart (menus); a marked enemy within 5 m is unmarked ("combat reachable
      again"); before, spiders 1731, 1724, 1727 were marked while the player
      paused (02:28:07) and then fought at 2 m, only in Все; (2) at the fight's
      start every cached hostility answer is aged and in a fight creatures are
      re-asked every 1.5 s (was 4 s): Часовой 1283 was friendly in the cache at
      03:06:41 ("Бой!" with no enemy); "Бой!" with no enemy looks again 5 times
      every 0.7 s and says "Враги: N. Ближайший: ..." (log "combat enemies
      late"); (3) a quest target or giver that is a live hostile creature gets
      Entry::foe: listed in Враги, "враг, цель задания «...», X" (the Sloth demon,
      quest:320, was only in Задания).
    - "Предметы получены" now says "Получено: X, Y, 2 шт." from the backpack and
      plot item lists' NewlyAcquired items (agent NewItems), each item id once per
      session, at most 6 then "и ещё N"; the alert's text when none is found. Log
      "items alert: lists <sizes>; new: id:name xN...; told K". DISPROVED in test
      2026-11-22 16:25:06 and 16:25:12 (herbs Корень смерти, Полевой цветок):
      "lists 0 0", only "Предметы получены" spoken; the game showed the name as a
      style 0 floaty only for the second one. Fixes v11 (player chose "item
      names first", built 2026-11-22, NOT deployed, NOT tested): a hook on the
      game's own item-added handler (Proven facts, Received items): every item
      the party gets is logged "item added: "<name>" xN" and queued; the alert
      says "Получено: X, Y, 2 шт." from the items of the last 4 s (log "items
      alert: hook N items, told K"), else the agent lists, else the alert text.
      Startup log: "objects: mode capture ... items=on". HYPOTHESIS: the name is
      already set when the handler runs, and StackSize +0x5FC holds the count.
      Same build: no "выберите цель" for 1.5 s after the mod cast an ability.
    - Placeables say their state or verb after the name (scanner, from act=):
      doors "закрыта / открыта / заперта" (gates plural), others "заперто",
      "открыть", "использовать", "только осмотреть", "ловушка, обезвредить"...;
      exits and loot say nothing more (locked loot: "заперто"). Enter says
      "Открываю / Закрываю / Обыскиваю / Использую / Осматриваю / Переход: X" and
      "Заперто, пробую открыть: X" for a locked one (was silent: "Вторая дверь",
      7 presses). Counts in that play: doors closed 15, open 20, locked 6; Use 4;
      Examine 21; Loot 54; transitions 6.
    - Ambient lines (player 2026-11-22: subtitles of people talking among
      themselves, or somewhere else, are not read; conversations are): they are
      FloatyLayer floating texts (Proven facts, Floating texts), which the mod
      only scanned for the pause sign. Agent v40 wraps
      FloatyLayer.DisplayFloatyMessage and DisplayStatusMessage (queue in
      FloatyLayer.__daoaccessQ); menu.cpp SpeakFloaties speaks styles 3, 0, 4 and
      the status line, queued, the same text once in 4 s; hit numbers (1, 2) and
      texts without letters only logged. Log "floaty style S id N "text"". The
      speaker is not named (the message floaty's id is not proven to be a
      creature id). Deployed 2026-11-22 15:31 with everything above. PROVEN in
      test 2026-11-22 15:55: 17 ambient lines (style 3, the Ostagar priest's
      sermon, the soldiers) and 52 style 0 texts ("Оглушен!", talents) spoken;
      one "floaty: no FloatyLayer class" was a movie without that layer.
- Party selection v1 (2026-11-18 ~20:00, adding proven 21:37, see test log; player: only "Удалить все"
  and "Принять" readable): the PartyPicker screen now starts with one item per
  candidate from the game mode's own list: "Морриган, уровень 6, в отряде /
  не в отряде, закреплён сюжетом, 2 из 5"; Enter adds or removes ("В отряде:
  X", "Убран из отряда: X", "В отряде уже четверо...", "...сюжет не даёт
  изменить"). Log lines "party picker: N chosen; id=state..." and "party toggle
  id ... -> code". Level from Objects.<id>.Attributes.15.Current is a
  HYPOTHESIS (left out when empty).
- Injuries v1 (agent v27, 2026-11-18 ~20:20, proven 21:40, see test log; player asked to hear
  injuries): A.Party adds each member's injuries (A.Injuries: EffectList entries
  with AbilityID >= 500000, name from 2DA 160 NameStrRef through Get2DAValue and
  GetTlkString). Spoken: after a load "Травмы. Айдан: <name>." once; later "X
  получает травму: <name>", "X: травмы вылечены"; Shift+Home adds ", травмы:
  ...". Log "party injuries ...". Whether Get2DAValue exists in the Minimap
  movie is unproven (fallback "травма N").
- Level-up screen (levelup.gfx) read in test 17:51: header, specialisation
  icons as text, attribute rows, talents with descriptions; a talent was
  learned ("Умения, 0"). Flaws fixed 18:34, not tested: every talent said
  "можно изучить" (the movie carries the CharGen class with CreatureID 0, so
  HasAbility asked creature 0; now LevelUp.CreatureID first); bare numbers
  ("3" points left) read with the text before them ("Характеристики, 3").
- Autowalk (autowalk): in progress. The game's own click-to-move path is
  proven (see Scanner); arrival and stop announcements deployed, not tested.
  Interaction (Enter) proven in test 03:30 (talk to a guard, open a chest).
- Inventory (player request, 2026-11-17): agent v12 deployed, not tested:
  items read as "<name>, N шт., надето: <who>", Enter = the game's own
  double-click action (equip, use, take from a container), equipment slots
  read as "<slot>: <item>" and Enter takes the item off, Space reads the
  item's tooltip (stats and description). Space in a conversation repeats the
  other person's line; unvoiced lines are read automatically.
  Quickbar (agent v13, deployed 2026-11-17 04:10, not tested): Ctrl+1..Ctrl+0
  on an item with an ability (poultices, potions) or on an ability icon sends
  the quickbar's own drop message "GUI.SelectedChar.SetQuickslot <charId>
  <slot0> <abilityId> <itemTag> " (quickbar.gfx QuickBarIconSlot
  .SetQuickslot; items without an ability have AbilityID 200000 =
  ITEM_ABILITY_INVALID). Needed for the quickbar training step. Proven in
  test 03:54 (poultice put on slot 10).
  Inventory v2 (agent v14, deployed 2026-11-17 04:40, not tested): the player
  found the screen disorganised (test 03:54: unlabeled stat numbers, dog and
  Shale slots listed for the hero). Now: slots filtered by
  GUI.SelectedChar.EquipmentLayout (0 default, 1 Shale 14-17, 2 dog 18-19);
  stat numbers prefixed from their clip name (Здоровье, Мана/Выносливость by
  Stats.ClassHasMana, Усталость, Броня, Защита, Атака, Урон, Рюкзак);
  unlabeled buttons named from their instance name (Предыдущий, Следующий...);
  PageUp/PageDown jump between sections (top-level clips: Персонаж,
  Экипировка, Рюкзак, Характеристики, Категории предметов, Содержимое) in any
  screen; list screens open on their first item.
  Off-hand equip (agent v21, proven in test 21:28; player: Enter on
  a dagger always replaced the main hand, dual wielding impossible): Shift+Enter
  on a backpack item = DAOAccess.EquipAlt, sends "GUI.SelectedChar.EquipItem
  <id> <second slot of GetEquipSlotIndices>" (slot 1 off hand, 10 second ring),
  speaks "Во вторую руку: <name>". Refusals spoken: heavy one-hand weapon
  without the dual-wield master talent, single-slot item, container open,
  equipment locked. Log line "equip alt <path> -> <result>".
- Codex, money and pause v1 (agent v35, built 2026-11-20 ~19:30, deployed the same
  evening, NOT tested; player: "странно и неудобно читать кодекс", "деньги: видим только
  цифру"; tester log 18:32-18:46). Shared core, so Dragon Age II gets it too.
  - Codex groups read "<name>, найдено N из M, развёрнуто / свёрнуто[, есть новое]"
    (kind "group"; before: "1 / 69, флажок, не отмечен"); entries by name (before:
    the index number "250", the name was only the tooltip) with ", новое";
    entries not found yet (Locked) are left out (player's choice 2026-11-20).
  - Codex list rows placed as if not scrolled (agent UnscrolledPos): the list
    stays after the tabs and Close, rows scrolled above the screen stay listed.
  - Long texts (300+ characters with line breaks) are listed one paragraph per
    item ("<path>#2"...); the same text under two field names in one clip is
    listed once (codex description was spoken twice).
  - Money alert: "Получено 1 сер. 76 мед." from <Token>.IntegerArg (before:
    "Получено 1 76"). Items alert: still "Предметы получены"; the mod logs its
    catalogue entry ("items alert <token>:") to find the item names.
  - Pause: from Client.Paused (agent PauseLine), the sign's clip only when that
    cannot be read; ends the "Пауза снята" / "Пауза" flicker.
  - Test: journal, Оглавление кодекса: names of groups, Enter opens a group and
    its found entries follow; Enter on an entry reads title then paragraphs,
    arrows step through paragraphs, Up reaches the tabs and Закрыть. Loot a body
    with coins: "Получено ... мед.". Pause with Space and wait 10 s: one "Пауза".
    Pick up an item; Claude then reads the "items alert" lines in the log.

## Resources

Community, tools, dumps and documentation found for this game, one link each with
what it gives.

Accessibility search, 2026-11-16 (nothing found anywhere):

- Molitvan/blind-accessible-games-list README (GitHub API): no Dragon Age entry.
- GitHub repository search "dragon age" plus accessibility, blind, screen reader,
  nvda, access: no accessibility repository. The gh CLI is not logged in on this
  PC; the public REST API was used instead.
- Nexus Mods GraphQL, domain dragonage, names with access, blind, screen reader,
  speech, narrator, tts, voice: only cosmetic and voice-acting mods. "Color Blind
  Portrait" (890) is a portrait, not accessibility.
- Web search in English and Russian: none. A June 2024 r/dragonage thread asking
  for accessibility mods for the DA games found none. A text-scaling mod
  (Nexus 73) helps low vision only.
- forum.audiogames.net (web search): no Dragon Age thread indexed.
- Not done yet: Spanish and Chinese searches, Accessible Gaming Wiki.

Modding tools and community, checked 2026-11-16:

- JPEXS FFDec 26.5.0 (GPL-3),
  https://github.com/jindrapetrik/jpexs-decompiler/releases/tag/version26.5.0 ,
  unpacked in vendor\ffdec-26.5.0 (zip SHA-256
  8d00c2713a724bb3f7e6c9a64994836ce3a953b5ac264f405d98f92e7c266d33). Decompiles
  the .gfx menus to readable AS2 (java -jar ffdec-cli.jar -export script OUT
  FILE) and compiles our agent (-replace). Needs Java; Temurin 21 is installed.

- Dragon Age Toolset, official BioWare editor, wiki http://www.datoolset.net/wiki/Main_Page
  : campaigns, scripts, GDA tables, GUI. Gives the game's script source and the
  data formats.
- Override folder %USERPROFILE%\Documents\BioWare\Dragon Age\packages\core\override
  (exists): the official way to replace game resources without touching F:.
- Background Autosave Mod, https://github.com/IlIHydraIlI/Dragon-Age-Origins-Autosave
  (found through PCGamingWiki): a dinput8.dll proxy dropped in bin_ship. Shows a
  dinput8 proxy loads in this game. Licence not checked yet.
- llamathings/EclipseBinkProxy, https://github.com/llamathings/EclipseBinkProxy
  (last push 2024-11-14): a binkw32 proxy that logs the files DAOrigins.exe
  opens. A second proxy slot and a file-tracing tool. Licence not checked yet.
- Nexus 6981 Lyrium Memory Fix and 6978 Lyrium Memory Handler (2026-08): patch
  daorigins.exe against memory fragmentation.
- Nexus 6435 DXVK performance fix (2024-12): replaces d3d9.dll, so it would take
  the d3d9 proxy slot.
- PCGamingWiki page Dragon_Age:_Origins (MediaWiki API): engine Eclipse
  (BioWare), GOG DRM-free, GOG LAA enabled since 2024-11-13, Logitech GamePanel
  support (shows the selected character's name, health and mana).
- Gameplay mods asked about by the player (Nexus GraphQL descriptions,
  2026-11-17; none installed): Forced Deathblows (220, creatureranks .gda in
  override; All / Lieutenants / Lieutenants Plus / Minimal), Deathblow
  Multipliers (1613, also overrides combat_h.nss), Deathblow (2110, 44 new
  finishing moves plus weapon balance changes), TSM Random Encounters are
  Less Random (4052), Guaranteed Random Encounters (4406), Lemmings Smarter
  Enemies (4065). No mod found that adds more enemies to ordinary fights.
  Nexus pages and downloads are behind a bot check (403 to automated
  fetches and the built-in browser); the GraphQL API still answers.
  All are .gda/.nss in the override folder, so they do not touch our .gfx.
- No script extender or hook framework exists: Nexus names with extender, hook,
  dll, loader give 0 results; web search finds none.

## Decisions

Each decision with its reason, so it is not reopened by accident.

- 2026-11-19: publishing on GitHub, the installer and the merge with the other
  Dragon Age game into one project: see the root NOTEBOOK.md (Decisions).

- Object offsets differ between Origins and Dragon Age II and are proven again in
  each exe. (The earlier decision to run the two games as parallel projects is
  superseded by the merge of 2026-11-19, root NOTEBOOK.md.)

- 2026-11-16: project folder %USERPROFILE%\Documents\DragonAgeOriginsAccess,
  chosen by the player. Player profile (non-coder, Russian, NVDA) taken from
  the player's other access projects.
- 2026-11-16: the player chose to start recon right away.
- 2026-11-16 recon verdict: Hard. Native 32-bit engine with no community
  framework, so every hook is ours to find. In our favour: official mod support,
  Scaleform UI, 1539 RTTI class names, named GFx exports, no DRM or anti-cheat.
- 2026-11-16 injection route: a proxy DLL named dinput8.dll in bin_ship
  (Ultimate ASI Loader 32-bit, or our own proxy). Reason: DAOrigins.exe imports
  DINPUT8.dll by name, and the autosave mod already loads that way. Fallbacks:
  binkw32.dll or d3d9.dll proxy.
- 2026-11-16: after hearing the verdict, the player chose to go on with the
  inject stage right away.
- 2026-11-16 hook: vtable slot replacement of GFxMovieRoot::Advance (no
  inline-hook library); our code runs on the game thread right after each
  movie's Advance, wrapped in SEH (a crash disables the reader, not the game).
- 2026-11-16 menus, first way: drive the Flash FocusManager left from the console
  version (FocusManager.Navigate*, <focused>.SimulateButtonPush). Proven on the
  main menu, now only the fallback (FocusManager mode in src/menu.cpp).
- 2026-11-17 menus, Flash agent: most screens register nothing in the
  FocusManager. AddItem/AddContainer calls exist only in characterrecord,
  chargen, conversation, deathscreen, inventory, itemexamine, itemupgrade,
  journal, mainmenu, quickbar, saveload, slideshow, startmenu2, store. GFx 2.x
  gives C++ no object walking, so the mod loads its own AS2 helper into every
  movie: _root.createEmptyMovieClip("__daoaccess", 16001), then
  __daoaccess.loadMovie(url), trying in turn "daoaccess.gfx", the mod folder
  path, file:/// of it, the override path, "daoaccess". The helper defines
  _global.DAOAccess.Scan/Press/Adjust and walks the clip tree with for..in.
  Source src/agent/daoaccess.as, compiled by FFDec through tools/build_agent.py
  into build/daoaccess.gfx. The mod keeps its own cursor over the scanned list,
  sorted by screen row then x. Keys: Up/Down, Home/End, Enter, Left/Right on
  sliders, F11 = dump. Active screen = PopupLayer when it shows widgets, else
  the newest movie with widgets. The player approved the FFDec download and the
  override file.
- 2026-11-17: the player chose to move to navigation (scanner, autowalk)
  before polishing more menus: readable menus matter little if the world cannot
  be walked. Menus go on gradually later.
- 2026-11-17 scanner keys: the player asked for the usual access-mod layout
  (camera zoom is useless blind). PageDown/PageUp next/previous, Home repeat,
  End walk, Ctrl+PageDown/PageUp category. The game had PageUp/PageDown as
  secondary zoom keys (ZoomOut_1/ZoomIn_1); with the player's approval they
  were set to (UNASSIGNED) in KeyBindings.ini (backup
  KeyBindings.ini.before-daoaccess next to it). Mouse wheel zoom unchanged.
- 2026-11-17 quickbar list key: Backspace (unbound in KeyBindings.ini; the
  scanner keys only work in the world, so text editing is not affected).
- 2026-11-17 quest hint key: Delete (unbound in KeyBindings.ini, sits in the
  Home/End/PageUp/PageDown block). Insert was avoided: it is the NVDA key.
- 2026-11-18 movement keys cancel autowalk (player request): read from the
  game's own bindings, so a rebinding keeps working.
- 2026-11-18 approval key: Shift+Delete (Delete is the quest key; same pattern
  as Shift+Home / Shift+End for party info).
- 2026-11-18 gifts are given with Enter on the gift in the inventory (the game's
  own GiftItem message), not through a separate key: the game itself ties the
  gift to the selected companion there.
- 2026-11-16: the player said to install each new build of our mod without
  asking every time (deploy.ps1 only; new kinds of game-folder changes are still
  asked).

## Proven facts

Each fact with how it was proven (file and line, log line, in-game test).

- Object manager (2026-11-22, disassembly of games\dao\work\DAOrigins.unpacked.exe with
  capstone, scratch script; also named in the Ghidra project DragonAgeAccess.gpr):
  GetObjectManager 0x4C5380 = mov eax,[0xC2BE24]; ret. LookupObjectById 0xA58190
  calls ObjMgr_FindAndAddRef 0x4D5EB0(id, 1) on it, which finds through 0x8170B0
  -> IdHashMap_LowerBound 0x6FE370 and adds a reference (value vtable slot 2 via
  0x4D5950); 0xA58190 releases it again (0x4D68B0). The manager begins with a
  Visual C++ 2008 stdext::hash_map: list +4 (head node pointer +8, size +0xC),
  buckets +0x14..+0x18 (8 bytes each), mask +0x20, bucket limit +0x24. Node: +0
  next, +4 prev, +8 key (object id, compared unsigned), +0xC value.
  ObjMgr_CollectAllObjects 0x4D5F00 walks this list from [m+8] to the head again
  and pushes every value's vtable slot 0x18 result into a vector: so the list is
  the full set the lookup knows. Reading it in the running game: NOT yet proven
  (first test pending, see Stages, Scanner, Object source).

- Ability targeting (2026-11-22, static: Ghidra headless tools\ghidra_cli.py and
  capstone on games\dao\work\DAOrigins.unpacked.exe, handlers from
  games\dao\tools\handlers.py, table ABI_base.gda of packages\core\data\2da.erf
  through g2da.py; named in DragonAgeAccess.gpr as GME_*). quickbar.gfx
  QuickBarAbilityIcon.OnButtonAction sends GameModeController.PerformAbility
  (ability, caster, source item). GameModeExplore handlers: PerformAbilityMessage
  0x711B50, ...OnTargetUnderCursor 0x7116C0, ...OnSelectedTarget 0x711A00,
  ...OnPlayer 0x711940, CancelAbilityTargeting 0x711AC0 (GameModeGUI has its own
  PerformAbility 0x718110). 0x711B50 reads ability columns by CRC32 name
  (0xA2CCC0(1, id, hash, &out)): TargetType 0x0F642429, UseType 0xACEFC4C2,
  0x1C7CA631 (name unknown: 1 for Lightning, Drain Life, Arcane Bolt, 0 for
  Shock, Flame Blast), AOE_Type 0x2EB7565C, Range 0x81F2A384. It casts at once
  for TargetType 1 (self), or on the caster's current attack target when column
  0x1C7CA631 is 1 (why Lightning "worked" with Enter); else 0x70BA30 enters the
  targeting state: struct 0xC35F60 (vtable), +0x40 ability (0xC35FA0), +0x44
  column 0xE1A8F4EA, +0x48 item, +0x4C TargetType mask, +0x50 caster, +0x54
  mask used with no ability; 0xC35F74 = AOE_Type (2 cone: the ground click is
  clamped to Range). 0x711A00: mask = 0x70ABE0 (ecx = 0xC35F60), valid =
  mode->0x70D550(obj, mask) (thiscall ret 8: bits 1 self, 2/4/0x80 creatures
  by hostility, 0x40 dead, 8 placeable, 0x10 type 12), then
  mode->0x711680(ability, caster, item, target id, &vec3 zero) (thiscall ret 0x14)
  = 0x7F6690 cast + 0x7113F0 leave targeting. 0x7116C0 on the ground passes
  target -1 and the picked point. ABI_base rows: 14000 SHOCK TargetType 126
  Range 2 AOE_Type 2 (35, 10), 10001 FLAME_BLAST the same, 14001 LIGHTNING
  TargetType 4 AOE 0. Calling these from the mod: NOT yet proven in a game
  (Fixes v9).
- Received items (2026-11-22, static: Ghidra MCP and capstone on
  games\dao\work\DAOrigins.unpacked.exe; named in DragonAgeAccess.gpr): class
  Alert (RTTI .?AVAlert@@), constructor Alert_ctor 0x627DD0, alloc 0x180, type
  attribute at alert+0x58 (types seen: 1, 2, 3 items, 4, 5, 6, 7, 8, 9, 10, 0xB,
  0xC). Alerts_AddItemsReceived 0x629620 (type 3, once until shown) is called
  only by CCParty_OnItemAdded 0x6313A0 = slot 7 of CCParty's secondary vtable
  0xB05DBC (thiscall, ret 4, argument = the item object; skips the alert when
  byte 0xC371B4 has bit 2 or 4; sets NewlyAcquired unless bit 0x40). Item
  attributes (CCItem_RegisterAttributes 0x7546F0, name then lea ecx,[edi+off]):
  MaterialType 0x4FC, MaterialProgression 0x50C, Description 0x51C, SizeX 0x550,
  SizeY 0x560, Icon 0x570, EquippableSlots 0x598, Cost 0x5A8, StoreFlag 0x5C0,
  DefaultAmmo 0x5D0, Damaged 0x5E0, StackSize 0x5F0, StackingGroupID 0x600,
  Unique 0x610, NewlyAcquired 0x620, Irremovable 0x630, Indestructible 0x640,
  InfiniteStock 0x650, AbilityID 0x660 (the handler skips its last step for
  AbilityID 200000), Properties 0x680, PropertyPowers 0x69C, RuneSlots 0x4BC,
  RuneSlotMask 0x4D8; an attribute's value is at +0xC (0x53CC40). CCObject
  (0x699670) registers Tag +0x78, Name +0xD4, WorldPositionX +0x88, Y +0x98,
  WorldOrientationY +0xA8, MapNoteVisible +0x25C. Hooked in Fixes v11: NOT yet
  proven in a game.
- Floating texts (floatylayer.gfx, FFDec 2026-11-22: FloatyLayer.as,
  FloatyMessage.as): the engine fills FloatyLayer.BatchUpdateMessageIDs / Strings /
  Styles / Colours / Durations and ProcessBatchMessageFloaties calls
  DisplayFloatyMessage(floatyID, text, style, colour, duration) for each; styles
  (FloatyMessage): 0 default, 1 hit, 2 critical hit, 3 ambient dialog, 4
  description. Floaties are created by id (BatchCreateIDs, symbol 1 = message);
  their relation to object ids: unknown. DisplayStatusMessage(text, colour) feeds
  StatusMessageRoller. DragonAge.ini [GameOptions] SubtitlesLevel=2 on the
  player's PC.
- Inventory lists (inventory.gfx patch003, FFDec 2026-11-22): PlayerItemList
  lists GUIInventory.PartyBackpackView and GUIInventory.PartyPlotItemsView;
  entry N's item token is <list>.N.Item (ItemEntry.TokenName) with ID, Name,
  StackSize, NewlyAcquired (SharedLibrary.Item); viewing it sends
  GUIInventory.ClearNewlyAcquiredFlag <ID>. An items alert (Type 3,
  Party.Alerts.3) holds only WasUber, IntegerArg, ObjectID 4294967295, Type and
  empty quest fields (log 2026-11-22 00:07:22).

- Store (since 2026-11-20): Steam app 47810, appmanifest_47810.acf in
  F:\SteamLibrary\steamapps (installdir "Dragon Age Ultimate Edition", buildid
  21245176). Install folder holds bin_ship, data, modules, packages, Addins, Offers,
  Bonus, docs, redist. Until 2026-11-19: GOG 1949616134 in F:\games\Dragon Age -
  Origins (deleted).
- Steam exe addresses, proven 2026-11-20 without a game run (scratchpad scripts
  vcheck.py / xrefs.py over the Steam file and its Steamless copy):
  - .rdata (plain in the Steam file): GFxMovieRoot vtable 0xB4CA6C slots 12 / 18 /
    30 = 0x954AD0 / 0x956E80 / 0x959590 (GetVariable / Invoke / Advance), as in
    gfx.cpp.
  - Decrypted code: 0x70BE80 calls 0x5D58C0, 0x764020, 0x7F6370 and reads
    0xC327EC; 0x70C9A0 reads 0xC35E77 and calls 0x70BE80; 0x713350 calls 0x70CAD0
    and 0x712830; 0x5D5BE0 calls 0xA58190; 0x70CAD0 calls 0x838210 and 0x6AB100;
    0x768AE0 calls 0x7675B0, 0x766040, 0x7661B0, 0x7658E0; byte 0xB59409 is used
    in the lookup code; every function the mod calls starts right after int3 / ret
    padding. 17 of 17 checks matched move.cpp / objects.cpp.
  - Vtable lengths (decrypted copy): GameModeExplore 0xB16374 has 32 function
    slots, then the UTF-16 string "Target Required"; GameModeCombat 0xB1D89C 32,
    then floats; GameModePartyPicker 0xB1D31C 30, then "LockedPartyMembers". The
    mod hooked 39 / 32 / 39 until 2026-11-20 (fixed to 32 / 32 / 30).
- HKLM\SOFTWARE\BioWare\Dragon Age, value Path (32-bit view) points to the Steam
  install: the installer's /find found it there 2026-11-20 (was a HYPOTHESIS).
  The same key also holds Flavour=steam, language=en.
- Language of the Steam copy (2026-11-20): English only. Steam store API
  (store.steampowered.com/api/appdetails?appids=47810): English (full audio),
  Italian, Spanish, German; no Russian. appmanifest_47810.acf language "english";
  packages\core\data\talktables holds only core_en-us.tlk; only some DLC folders
  (Addins\dao_prc_cp_1 / _2) carry ru-ru talk tables and voice.
  Documents\BioWare\Dragon Age\Settings\DragonAge.ini [GameOptions] has
  Locale=en-us (text) and VOLanguage=en-us (voice), so text and voice are set
  apart. The GOG copy played in Russian. The Russian text is already on the PC:
  Documents\BioWare\Dragon Age\packages\core\override\enpy (71 MB, files dated
  2018-12) holds the ENPY Studio / IMK fan text translation ("Русификатор для
  Dragon Age: Origins (+ Awakening, + DLC) от 29.01.2019", playground.ru, text only,
  English voice; its page says for Steam: game language English, then install):
  packages\core\data\talktables\core_ru-ru.tlk (+ _f), packages\core_ep1 (Awakening),
  modules\single player talk tables, AddIns\<every DLC> and Offers\<promo> ru-ru
  talk tables, ru-ru world map textures and intro videos. Since the override folder
  is loaded whatever the install, Locale=ru-ru alone was tried 2026-11-20: the game
  wrote Locale=en-us back at its 14:01 start. Why, from the decrypted exe's strings:
  it checks for packages\core\data\talktables\core_<Locale>.tlk in the install
  folder ("Could not find a valid file for the Locale language (Talk Tables)
  setting %s (as specified in DragonAge.ini) in path %s", fallback core_en-us.tlk),
  and it reads HKLM\SOFTWARE\BioWare\Dragon Age\Language with a table ru, pl, hu,
  cz, es, it, de, fr, en -> ru-ru ... en-us (Steam's 47810_install.vdf writes "en";
  its languages are en, es, it, de only). Done 2026-11-20 (player approved): the six
  ENPY talk tables copied into the install folder (Game folder changes) and
  Locale=ru-ru set again. Proven 2026-11-20 14:54: that is enough, the registry
  (still language=en) is not needed; menus and options are Russian, the copyright
  line ends "© ENPY Studio", Locale=ru-ru stays after the game exits. Other patches found
  2026-11-20: the 2009 "звук + текст, официальная локализация" archive (no version
  stated) and a 2025 fix for the EA app edition (says not for Steam).
  Official source (read 2026-11-20; the site's TLS certificate has expired, read
  with curl -k): https://enpy.net/projects/localizations/games/dragon_age_origins/
  ("Статус: Закончен", a support topic and a download page on the site's forum,
  /forum/index.php?app=downloads&module=downloads&controller=view&id=74, which shows
  nothing without a login). Terms, https://enpy.net/license/: free, no commercial
  use; no warranty; no hosting anywhere and no changes to it or any part without
  the consent of the ENPY.NET editors; only for licensed copies of games. Age does
  not lift copyright, so the project may not ship these files or a modified copy
  without ENPY's written permission; copying the player's own installed files on
  the player's PC, or a link to ENPY's page, is fine.
- DLC in the Steam copy (game log Documents\BioWare\Dragon Age\Logs\DragonAge_1.log,
  session 13:12): all 20 add-ins enabled (DAO_PRC_*, PC_GAMER_TALE,
  DAO_PRC_PROMO_ME, DAO_PRC_NRX_1) without any EA sign-in; online features
  "Connection failed", harmless.
- Memory without LAA (Steam, log 13:13-13:22 in camp): used 1965 of 2047 MB
  address space, largest free block 33 MB, i.e. at the edge where the GOG copy
  crashed before LAA (2026-11-17). Hence the LAA exe (Game folder changes): from
  13:43 the address space is 4095 MB with 2046 MB free in one block.
- 32-bit, native, no CLR: pefile on bin_ship\DAOrigins.exe, Machine 0x14c, COM
  descriptor 0. Sections .text .rdata .data .tls .rsrc .reloc (no SteamStub).
- Engine Eclipse: 38 "Eclipse" strings in DAOrigins.exe plus the PCGamingWiki
  engine row. Version 1.05.13263.0 from the exe version resource.
- UI is Scaleform GFx: packages\core\data\guiexport.erf holds 67 .gfx files
  (abilities.gfx starts with "CFX", compressed GFx version 8). The exe exports
  153 Scaleform functions (GBufferedFile, GFxLoader, GImage and more) and holds
  271 "GFx" strings and 1539 MSVC RTTI class names.
- Imports by name (proxy candidates): DINPUT8.dll, d3d9.dll, WINMM.dll,
  VERSION.dll, binkw32.dll (a game-folder DLL). Read with pefile.
- Game archives: .erf (ERF V2.0, UTF-16 names), .rim, .tlk talk tables (all game
  strings), .gda tables. Russian strings:
  packages\core\data\talktables\core_ru-ru.tlk and core_ru-ru_f.tlk.
- The player's game runs in Russian with Russian voice: Documents\BioWare\Dragon
  Age\Settings\DragonAge.ini has Locale=ru-ru and VOLanguage=ru-ru.
- The game writes its own log: Documents\BioWare\Dragon Age\Logs\DragonAge_1.log
  (UTF-16, init and shutdown steps; last written 2026-11-16 21:53, so the game
  starts on this PC).

- Loader: Ultimate ASI Loader v9.7.4 (MIT), x86 build, copied from the player's
  AgeOfTheRingAccess project (vendor\ual-9.7.4, SHA-256
  D5A059AA467A7A7127C8F6169F79FA63FF0F55986EE9EB2FD9A281BEBF2AA2E6, proven there
  2026-10-31). Release: https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.4
- Proxy checks (2026-11-16): DAOrigins.exe imports DINPUT8.dll by name (pefile);
  dinput8.dll is not in KnownDLLs (registry read); bin_ship had no dinput8.dll and
  no scripts folder before our deploy (Test-Path).
- Probe: src\main.cpp, built by build.ps1 (vcvars32, cl /LD /MT) into
  build\DAOAccess.asi, PE Machine 0x14c, exports InitializeASI. DllMain only
  starts a worker thread; the worker logs exe path, version and PE stamp,
  command line, which dinput8.dll is loaded, then speaks through Prism and waits
  up to 2 minutes for the first visible window.
- Speech is bundled in the same test: Prism 0.18.2 hand-built 32-bit
  (lib\prism-x86\prism.dll, copied from AgeOfTheRingAccess, SHA-256
  DA99650E809F4767CD14173A6BBF3CF31E3AEDAB09C9781D62B66EF6D74590DE; NVDA backend on,
  JAWS and SAPI off). src\speech.cpp loads it by full path from
  bin_ship\scripts\DAOAccess\.
- Mod log: F:\games\Dragon Age - Origins\bin_ship\scripts\DAOAccess.log.
  Deploy: deploy.ps1 (builds, refuses while DAOrigins runs, copies).
- Proof of injection and speech (launch 2026-11-16 22:19, build Nov 16 2026
  22:16:27): "DAOAccess probe loaded | bits=32 | exe=...\bin_ship\DAOrigins.exe
  (1.5.13263.0, ts=0x4eb19d53 size=0x9ec0a2)", loader
  "dinput8.dll=...\bin_ship\DINPUT8.dll (9.7.4.0)", "speech: backend NVDA",
  both speak lines "-> Success", first visible window "DragonAge" 0.1 s after
  load, system d3d9.dll. The command line had no arguments.
- UI architecture (RTTI names and strings in DAOrigins.exe, 2026-11-16):
  - Scaleform GFx with ActionScript 2 classes: GFxMovieRoot, GFxMovieView,
    GFxSprite, GFxButtonCharacter, GFxEditTextCharacter, GASExternalInterface.
  - Flash talks to the game through EclipseGFxExternalInterface and
    EclipseGFxFSCommandHandler; the game answers with typed messages handled by
    named C++ classes, for example ConversationData / SelectResponseMessage,
    GUICharGen / CharGenSelectRaceMessage, CClientExoApp / GUIDialogResultMessage,
    DragonAgeGUISystem / PushGUIScreenMessage and PopGUIScreenMessage,
    CClientOptions / SetClientOptionMessage.
  - Screen data is bound through a reflective layer "SmartData" (Attribute,
    ObserverNotification) holding typed lists: Response, ConversationLine,
    SaveFileInfo, ModuleInfo, Inventory, JournalEntry, CodexEntry,
    PartyPickerEntry, AbilityGUIInfo, MapLocation and more.
  - Mouse input reaches Flash as GUI messages to GUI::ScaleformManager
    (GUICursorLeftButtonDown, Up, DblClick...). The exe imports DINPUT8, so the
    cursor is likely read through DirectInput. HYPOTHESIS: this is why NVDA's
    mouse routing (SetCursorPos) does not move the game cursor.

- Scaleform version is GFx 2.x, not 3.x: GBufferedFile/GThread exports, no
  Direct Access API strings (GetDisplayInfo, VisitMembers absent), InvokeParsed and
  GetVariableRaw error strings present. So C++ cannot walk Flash objects; it can
  read any ActionScript path (GetVariable) and call any function by path (Invoke).
- GFxMovieRoot vtable 0xB4CA6C (RTTI COL scan, 52 slots). Slots proven by which
  error strings each reaches (capstone call graph) and by ret sizes:
  - 11 SetVariable 0x95A3B0 (ret 0xC)
  - 12 GetVariable(GFxValue*, path) 0x954AD0 (ret 8, reaches "GetVariable failed")
  - 17 Invoke(name, result, fmt, ...) cdecl 0x9570E0
  - 18 Invoke(name, result, const GFxValue* args, UInt n) 0x956E80 (ret 0x10)
  - 19 InvokeArgs(va_list) 0x9571D0 (reaches "InvokeParsed", "Can't find method")
  - 30 Advance(Float, UInt) -> Float 0x959590 (ret 8, returns 1/framerate)
  - 31 Display 0x9525C0
- GFxValue layout (converter 0x954910): int type at +0, value at +8, 16 bytes.
  Types 0 undefined, 1 null, 2 bool (byte), 3 number (double), 4 UTF-8 char*,
  5 wchar_t*. Bit 0x10 on the input type asks for conversion. Objects come back as
  type 4 through ToString: a movie clip gives its target path, for example
  "_level0.StartButtons_mc.StartMenuButtons_mc.NewGameButton" (log, test 23:37). Strings live in a movie pool: copy at once.
- The game uses path names for Invoke itself: "_root.PlayScene",
  "_root.Filename", "TokenManager.PerformBatchTokenReplacements" (exe strings), and
  "InvokeCallbackMessage conversation FocusManager.NavigateUp" (Flash).
- Flash menus (extracted from packages\core\data\guiexport.erf, CFX = zlib SWF 8,
  disassembled with tools/erfx.py + tools/as2dis.py; tools/methods.py lists
  class methods; tools/rtti.py finds vtables):
  - Every movie embeds the same BioWare AS2 library: GUISystem, FocusManager,
    ContextSystem, ExternalCommands, Localization, SharedLibrary.* buttons.
  - FocusManager (console navigation) is still alive on PC: NavigateUp/Down/
    Left/Right, OnAction, SetCurrentFocus, m_CurrentFocus, sm_CurrentRootContainer.
  - startmenu2.gfx: every main menu button init calls FocusManager.AddItem(this)
    on every platform; only SetCurrentFocus is skipped when GUISystem.Platform is
    "PC". Labels come from Localization.Strings.LOC_* through ButtonBase.SetText,
    which writes the button's TextField.
  - Behaviour.PushButton.SimulateButtonPush() presses a button (onPress +
    onRelease) without the mouse.
  - GUISystem.InConsoleMode() = Platform != "PC" or
    ExternalCommands.GetValue("DebugOptions.EnableController") == 1. DebugOptions
    is a C++ options class (RTTI) and "DebugOptions" sits among the UTF-16 ini
    names in the exe. HYPOTHESIS: a [DebugOptions] EnableController=1 section in
    DragonAge.ini turns on the console menus; not tried, kept as a fallback.

- In game (log, test 2026-11-16 23:37): the Advance hook fires on the game thread
  for every movie; movies seen at the title screen are Tooltips, PopupLayer,
  startmenu2 (each has _root.Filename), then Credits when pushed. Only startmenu2
  has a FocusManager root. FocusManager.NavigateDown/Up/Left/Right via Invoke
  move the focus (one log line per press, path changes each time) and
  <focus>.SimulateButtonPush pressed Credits (the Credits movie opened).
- Main menu (test 23:45, no saves on this PC): screen y NewGame 54, Module 122
  ("Другие кампании"), DLC 156, Screenshots 190 ("Профиль игрока"), Options 224,
  Credits 258, Quit 292. FocusManager Down goes top to bottom and wraps; with no
  focus yet, the first press lands on Quit.
- Button label: the readable text is <button>.RollOver_mc.ButtonText_MC
  .TextField.text (test 23:45, Russian). <button>.ButtonText_MC.TextField.text
  and <button>.TextField.text give nothing.
- Screens (test 23:45): Options opens a separate movie "OptionsMenu"
  (optionsmenu.gfx); New Game opens a movie "NewGame" while the start menu stays
  visible; neither has a FocusManager root. "Другие кампании" switches
  startmenu2 to its module list (6 DLC campaigns plus Назад, Играть).
- Flash agent works (log, test 2026-11-17 00:05): _root.__daoaccess.loadMovie
  ("daoaccess.gfx") loads our file from the override folder by bare name, ready
  1000 ms later (first check), in PopupLayer, Tooltips, startmenu2, OptionsMenu
  and NewGame. Scan returned 10 items on the main menu (Load Game reported
  disabled: no saves), 21 on the Options Game tab (5 tabs, pickers with values,
  toggles with state). Press on a tab selected it; Press on a picker opened its
  PickerEntry list.
- The game loads its menus from the patch archives:
  packages\core\patch\patch003.erf holds 57 .gfx (startmenu2, optionsmenu,
  newgame...), patch002.erf 68. Study those copies, not guiexport.erf.
- newgame.gfx (patch003, FFDec): campaign choice. GameButton0_mc /
  GameButton1_mc have no text; their names sit in sibling clips GameLabel0_mc /
  GameLabel1_mc (from ExternalInterface GetMainModuleName). CloseButton_mc has
  only TooltipSource = LOC_CLOSE.
- chargen.gfx (patch003, FFDec, 2026-11-17): scenes RaceGender, Class
  Specialization, AppearanceSoundSet, Attributes, Skills, SpellsTalents, Review,
  Import. On "Начало" it first shows Popups.ShowDialogBox(LOC_BACKGROUNDS_MESSAGE,
  LOC_CLOSE). RaceGender: groups gender_mc, race_mc, class_mc, background_mc,
  each with HeaderText_mc and icons.iconN (CharGenButton, a StickyButton with
  no text; name in oTarget.sName and TooltipSource; disabled via
  m_bStickyButtonEnabled = false). The chosen item's description goes to
  race_description_mc.description_header_mc and
  scrolling_description_mc.Scrollable_mc (SetDescriptionText). Next/Previous
  are race_nav_mc.next_mc / previous_mc. The game ends chargen with a
  difficulty picker popup (Popups.ShowDialogBoxWithPicker).
- Chargen in game (log, test 2026-11-17 00:39): "Dragon Age: Начало" goes
  through movies load_town and conversation (about 3 minutes) to CharGen. The
  backgrounds message comes as a PopupLayer dialog (message text, then
  _level0.null.Buttons_mc.Button1_mc "Закрыть"; the dialog clip has no name, so
  its path holds "null"). CharGen RaceGender listed 25 items: header texts
  "Создание персонажа", "Пол", "Раса", "Класс", "Биография", the chosen
  gender text, 2 gender + 3 race + 3 class + 6 background icons (class and
  background disabled until a race is chosen), description header and text,
  "Назад", "Далее" (disabled). Icons had no label: CharGenButton.oTarget has no
  sName on this build; the name is oTarget.nNameStrRef (talk table), and
  ExternalCommands.GetTlkString(id, "") resolves it inside the movie.
- Chargen later scenes (log, test 2026-11-17 01:07; chargen.gfx FFDec):
  - Appearance: SliderGroupNames_mc.SliderGroup0..9 (Набор установок, Кожа,
    Волосы, Глаза, Нос, Рот, Челюсть и щеки, Шея и уши, Портрет, Голос) and
    SoundSetList_mc.SoundSetButtonN (voice sets: Мудрый, Дерзкий...), both sticky
    choices; name field Portrait_mc.name_name_mc.TextField is an input field
    (default "Айдан", maxChars, restrict "^ /t", onSetFocus calls
    FilterKeyboardInput), last name text "Кусланд"; play_mc "Быстрая загрузка".
  - Attributes: AttributeList_mc.AttributeAdjusterN with Label_mc (Сила...),
    PointsLabel_mc (value), Minus_mc / Plus_mc (ActionOnPress buttons whose
    onRollOverHitArea calls AttributesScene.SetDescriptionText); points left in
    attribute_points_mc.top_frame_mc.pointsleft_mc. The description panel says
    "hover with the mouse", so descriptions need the game's hover.
- Text input works (log, test 2026-11-17 01:25): DAOAccess.Focus
  (Selection.setFocus + setSelection) on the chargen name field returned "ok",
  the game passed typed Russian letters into the field ("г", "га" ... "галар"),
  and the field kept the name after Unfocus. Typing replaced the selected
  default name.
- In game (log, test 2026-11-17 01:35):
  - Skills and talents icons: content_skills_mc.ScrollingSkills_mc
    .Scrollable_mc.AbilityChainN.iconK and content_spellstalents_mc...
    AbilityChainContainer_mc.AbilityChainN.iconK. AbilityChain.AddAbility puts
    the SharedLibrary.Ability on the icon as m_oAbility (sNameStrRef is the
    resolved name after LoadDeferredAbilityInfo, nID, nDisplayState 1 have /
    2 available / 3 unlockable). DAExternalCommands.HasAbility(creature, id)
    and HasAbilityPreRequisites give the live state. Pressing an icon spent a
    point.
  - The end of chargen is a PopupLayer dialog "Пожалуйста, выберите уровень
    сложности." with _level0.null.Picker_mc and Buttons_mc.Button1_mc.
  - Conversation: movie "conversation", responses
    _level0.ButtonContainer_mc.Buttons_mc.ResponseN_mc, labels "1. ..." read and
    pressed by the agent; the movie disappears between lines ("screen none").
  - In the world the HUD movies appear: ArmyControl, Minimap, Portraits,
    NavBar, Quickbar, SaveScreen, FloatyLayer (floating names, "Пауза" in
    PauseIndicator_mc), Notifications (NotificationActivateN buttons such as
    "Предметы получены", "Задание обновлено"). Tutorials come as PopupLayer
    texts (TrainingTutorial_mc, TutorialContainer_mc with "Далее"/"Закрыть").
    Esc opens the movie "MainMenu" (mainmenu.gfx), first item "Возобновить
    игру".
- Catalogue dump (F10, test 2026-11-17 02:19, Castle Cousland, 886 nodes in
  about 220 ms from the Minimap movie):
  - roots include Party, SelectedChar, Area, MapNoteList, Journal,
    Conversation (HasVO, Responses, NPCLine...), GameMode / GameModeExplore,
    Objects (value 0, no members: maybe an id lookup, untested), GUI, SaveLoad.
  - Party.Leader / Party.Hero / SelectedChar = object 130 ("Айдан", Tag
    "player") with WorldPositionX, WorldPositionY, WorldOrientationY, Name, ID,
    IsHostile, InCombat... Party.Members has 4 slots.
  - MapNoteList.MapNotes: 10 entries with Tag, Name, MapNote, Icon, ID,
    WorldPositionX/Y, WorldOrientationY, MapNoteVisible.
  - Journal.PlotDestinations: 1 entry (Object, ActiveQuest, Tooltip);
    Journal.PlotGivers: 0 here. Area.Name "Замок Кусланд".
  - Unit: the leader moved 17.5 units during about 3 s of W (running), so the
    unit is about a metre (HYPOTHESIS, to confirm with an object next to the
    player).
  - Direction: camera minus pivot points behind the leader; with orientation o
    (degrees, GameMode.CameraOrientationY = SelectedChar.WorldOrientationY
    after running) forward = (-sin o, cos o): proven on two dumps (o = -29.85
    gives forward (0.50, 0.87); o = -164.69 gives (0.26, -0.96), both matching
    the camera offset).
  - The minimap batch holds only the camera (id 0) and the player (130) here;
    its positions are minimap pixels (367, 224), not world units.
- Scanner and autowalk candidates (2026-11-17, static study):
  - The game's data catalogue ("SmartData"), read from any movie through
    ExternalCommands: GetValue(path) (ExternalInterface "GetValue"),
    GetNumMembers(path) and ListMemberNames(path) (fscommands answered in
    _root.fsc_retval). The map code reads Journal.PlotGivers.N.{Name,
    WorldPositionX, WorldPositionY, MapNoteVisible}, Journal.PlotDestinations.N
    .Object.{Name, WorldPositionX/Y}, .ActiveQuest, MapNoteList.MapNotes.N
    .{MapNote, Icon, WorldPositionX/Y}, Area.{Name, ResRef, North, Minimap*},
    GameModeExplore.CameraPivotPositionX/Y, GUIMain.MinimapDistance (map.gfx,
    minimap.gfx patch003, SharedMap.as). HYPOTHESIS: the catalogue holds more
    world data; the F10 dump lists it.
  - Minimap creature batch: the game writes arrays into
    SharedLibrary.SharedMap.BatchCreatureIDs, BatchCreaturePositionsOrientations
    (x, y, orientation per creature) and BatchCreatureFlags (bits IS_HOSTILE,
    IS_IN_PARTY, SHOW_AS_ALLY, IS_PLAYER, IS_WITHIN_MAP_RADIUS; CreatureMapIcon.as)
    of the Minimap movie (exe strings at file 0x7036d4, 0x703710).
  - Client object classes (RTTI): CCObject, CCCreature, CCPlaceable,
    CCTrigger, CCTriggerAreaTransition, CCTriggerTrap, CCWayPoint, CCItem,
    CCArea, CCModule, CCParty, CCPlayer; CClientVirtualMachineCommands and
    script command names COMMAND_COMMANDMOVETOOBJECT, ...MOVETOLOCATION.
  - Movement (disassembly, 2026-11-17): the two messages carry no data (their
    factories allocate 4 bytes, just the vtable). GameModeExplore's
    PreMoveCharacterToLocationMessage handler (0x70AAF0) only sets byte
    0xC35E77 = 1 (button down). The MoveCharacterToLocationMessage handler
    (0x70C9A0, registered at 0x70DEC2) plays "Click" and stops when that byte is
    0 (this is what the three SendMessage forms hit in test 02:31: no
    movement); else it takes the selected creature (0x694990), and when the
    picked point is valid ([this+0xD4], pointer [this+0xD0] to a 16-byte
    x,y,z,w vector) calls 0x70BE80(this, creatureId, &point, -1). That one
    stores the point at [this+0xA0], gets the object (0x5D58C0, thiscall on
    *(0xC327EC), ret 4), skips it when vtable slot 0x1D0 returns non-zero, then
    calls 0x764020 (thiscall on [obj+0x764], args &point and -1, ret 8,
    returns a byte) and 0x7F6370(obj, -1) (cdecl). The creature id is
    [obj+0x10] (0x53CC40 on obj+4 returns [+0xC]).
  - Pathfinding namespace (MapSearchEnvironment, MapAbstractionSearch...)
    exists in the exe.
- Pitfall (2026-11-17): a scripted find-and-replace inserted the spin code into
  the first matching function end (A.Press) instead of A.Adjust. After any
  scripted edit of the agent, check the decompiled result function by function.
- Engine objects (disassembly, 2026-11-17; runtime proof pending in the F10
  dump):
  - 0xA58190(id, &out), __stdcall ret 8, looks an object up by id through the
    manager at *(0xC2BE24) (0x4C5380 returns it); al == 0 means found (byte
    0xB59409 is 0). 0x5D5BE0 (thiscall on the client app *(0xC327EC), ret 4)
    wraps it and returns out->vtable[6]() but returns garbage (1) when the id is
    free, so call 0xA58190 directly.
  - Class layout: CCObject derives from CGameObject at offset 0 (RTTI; client
    classes CCCreature, CCPlaceable, CCItem, CCTrigger,
    CCTriggerAreaTransition, CCWayPoint all start with a CCObject vtable).
    vtable slot 4 = AsCreature (this or 0: 0x79AF30 / 0x4137B0), slot 6 =
    AsClientObject, slot 9 = AsPlaceable. Slot 4 of the secondary vtable at
    +0x18 is the deleting destructor: never call it.
  - byte +0x14 = object type (cursor code 0x70CAD0: 6 creature, 7 item, 10
    placeable; placeable byte +0x42C = usable). +0x120 = x, y, z.
  - The CCObject constructor (0x699670) registers its catalogue attributes
    inside the object: Tag at +0x78 (its value pointer at +0x84 points to
    +0xB8), Name at +0xD4, WorldPositionX +0x88, WorldPositionY +0x98,
    WorldOrientationY +0xA8, MapNoteVisible +0x25C, ID before them. Strings are
    Visual C++ 2008 std::wstring (capacity at +0x18, inline under 8).
  - GameModeExplore handles, among others, SelectNextUsableObjectMessage
    (0x70F500), SelectNextCreatureTargetMessage (0x70EE10),
    PerformDefaultActionOnTargetMessage (0x713350), PerformUseObjectMessage
    (0x70BC60), CenterOnObjectMessage (0x70C140), TeleportPlayerMessage
    (0x70EF10) (tools/handlers.py). 0x713350 takes the target id from global
    0xC06794, looks the object up, gets the cursor action 0x70CAD0(obj) and
    calls this->0x712830(obj, action, 0) (thiscall, ret 0xC).
  - GameModeExplore vtable 0xB16374 (32 slots; 39 before 2026-11-20, see Proven facts, Steam), GameModeCombat (derived)
    0xB1D89C (32 slots); modes live in GameModeController's array. The mod
    captures the instances with per-slot thunks (mov [g], ecx; jmp [orig]).
- Journal quests (journal.gfx patch003, FFDec QuestList.as / QuestEntry.as,
  2026-11-17): the journal lists Journal.PlotList.N with ResRef, ParentPlot,
  Name, Description (current text, a heading in <emp>..</emp>), Completed,
  Updated (cleared by the journal when viewed); Journal.ActivePlot.ResRef is
  the tracked quest. Journal.PlotDestinations.N.Tooltip is the quest's name
  (log, test 17:07: "Поручение отца", "Отыскать снаряжение").
- Party catalogue (F10 dump 02:19): Party.SelectedCount, Party.MemberSelected.N
  (0..3), Party.Members.N (same fields as SelectedChar), Party.HoldPositions,
  Party.Leader. KeyBindings.ini: F1..F4 SelectPartyMemberN, Shift+F1..F4
  ...Inclusive, F5 QuickSave, Backslash ToggleTactics.
- Quickbar (quickbar.gfx FFDec, 2026-11-17): slot N of the selected character
  is the ability instance GUI.SelectedChar.Stats.QuickSlots.N (value 0 =
  empty) with CooldownPercentage (0 ready, up to 1; PieCooldown.as), Status
  ("1" = active highlight, sustained on), PendingActivation, IsError (icon
  disabled), IsWarning (prerequisite missing), AbilityTemplate.{ID, Name,
  NameStrRef, AbilityType, Cooldown, Cost}, SourceItemName,
  SourceItem.StackSize. A click sends "GameModeController.PerformAbility
  <abilityId> <playerId> <sourceItemId>".
- Level-up (levelup.gfx FFDec): LevelUp.CreatureID = CharGenCreature.ID; the
  movie also defines CharGen with CreatureID 0. Learning sends
  "GUILevelUp.CharGenAssignAbility <creature> <id> 1".
- Health and game mode (portraits.gfx, inventory.gfx, GUISystem.as, FFDec
  2026-11-17): a creature's health is <token>.Attributes.7.Current / .Total,
  mana or stamina Attributes.8, level 15, experience 19, attack 9, defence 10,
  armour 11; GameModeController.CurrentGameMode is 0 explore, 1 combat.
- Inventory screen (inventory.gfx patch003, FFDec): SharedLibrary.Item clips
  carry TokenName, ObjectID, Name, Description, StackSize, EquippedBy,
  EquippableSlots, AbilityID and DoubleClickAction (container: Container
  .AcquireItem; backpack: GUI.SelectedChar.EquipItem id 255, or
  GameModeController.PerformAbility for usable items). EquipSlot clips:
  m_nSlotID, m_sSlotToken "GUI.SelectedChar.Equipment.<slot>", m_nItemID,
  GetContainedItemID(); unequip = SendMessage "GUI.SelectedChar.UnequipItem
  <id>". Slot ids: 0 main hand, 1 off hand, 2 ammo, 4 armour, 5 helm, 6 boots,
  7 gloves, 9 and 10 rings, 11 amulet, 12 belt, 14-17 Shale, 18-19 dog.
- GDA tables are GFF V4.0 "G2DA V0.2": column names stored as CRC32 of the
  lowercase name in UTF-16LE (matched "id", "label", "fdeathblow", "color" in
  creatureranks.gda, 2026-11-17); rows list at data+0x5C, 40-byte rows.
  creatureranks fDeathblow is column 4 (float, row offset +16). tools\g2da.py
  reads any table (python tools/g2da.py <erf> <name.gda>). Random encounter
  tables rand_*.gda: ID, Label, TriggerChance (percent), Repeat, a plot
  name, plot flag, Area; values match the Nexus 4052 description
  (AXE_IN_STUMP 1, WILD_SYLVANS 20, WEREWOLF_1 30, SP_SUPERMETAL 2).
  Whether a trip rolls an encounter at all is decided elsewhere (Nexus 4406). HYPOTHESIS
  (Nexus 1613 description): combat_h halves the chance when more than two
  enemies surround the attacker, so 1.0 may still miss some deathblows.
- Quest markers: the game's option "Сюжетные подсказки" (GameOptions.PlotAssist,
  optionsmenu.gfx: 0 Выкл, 1 Только в исследованных областях, 2 Всегда;
  DragonAge.ini PlotAssist=1 on this PC, 2026-11-18). Its help text: markers on
  the current area map showing where to go next for the current quest. They
  are Journal.PlotDestinations (the scanner's "Задания"); the world map marks
  the tracked quest's area pin (WorldMap.m_nActiveQuestIndex, only with
  PlotAssist 1). Only steps the designers marked have one: 18 different markers
  in the logs (castle, Ostagar, tower, wilds), none in the camp.
- Scrolling lists (inventory.gfx SharedLibrary.OptimizedList, FFDec
  2026-11-18): only the rows that fit the viewport exist (pool of
  viewport/pitch + 2 ItemEntryN clips, reused); entry.List = the list,
  entry.ListItemIndex = place in the whole list, List.m_nNumVisibleEntries =
  the full count, pitch = ItemHeight (38) + 2, scroll = List.YPosition
  (DataType.ConstrainedInteger; SetWithConstraints notifies OnScroll ->
  SynchronizeList, which re-fills rows), List.m_aVisibleEntries = rows on
  screen. Agent v24: item lines carry "index/total"; DAOAccess.ListGo(path,
  index) scrolls and returns the row's path. Up/Down/Home/End on such rows go
  through the whole list ("15 из 40"); "Начало/Конец списка" at the ends.
  Proven in test 2026-11-18 15:10 ("Кожаные перчатки, 1 из 53"; player: the
  inventory shows many more items). OptimizedList is used only by inventory,
  saveload and store (grep of all 67 decompiled menus, 2026-11-18); every other
  list (ListGenerator, Scrollable) creates one clip per row, so nothing is cut.
- World map (worldmap.gfx WorldMap.as, SharedLibrary/MapPinIcon.as, FFDec
  2026-11-18): pins carry m_sPinToken = "WorldMap.PrimaryMap.Locations.N"
  with Name, Tooltip, State (1 not active = hidden, 2 active, 3 grayed out,
  4 unlockable, 5 completed, 6 destroyed), AreaTag, Type, X, Y; travel =
  "GameModeController.AreaTransition <player area tag> <index>" only when
  State 2 and not WorldMap.m_bReadOnly (WorldMap.GUIStatus 1 = read only);
  WorldMap.m_nCurrentLocIndex = player's pin, m_nActiveQuestIndex = tracked
  quest's pin. Agent v24 reads pins as "<name>, можно отправиться / закрыто /
  пока не открыто / пройдено / разрушено ..., вы здесь, цель задания"; Space
  gives the description. Proven in test 2026-11-18 15:12 (opened from the
  area map in the camp: "Стоянка отряда, сейчас отсюда не отправиться, вы
  здесь", "Денерим, закрыто"); travel not tried yet.
- Companion approval (inventory.gfx / characterrecord.gfx SharedLibrary
  .ApprovalBar, CreatureUtils.GetApprovalInfo, FFDec 2026-11-18): the catalogue
  list Party.Approval.N.{ObjectID, Approval (-100..100), Desc (word such as
  "Тёплое")}; the bar hides itself when the selected character has no entry
  (the hero). Approval changes come as Notifications alerts (ALERT_APPROVAL,
  LOC_APPROVAL_INCREASED): "Морриган одобряет (+4)" was already spoken by the
  passive Notifications reader (log 15:16:41).
- Gifts (inventory.gfx Item.as, ItemEntry.as, RadialMenu.as, PaperDoll.as):
  gift items have BaseItemID 109; their tooltip says LOC_GIFTABLE. Double click
  does nothing for them; the radial "gift" button and a drop on the paperdoll
  send "GameModeController.GiftItem <GUI.SelectedChar.ID> <item ObjectID>", only
  when the selected character has an approval entry. So a gift is given in the
  inventory with the companion selected (Персонаж, Следующий).
- Feastday gifts (addins dao_prc_cp_3 "Feastday Gifts", dao_prc_cp_7 "Feastday
  Pranks", designeritems.erf template names; mod log 2026-11-18 20:55-21:28): the
  DLC gifts are val_im_gift_X (base 109); after GiftItem the item stays in the
  backpack under the same name, moved to the top, without base 109, and the
  backpack count does not drop (puppet, Alistair doll, lute, stick). The DLC has
  matching keepsakes val_im_qck_X (amulet, ball, brandy, doll, flask, horse, lute,
  prayer, rock, soap) and wearables (val_im_arm_shd_twr_mrc, val_im_arm_bot_lgt_ugl,
  val_im_wep_mag_sta_cat, val_im_spc_dog_col_shame, val_arm_shl_pigeon). So this is
  the game's own swap, not a mod fault. Approval +50 per Feastday gift. Item clips
  have no tag field filled in ItemEntry; the agent reads <TokenName>.Tag
  (Item.as reads the same key). HYPOTHESIS: qck items are usable (companion plays
  the lute: val_leliana_music audio in cp_3).
- Specializations (levelup.gfx LevelUp.as, ClassButton.as): attributes scene,
  specializations_mc.Scrollable_mc.iconN (ClassButton: sName, sDescription,
  bSelected, bGrey, sAchievement, nSpecializationAbilityID); click =
  onPressHitArea (spends LevelUp.nSpecializationPoints =
  CharGenCreature.Attributes.38.Total). Grey when not unlocked
  (DAExternalCommands.HasAchievement(sAchievement) false: a trainer or a manual
  opens it), no point, or requirements missing. Before agent v25 these were
  read as plain text and could not be chosen (log 2026-11-17 20:08).
- Store rows (store.gfx StoreItemEntry): no DoubleClickAction; onPressHitArea(
  true) calls Store.StartBuyTransaction / StartSellTransaction; OwnedByStore,
  GetCost() (copper, mark-up applied), IsTooExpensive() (Party.Money.Total).
  Before agent v25 store rows were not recognised as items.
- Active flag (2026-11-19): CCObject byte +0x2C5 = the object is active (in
  play). Proof: (1) the CCObject constructor 0x699670 writes 1 there (0x699BA4);
  (2) 0x698CD0 thiscall(obj, bool) stores its argument there and, when it
  changed, updates the object (vtable 0x154, 0x13C with 1/0): the SetActive
  setter; (3) the script event dispatcher 0x6976B0 returns at once when it is
  0 (inactive objects run no scripts, as the toolset documents); (4) data: the
  area file lot100ar_lothering.are (designerareas.erf, GFF V3.2, CreatureList
  field Active) against the Lothering objdump: all 26 creatures with Active=0
  (outdoor Leliana, refugee ambushers, bears, wolves, hurlocks) read 0, 63 of
  66 with Active=1 read 1; the ones at 0 later are explained by the plot
  (villagers after their scene, Sten's cage copy after hiring, the dog and
  companions parked outside the party at 125,124). Reader:
  tools\gff3.py (GFF V3.2 .are files). DA2 has another mark (see its notebook):
  the analogous CCObject +0x3D4 (0x80000000 from the constructor, DA2 +0x2E4)
  is 0x80000000 for every creature here, so it does not port.
- Target flag (disassembly 2026-11-18): CCObject byte +0x23C, set by vtable slot
  0x134 (0x69B640; CCPlaceable 0x74BDC0) and read by slot 0x138 (0x6951C0;
  CCCreature 0x6A0E70 adds: hostile or +0x7F0 == 0). The Tab target filter
  0x70BFA0 (SelectNextCreatureTargetMessage 0x70EE10) needs it non-zero; the
  constructor 0x699670 sets +0x23C = 0, +0x23D = 1; 0x67CA60 clears it for a list
  of objects and 0x67CB40 sets it back (a scene state). It is NOT a hidden mark
  (log 18:51:54, chantry: 20 of 42 creatures at 0 right after entering, the
  revered mother, priests and guards among them; 0 of 42 at 18:56; Lothering
  89-101 of 111 at 0 at load, 0 seconds later). Interiors use negative world
  coordinates (chantry -37..4).
- Party picker (partypicker.gfx PartyPicker.as, PartyPickerPortrait.as;
  disassembly 2026-11-18): Flash holds only PortraitGroup_mc.Portrait1..4_mc
  (PartyPicker.SelectedPartyMember.N; a click sends
  "GameModeController.UnselectPartyMember N"), CancelButton_mc "Удалить все"
  (ClearPartyPicker), ConfirmButton_mc "Принять" (SubmitPartyChanges), and a
  hover tooltip (PartyPicker.MouseOverName / MouseOverDescription). Candidates
  are chosen in the 3D scene: GameModePartyPicker (vtable 0xB1D31C, 30 slots)
  handler SelectPartyMemberUnderCursorMessage 0x768AE0 raycasts (0x7675B0) into
  the entry array at mode +0x8C..+0x90, 0x34 bytes per entry: +0 creature id, +4
  creature object, +8 state (2 free, 1 chosen), +0x10 locked byte, +0x24 slot;
  skipped while bytes +0xA0/+0xA1 are set; state 2 -> 0x766040(mode, entry)
  thiscall ret 4 (refuses at word +0xA2 >= 4 = chosen count), state 1 ->
  0x7661B0(mode, entry) ret 4; then 0x7658E0(entry). The mod captures the mode
  with vtable thunks like explore/combat ("partypicker=on" in the start log).
- Injuries (portraits.gfx SharedLibrary/CoreStats.as, EffectIcon.as; F10 dump
  2026-11-17): every creature token (GUI.SelectedChar, Party.Members.N,
  Party.PoolMembers.N) has EffectList.N.{AbilityID, RemainingPercent,
  RemainingTime, IsConsideredHostile}; an injury is AbilityID >= 500000
  (EffectIcon.INJURY_ABILITY_ID), row AbilityID - 500000 of table 160
  (_2DA.TABLE_INJURIES), columns NameStrRef hash 846460685, TooltipStrRef
  3395304241, Label 3030163814, Icon 3168647281.
- Placeable actions (disassembly 2026-11-18, cursor code 0x70CAD0 at 0x70CBE3):
  for a placeable (byte +0x14 = 10) the cursor gives action 8 when +0x42C is set
  and CCPlaceable vtable slot 0x100 (0x74C720, thiscall, arg 0 or 1, returns a
  word from the state table at +0x654 with state +0x658) gives a non-zero action
  for 0 or 1. Ids are rows of placeable_interactions.gda (core 2da.erf): 0
  Invalid, 1 Open, 2 Close, 3 Unlock, 4 Destroy, 5 Use, 6 Examine, 7 Disarm, 8
  Flip Cover, 10 Dialog, 11 Loot, 12 Trigger Trap, 13 Topple, 15 Area
  Transition, 16/17 Turn, 18 Repair. Which objects carry which ids: runtime
  proof pending (act= in the log).
- Store screen (store.gfx Store.as, log 17:35): movie file "Store"; tabs All,
  Weapon, Armor, Accessories, Other, Crafting, BuyBack/SellBack (category 7),
  PlotItems; stacks open a PopupLayer "Сколько предметов продать?" with
  Money_mc, Slider_mc (ArrowDown_mc, ArrowUp_mc, SliderNumber_mc "2 / 2"),
  Buttons_mc.Button1_mc "Продать", Button2_mc "Отмена".
- Quest without a marker (log 17:08): lot100pt_sten "Узник кунари" ("talk to the
  revered mother") had no PlotDestination in Lothering (PlotAssist 1 marks only
  explored areas); Lothering's Задания held only "Большой мир" and Доска певчего.
- Equipping into a chosen slot (inventory.gfx patch003, FFDec 2026-11-17):
  double-click sends EquipItem <id> 255 (the game picks the slot, the main
  hand for weapons); a drop on a slot (EquipSlot.EquipItem) and the radial
  "equip alt" send EquipItem <id> <slotId>. Item.EquippableSlots is a bit mask
  (3 = either hand); Item.as lowers it to 1 for a heavy one-hand weapon when
  CreatureUtils.IsDualWieldMaster is false. Backpack items have
  _parent.ListAttributeName containing "Backpack".
- Test design (player, 2026-11-17): the player is blind and cannot walk to a
  place, a person or outdoors without navigation. Ask only for actions done
  on the spot or with features the mod already has.
- Pitfalls met in test 00:05: a clip listed twice under two member names
  (login_mc; the Options tab group also keeps its selected tab in a field), which
  froze Down on a duplicate; the login widget sits off screen at x = -182; open
  picker entries mixed into the rows around them.
- optionsmenu widgets (FFDec decompile of OptionsMenu.as, OptionsUtils.as):
  tabs are StickyButtons; toggles are Behaviour.ToggleButton (state
  SI_ItemSelected, label Label_mc); sliders have SliderValue, UpArrow_mc,
  DownArrow_mc, Value_mc; pickers have PickerButton_mc and build PickerEntry
  buttons when opened. Closing Options calls StartMenu.ShowContents.
- Hidden screens keep their items in the FocusManager: after Credits opened,
  Down moved to the hidden module list (ModuleListAndDescription_mc). StartMenu
  .HideContents calls MovieUtils.HideMovie, which sets _visible = false,
  hitTestDisable and MovieHidden on the parent clip.
- Codex (journal.gfx and notifications.gfx of packages\core\patch\patch003.erf,
  Steam copy, FFDec 2026-11-20, CodexGroup.as / CodexEntry.as /
  CodexGroupButton.as): rows live in _level0.ScrollingCodexList_mc.Scrollable_mc
  .CodexGroupN. A CodexGroup has m_sEntryData (<list>.N), m_nCodexGroup,
  m_bShowingChildren, ShowChildren / HideChildren; its name is <entry>.Name (shown
  in its Entry clip), its progress text "found / total" counts
  <entry>.CodexList.K.Locked == 0; <entry>.Updated marks news. Button_mc is a
  Behaviour.ToggleButton: a push opens (OnButtonAction) or closes
  (OnSecondaryButtonAction). Children_mc holds CodexEntry sticky buttons (rows of
  6, WrapSize 300) with m_sEntryData, m_sResRef, UpdateDescription; they show only
  <entry>.Index as text, the name <entry>.Name is the tooltip; Locked entries are
  Disable()d. Selecting one fills _root.CodexTitle and
  _root.CodexDescription_mc.Scrollable_mc (its TextField is also reachable as
  m_mcTextField: both names were listed, tester log 18:32).
- Notifications (notifications.gfx Alert.as): each alert is an Alert button with
  Type (0 custom, 1 quest, 2 codex, 3 items, 4 level up, 5 money, 6 tactics,
  7 ability, 8 approval, 9 new content, 10 world map, 11 specialization, 12
  connection lost) and Token (catalogue path). Money: <Token>.IntegerArg in copper,
  text LOC_RECEIVED / LOC_LOST + ItemUtils.FormatMoneyString, which puts the
  coins as <img src='ico_gold|ico_silver|ico_copper'> pictures, so the text read
  is "Получено  1  76". Items: only LOC_ITEMS_RECEIVED, no names in the movie
  (what the token holds: HYPOTHESIS until the "items alert" log line).
- Pause sign (floatylayer.gfx FloatyLayer.as): PauseIndicator_mc and
  PauseBorder_mc follow the catalogue value Client.Paused through a token observer
  (intro / outro animations; Client.UserPaused picks outro). Reading
  Client.Paused with GetValue is a HYPOTHESIS until the v35 test.

## Game folder changes

Added (deleting them returns the game to normal; nothing was replaced):

- Steam folder F:\SteamLibrary\steamapps\common\Dragon Age Ultimate Edition,
  added 2026-11-20 by deploy.ps1 (player approved): bin_ship\dinput8.dll
  (Ultimate ASI Loader 9.7.4 x86) and bin_ship\scripts\ with DAOAccess.asi and
  DAOAccess\ (prism.dll, daoaccess.gfx, third-party\prism\mpl-2.0.txt, NOTICE);
  DAOAccess.log appears at the first start. The mod files and the LAA exe of the
  old GOG folder went away with that folder.
- The Documents files below stay from the GOG time and are used by the Steam
  copy too (same Documents\BioWare\Dragon Age folder; the game ran there
  2026-11-20 and rewrote DragonAge.ini, addins.xml, offers.xml).
- %USERPROFILE%\Documents\BioWare\Dragon Age\packages\core\override\daoaccess.gfx
  (our Flash agent; the folder already held the player's own "enpy" folder,
  left untouched)
- ...\packages\core\override\deathblow_all\creatureranks.gda (2026-11-17,
  player asked for "deathblow always"): the game's own creatureranks.gda
  (core\data\2da.erf) with fDeathblow set to 1.0 for Critter, Normal,
  Lieutenant, One-Hit Kill, Weak normal (were 0.2, 0.2, 0.75, 0.2, 0.2).
  Same idea as Nexus mod 220 "Deathblow All", which could not be downloaded
  (Nexus bot check; Chrome extension not connected). Copy kept in
  tools\creatureranks_deathblow_all.gda. Delete the folder to undo.
- ...\packages\core\override\random_encounters_always\ (2026-11-17, player
  chose "always" and Denerim too): rand_forest, rand_highway, rand_plains,
  rand_denerim .gda from core\data\2da.erf with TriggerChance 100 for every
  non-repeatable row (Repeat 0); MERCHANT (Repeat 1, Tegrin) and Denerim
  STEALING_2 left as they were. Plot conditions untouched. Copies in
  tools\random_encounters_always\. Delete the folder to undo.

- Russian text, added 2026-11-20 (player approved), copies of the ENPY files in
  Documents\...\override\enpy: ...\Dragon Age Ultimate Edition\packages\core\data\
  talktables\core_ru-ru.tlk and core_ru-ru_f.tlk, packages\core_ep1\data\talktables\
  core_ep1_ru-ru.tlk and _f, modules\single player\data\talktables\
  singleplayer_ru-ru.tlk and _f. Nothing replaced; delete them to undo.
  Player's personal backup (2026-11-20 evening, on request): %USERPROFILE%\Documents\
  Русификация Dragon Age Origins (ENPY).zip, 62 MB, 221 files: 1-Dokumenty-override\enpy
  (the whole override\enpy folder), 2-Papka-igry (these six talk tables) and
  Инструкция.txt (steps, Locale=ru-ru, ENPY link and terms). Never publish or ship
  it (ENPY terms, root NOTEBOOK.md Decisions).

Changed:

- ...\Dragon Age Ultimate Edition\bin_ship\DAOrigins.exe replaced 2026-11-20 (player
  approved, after the camp test showed 1965 of 2047 MB used) by the Steamless copy
  with LAA: games\dao\tools\make_laa_exe.py on games\dao\work\DAOrigins.unpacked.exe
  (security directory that pointed past the end cleared, Characteristics 0x102 ->
  0x122, checksum 0x9cae6d; 8 bytes differ from the Steamless copy), 10 217 312
  bytes, SHA-256 b74dca7a1b1b69f9242d94afb47b2f67e8c655df89914352cca7d31f76dd8f45,
  entry 0x4e3024 in .text (so the installer sees it as not wrapped, LAA on). The
  Steam original is bin_ship\DAOrigins.exe.before-laa (SHA-256 400c2c9e...47bd).
  Undo: copy it back, or Steam "Verify integrity of game files"; a Steam update
  also brings the wrapped exe back (the mod still works, only 2 GB again).
  Since installer 1.2 the installer makes this same file (byte for byte, SHA-256
  b74dca7a...8f45) for any Steam player who agrees: root NOTEBOOK.md, Decisions, LAA.
  Proven 2026-11-20 13:43: it starts from the Steam launch option, the mod loads,
  memory line "used 1973 of 4095 MB address space, largest free 2046 MB".

Changed (settings, not game files):

- %USERPROFILE%\Documents\BioWare\Dragon Age\Settings\DragonAge.ini: Locale=en-us
  -> Locale=ru-ru (2026-11-20, player approved; VOLanguage=en-us kept), to use the
  ENPY Russian text in override\enpy. Backup DragonAge.ini.before-ru next to it.
  Undo: set Locale=en-us again.

- Steam launch options of app 47810 (2026-11-20 13:07, player approved): Steam
  closed with steam.exe -shutdown, then "LaunchOptions" added to the block
  UserLocalConfigStore\Software\valve\Steam\apps\47810 of
  C:\Program Files (x86)\Steam\userdata\<the player's Steam id>\config\localconfig.vdf
  (value: the bin_ship\DAOrigins.exe path in quotes, then %command%), Steam
  started again. Backup localconfig.vdf.before-access next to it. Undo: clear the
  field in Steam, game properties, General, Launch options.
  Since installer 1.4 the installer does the same for any Steam player who agrees
  (root NOTEBOOK.md, Decisions, launch option).
  Proven with the real Steam 2026-11-20 21:50 (Claude had removed the line
  first; installer 1.4 wrote it back, closing and reopening Steam; root NOTEBOOK.md
  Test log).

- %USERPROFILE%\Documents\BioWare\Dragon Age\Settings\KeyBindings.ini:
  ZoomIn_1 and ZoomOut_1 (were Keyboard::Button_Next / Button_Prior) set to
  (UNASSIGNED) on 2026-11-17. Original kept as KeyBindings.ini.before-daoaccess.

## Puzzle map

Places that are not walk / talk / fight, found 2026-11-20 by listing every
resource name in every .erf/.rim of the install (tools\listall.py, ERF V2.0 and V3.0 names)
and reading the flag names of the plot files (.plo, GFF V4.0, UTF-16 strings).
Plot flags are proven; how each puzzle plays is from memory of the game, a
HYPOTHESIS until seen in a log or a video. Area codes are .are names.

- Main campaign, mandatory:
  - Circle Tower Fade, cir300-cir362 (Broken Circle): plt_cir300pt_shapeshifting
    (forms mouse, spirit, burning man, golem) and plt_cir300pt_fade_portal (islands
    Raw Fade, Burning Tower, Darkspawn Invasion, Mage Asunder, Templar's Nightmare,
    Weisshaupt, three green portals, Sloth Demon). The biggest risk: a maze of
    islands, each passage needs a given form. Needs scanner names for portals,
    mouse holes, burning doors and a "current form" line.
  - Gauntlet, urn230ar_the_gauntlet (Urn of Sacred Ashes): riddles in dialogue
    (RIDDLE_OPEN_RIGHT_DOOR / WRONG_DOOR, wrong answer = a fight), the bridge
    (plt_urn230pt_bridge: GHOST_TILE_APPEARS, SOLID_TILE_APPEARS, PLAYER_FELL;
    12 triggers urn230tr_bridge_switch_01-12, urn230ip_bridge_puzzle.dlg,
    urn230_party_bridge_help.dlg), the fire (EQUIPMENT_REMOVED, PC_PAST_THE_FIRE:
    take off all gear). The bridge is the second biggest risk.
  - Haven temple, urn200: plt_urn200pt_brazier (use a pearl or a taper on a
    brazier): an item use, low risk.
  - Ortan Thaig, orz530: plt_orz530pt_statpuzzle (3 doors, 3 traps, solved):
    medium risk, layout unknown.
- Origin stories (only one runs): Dwarf Noble, bdn210ar_thaig_chamber:
  plt_bdn210pt_shield_puzzle (3 floor plates, CAN_USE_RING). Dalish Elf,
  bed110ar_elven_ruins: mirror, pressure plate, statue room (traps, low risk).
- Awakening: Blackmarsh Fade stb200: plt_stb200pt_stone_puzzle (6 stones in a
  sequence, STB_PUZZLE_SEQUENCE_STONE_1-6_CORRECT): high risk if the order is shown
  only by visuals. Vigil's Keep deep roads vgk320/vgk330: puzzle pieces a-d, Korth
  statue, runestone, barrier switch. Wending Wood trp100: 8 statues to find
  (plt_trp100pt_statues). Silverite mine trp200, Kal'Hirol ltl*: levers.
- DLC: Golems of Amgarrak (gib: phased creatures, fin switch, levers), Witch Hunt
  (str: mirror shards, str510 mirror, door switch, statue), Leliana's Song (lel220:
  three gate levers). Not checked inside yet.
- Next: the plan is in NOTEBOOK.md, Puzzle plan (prepare from files, check on the
  spot, Circle Tower Fade first).

## Pitfalls

What went wrong and how to avoid it.

- The engine detector picked redist\PhysX_...exe as the main exe. The real game
  is bin_ship\DAOrigins.exe.

## Backlog (not urgent)

- Found in the log of 2026-11-21/22 (test log), offered to the player 2026-11-22,
  not chosen yet:
  - item tooltips (describe, inventory and containers) run name, type and
    properties together: "Серебряное кольцоКольцо", "Клобук ученикаМатерчатый
    шлем  +5 к психической устойчивости" (14 of 31 describe lines);
  - the achievement popup GUIAchievementUnlocked reads "без подписи" (radio item
    _level0.Achievement_mc.AchievementWindow_mc);
  - chargen appearance: slider arrows ...SliderGroup_mc.subentry_mN.ArrowDown_mc
    read "без подписи".

- Menu opening delay (player, 2026-11-17: about a second on every menu). Log:
  movie new -> screen took 2 s (identity check waited 1 s, agent readiness
  checked 1 s after loadMovie). Fixed 23:00, proven in test 23:30: identity checked
  every 50 ms until _root.Filename is known (or 1 s), agent readiness polled
  every frame (next URL still after 1 s), young movies (< 1.5 s) scanned
  every 100 ms and announced when two scans give the same count; log line
  "movie ... identified after N ms". Check "movie new" -> "screen" times.

- Reinforcements (player idea, 2026-11-17: the war should feel big, many
  enemies, not smarter ones): when a fight starts, spawn extra hostile
  creatures of the same kinds near the enemies, through the engine's own
  create-object path. Needs research (the creation function, templates of the
  creatures in the fight). Only after the crashes are settled: more creatures
  means more memory. No ready Nexus mod does this (search 2026-11-17); the
  nearest are Climax Army Enhanced (5513, final battle) and Redcliffe Better
  Siege (510).

## Open questions

- Crashes (2026-11-17, Windows event log, all access violations inside
  DAOrigins.exe on a null pointer): 20:33 at 0x53CC40 (getter mov eax,[ecx+0xC],
  level-up screen), 20:45 and 20:46 at 0x86883C (right after "Продолжить", save
  loading; the third try loaded), 21:54 at 0x874AB7 (render-state setter,
  mov edx,[ecx] with ecx = [this+0x48]; walking 284 m across Ostagar, 114
  objects). Cause proven by the memory log of the session 22:21-22:50 (LAA
  on): the game used 2035-2069 MB of address space (private about 1450 MB) in
  Ostagar and the Korcari Wilds, above the old 2048 MB limit; no crash. LAA
  set 22:20; the mod now logs "memory: used X of Y MB address space, largest
  free, private" every 30 s and "CRASH ..." lines (address, registers, exe and
  MOD+offset words on the stack) from an unhandled-exception filter. Next crash:
  read those lines first; a MOD+ word near the top means our code is involved.
- Logitech GamePanel support (PCGamingWiki): the game may already push the
  selected character's name, health and mana to an outside API. HYPOTHESIS, a
  possible cheap data source; settle by finding the LCD code in the exe.
- Plain screen text (titles, descriptions, dialogue) is not read yet; the agent
  lists only widgets.

## Test log

One line per in-game test: date, what was tested, result.

- 2026-11-18 20:53-21:41: camp inventory with agent v27. Gifts given by Enter
  worked (Алистер, Морриган, Лелиана +50 each). Player: given gifts still read in
  the backpack; the log shows the Feastday DLC swapping each one for a same-named
  keepsake. Marked in agent v28. Party picker proven 21:37 (Enter added 7330,
  15166, 17807: "4 chosen", hero 130 locked). Injuries proven 21:40 ("Айдан
  получает травму: Треснувший череп", then "Айдан: травмы вылечены"; the name
  came from 2DA 160 NameStrRef in the Minimap movie).

- 2026-11-18 19:24: save loaded inside the Lothering chantry, categories v8.
  Scanner showed only the two companions and the exit: the object walk stopped
  at the id gap (party up to 7330, chantry 16927+). Earlier visit 18:51 had also
  hidden the revered mother and priests through the +0x23C filter. Both fixed
  19:35 (see Store and quests v2).

- 2026-11-18 18:50-18:52: Lothering, categories browsed. Задания now 7 useful
  entries (Певчий Девонс, Доска певчего, Старейшина Мириам, Стэн, two board
  markers, Баннорны). Player: the church door hides among objects, "Купец" in
  Места, loot scattered. Fixed in categories v8.

- 2026-11-18 18:10-18:27: Lothering with agent v26. Player: "Задания" full of
  useless people (40 refugees etc.), Enter at the church did not enter: he used
  the map note "Церковь" (Enter on a note only walks) while the door was the
  object "Церковь". Fixed 18:40 (see Store and quests v2). No walk cancel tried.

- 2026-11-18 16:19-17:37: camp and Lothering. Doubles gone in the camp, approval
  of all three companions, approval line in the inventory, Morrigan approval
  notifications (each spoken twice), store prices, buy, sell with the quantity
  popup. Player: store is one long list (section mode was off: movie "Store"),
  could not find the revered mother (no marker), camp lists four "Эмиссар ..."
  not there yet, Flemeth listed earlier in the Korcari Wilds. Log: Dedupe hid
  same-named bodies and crates in Lothering. Fixed in agent v26.

- 2026-11-18 15:05-15:18: inventory full list proven (1 из 53), level-up
  (attributes, talents in all five warrior branches), character record with
  companions, camp scanner, conversation with Морриган, approval notification
  spoken. Player: two "Морриган" in the camp (map note + person; also "Пес",
  "Алистер", three "Карта мира"), asked about gifts, approval and missed list
  entries. Found by review: specializations unselectable, store rows not items.
  Fixed in agent v25 (see Companions and lists v1).

- 2026-11-16 22:19: probe + speech, launched the usual way. Loaded in
  DAOrigins.exe, NVDA lines Success, window up. Player reports: arrow keys do
  nothing in the menus, and NVDA OCR mouse routing does not move the game
  cursor.
- 2026-11-16 23:37: menu reader v1, main menu. Arrows move the focus and each
  press logs one new item; Enter pressed Credits. All spoken as "без подписи"
  (wrong label path). Arrows also walked hidden module-list items behind
  Credits. Holding a key moved only once (no repeat yet).
- 2026-11-16 23:45: menu reader v2 (FocusManager). Main menu read in Russian,
  order top to bottom, hold repeats, module list and Назад work, Enter opened
  Options and New Game. Options and New Game screens silent (no FocusManager).
- 2026-11-17 00:05: Flash agent v1. Loads everywhere via "daoaccess.gfx". Main
  menu and Options Game tab spoken with kinds and values; tab switch and picker
  open work; Down stuck on a duplicated Audio tab; New Game buttons unlabeled;
  picker entries mixed with other rows.
- 2026-11-17 00:25: Flash agent v2. All fixed: picker opens onto its entries
  ("Легко, 1 из 4"), picking returns to the picker with the new value, toggles
  flip and speak the new state, sliders step 1.00 with Left/Right (music 10 to 5
  and back), tab switch rereads the tab, Cancel returns to the main menu, New
  Game reads "Закрыть", "Dragon Age: Начало", "Dragon Age: Пробуждение", Quit
  works. Only flaw: off-screen login button still listed.
- 2026-11-17 00:39: agent v4, character creation reached. Popup message and
  button read, chargen header and description texts read, Tab skips texts,
  disabled icons marked. All 14 chargen icons "без подписи"; an unlabeled
  CharGenDragArea_mc first in the list. Nothing chosen yet.
- 2026-11-17 01:07: agent v5. Chargen icons named ("Человек", "Воин", "Знатный
  человек"), choosing works and reads the description; class and background
  unlock after race; Next leads to appearance, voice sets chosen, Next leads to
  attributes. Flaws: Next announced the last item instead of the new scene,
  then a flood of 15 changed texts; appearance columns interleaved; -/+
  attribute buttons "без подписи"; name field not editable; scrollbar arrow
  "без подписи".
- 2026-11-17 01:23: agent v6. New chargen steps announced as scenes, the two
  appearance columns read one after the other, name typed and kept ("галар"),
  attribute rows "Сила, 15" with descriptions read on each move. Player: points
  cannot be spent (Left/Right gave "none", Enter pressed Minus: code misplaced).
  Player: the menu structure keeps getting more comfortable.
- 2026-11-17 01:35: agent v7. Attribute points spent with Right (5 to 0, each
  step read with "Расходуемых очков"), skills and talents chosen by Enter (icons
  "без подписи"), difficulty popup and picker, first conversations answered,
  tutorials read, in-game Esc menu read. HUD movies (Quickbar, FloatyLayer,
  Notifications) took the arrows in the world.
- 2026-11-17 01:56: agent v8. Save loaded through "Продолжить", pause spoken,
  K opened Skills with every ability named and described, a talent learned,
  Character Record and tutorial popups read; the HUD stayed silent on arrows.
- 2026-11-17 02:19: F10 catalogue dump three times (start, after about 3 s of
  W, after more W). Dumps fine, no freeze reported; positions changed as
  expected.
- 2026-11-17 02:31: scanner v1. Player: objects are found ("объекты мы
  видим"); ], [, Home spoke the 11 entries with distance and direction. End
  (three SendMessage forms of MoveCharacterToLocationMessage, twice each):
  leader did not move (0.00 m every time).
- 2026-11-17 02:42: direct move call. End on "Главный зал" (15 m): moved 9 m
  in 2 s, arrived ("рядом"); End on "цель задания, Фергюс" (105 m): walked,
  conversation with Fergus started. Object position found at object +0x120
  (x, y, z). Two tutorial popups closed with Enter on "Закрыть"; the journal
  tutorial then opened the journal itself, which the player took for a
  popup that would not close. Player asked for PageUp/PageDown keys,
  categories, and inventory work.
- 2026-11-17 (player away): built without testing: scanner keys and
  categories, walk watch, engine objects, Enter interaction, inventory items
  and slots, Space descriptions, conversation lines.
- 2026-11-17 03:30: big test, part 1. Mode capture on (explore and combat).
  Engine objects with real names (275 in Castle Cousland: about 19 people
  ("Стражник", "Дункан", "Тэйрн Брайс Кусланд", "Гигантская крыса"), about 70
  doors, chests, books, sacks...). End to "Стражник" arrived (0.64 m). Enter on
  a guard: action 3 (talk); Enter on "Сундук": action 8 (use), the leader
  walked there and the locked-item tutorial came up. "Выходы" empty in the
  castle. CCItem entries were worn gear with false positions (dozens of guard
  shields "23 m"). Walk to the cook said "stopped" after 1.5 s while the game
  was still starting. Then the inventory training tutorial (text only, waits
  for the player to drag an item) took the keys and could not be closed.
  Fixed: text-only popups are read passively and never take the keys; CCItem
  dropped from the scanner; stop detection 3 s.
- 2026-11-17 03:53: big test, part 2. First fight (giant rats in the cellar),
  poultice put on quickbar slot 10 with Ctrl+0, a crate opened with Enter
  (container screen read), walk to the cook arrived. Player: the inventory is
  hard to move around, enemies must be quick to target in combat, killed
  enemies stay in the scanner. Fixes deployed 04:40 (see Scanner and
  Inventory).
- 2026-11-17 16:55: scanner v4, Human Noble prologue. Quest walks worked (cook,
  Fergus 112 m, chest), conversations read, inventory equip worked (armour put
  on), dead archer looted, save over a slot, quit to menu. Problems: after the
  chest the quest marker vanished and the player did not know where to go
  (tried locked doors, chests, Деррен and Айона with no result, one door four
  times with no feedback); "Пришли: " with an empty name (quest:218); [ jumped
  to an enemy 115 m away (55 soldiers listed over the castle); Enter on an
  enemy pressed about 15 times with only "Атакую" each time (no combat
  feedback). Fixes in scanner v5.
- 2026-11-17 17:45: scanner v5. Quest texts read, quest change announced after
  the conversation, door labels work. Player: the quest key picks unrelated
  quests (Delete gave the Shale DLC quest each time, the castle quest only after
  it changed), and combat is chaos (Enter pressed about 30 times per soldier
  with only "Атакую"; the cached friendly flag left hostile soldiers unmarked).
  Walk to the quest marker "Главный зал" 71 m: order accepted, the leader did
  not move (3 times; enemies 15 m away; cause unknown).
- 2026-11-17 19:21: scanner v6 + quickbar. Combat narration, quickbar echo,
  cooldown end, sustained on/off and target-pending ability all worked (see
  Scanner). Player asked for the selected party member to be spoken (F1..F4).
  Level-up not reached this time.
- 2026-11-17 20:03: list screens v1, party v1. Inventory, record, skills and
  level-up navigable by sections; party selection spoken. Player: switching
  skill branches on level-up is awkward (fixed 20:35).
- 2026-11-18 01:30-02:01: perf probe session (Tower of Ishal, Flemeth's hut,
  random encounter, camp, world map). No stall over 150 ms except loads; mod
  time 120-490 ms per 10 s while exploring (1-5 %), 730-890 ms on the world
  map (7-9 %); single calls of 30-80 ms in Minimap (scanner keys) while
  "scan timing" showed refreshes under 30 ms. Suspect found: LogLine opened,
  wrote, FlushFileBuffers and closed the 9 MB log on the game thread for
  every line. Fixed 02:30 (not tested): log.cpp queues lines, one background
  thread writes through one open handle, no flush (written data survives a
  process crash); LogLineNow writes at once for the crash filter; a log over
  20 MB moves to DAOAccess.old.log at start. Idle screens (no key for 3 s)
  rescan every 1000 ms instead of 400. Player also reported: the inventory
  reads only about 12 items, and asked for open / closed world map places.
- 2026-11-18 01:06-01:24: Ostagar, Tower of Ishal floors 1-3. Player: heavy
  lag of the whole game and the scanner. Log: memory steady (2.0-2.1 GB of
  4), no crash, no "objects ... read in" line (object walk under 40 ms), 88
  movies created (inventory/book screens), nothing repeating abnormally. No
  timing existed, so the cause is not proven. Changes 01:40 (not tested):
  - "perf: ..." every 10 s when a frame stall (> 150 ms between Advance calls)
    happened or the mod spent > 30 ms: mod time, worst call and its movie,
    game Advance time, stalls and the worst one (gfx.cpp AdvanceHook);
  - "scan timing: ..." when a scanner refresh takes > 30 ms (world, objects,
    names and how many asked);
  - HUD movies (Tooltips, Quickbar, Portraits, Minimap, NavBar, ArmyControl,
    SaveScreen) are no longer scanned every 400 ms (never read); FloatyLayer
    (a clip per creature) only under _level0.PauseIndicator_mc (agent v23
    DAOAccess.ScanUnder); Notifications still fully;
  - catalogue names asked once per object; only creatures within 60 m are
    re-asked every 4 s (before: every creature of the area on every press).
- 2026-11-18 00:08-01:00: party v2 + scanner fixes, Ostagar. Hold position
  was on at load, spoken, turned off with H; companions followed after that.
  The battle of Ostagar showed no enemies at all ("Враги 0" while being hit;
  "Айдан без сознания" spoken): every object id there was above 8000 (party
  mage 8644), the old fixed limit of EnumObjects. Fixed 01:10, not tested: the
  walk goes past the highest id ever seen until 4000 free ids in a row; a slow
  walk (> 40 ms) logs "objects: N read in X ms, highest id".
- 2026-11-17 23:30-00:01: scanner v7 in the Korcari Wilds. Works (player:
  "все вроде бы работает"); companions stayed behind (up to 121 m) and the
  player fought alone; enemy filter dropped 58 darkspawn (fixed).
- 2026-11-17 22:21-22:50: first session with LAA (Ostagar, Korcari Wilds,
  quit to menu): no crash, address space peaked at 2069 MB.
- 2026-11-17 21:54: game crashed while walking across Ostagar (no test of a
  feature; see Open questions, crashes). Earlier unreported crashes at 20:33
  and 20:45-20:46.
- 2026-11-17 21:28: off-hand equip (Shift+Enter). A heavy weapon gave "heavy"
  (refusal spoken), then "Кинжал" went to slot 1; player confirmed the dagger
  is in the off hand.
- 2026-11-20 (Claude, no game run): Steam copy checked from files: same build
  as GOG under SteamStub, all mod addresses found (Proven facts, Steam); vtable
  slot counts fixed to 32 / 32 / 30. In-game test of the Steam copy pending.
- 2026-11-20 ~12:55 (player): Steam copy started from Steam: the launcher showed
  no Play button to OCR, the game never started (no DAOAccess.log). Next: Steam
  launch options with bin_ship\DAOrigins.exe %command%.
- 2026-11-20 13:12-13:22 (player): Steam copy with the launch option: started
  without the launcher, mod loaded (ts=0x4eb19d53 size=0xa49000), scanner in camp,
  Действие on Morrigan, conversation replies, approval, pause menu, save screen,
  start menu, all spoken. Game text English (Steam has no Russian). Memory at
  1965 of 2047 MB. Launch option proven.
- 2026-11-20 13:43-13:48 (player): Steamless + LAA exe started from Steam, mod
  loaded, 4095 MB address space, played and saved, no crash. LAA exe proven.
- 2026-11-20 14:01 (player): Locale=ru-ru only: text stayed English, the game
  wrote Locale=en-us back (no core_ru-ru.tlk in the install folder). Talk tables
  copied, Locale=ru-ru set again; test pending.
- 2026-11-20 14:54 (player): Russian text works (start menu, options tabs and
  lists in Russian); 4095 MB address space. One English tutorial popup at start
  ("Shops & Merchants" / "Tutorial Body Text", likely an untranslated DLC
  string). Russian text proven.
- 2026-11-20 (a tester on another PC, release 0.2.0, store unknown, via the
  player): after creating a new character a training tutorial about the camera
  could not be dismissed, and from then on "nothing could be done". No log yet:
  asked for that PC's bin_ship\scripts\DAOAccess.log (and DAOAccess.old.log).
  Origins tutorials known so far: TutorialContainer_mc.Fade_mc.Tutorial_mc with
  TutorialNextOKButton_mc ("Next" / "Далее"), and text-only training popups read
  passively since 2026-11-17.
- 2026-11-20 15:56-15:57 (player): "cannot choose the campaign" on New Game. The
  log shows the buttons GameButton0_mc / GameButton1_mc ("Dragon Age: Начало",
  "Dragon Age: Пробуждение") and then the same names again as text items
  (GameLabel0_mc / GameLabel1_mc); Enter landed on the label text (reread), the
  buttons were never pressed. Agent v34: a "...Label..." clip beside a visible
  "...Button..." widget is not listed (A.IsButtonLabel). Deployed, not tested.
- 2026-11-20 16:01-16:18 (player): new game, city elf, area "Дом" (home in the
  alienage). Scanner Задания: "цель задания «Праздничный день», Цирион, 8 м" (a
  plot destination; the player had set Сюжетные подсказки to all areas) and
  "по заданию «Праздничный день», Сорис, 117 м". Enter on Цирион: talk, the
  conversation opened. Enter on Сорис (3 times, 117-127 m, outside the house,
  exit "Эльфинаж" 5 m away): interact result 1, action 3, but the leader never
  moved. Fix (core scanner.cpp kUseFar): out of a fight, Enter on a target farther
  than 10 m walks there first (StartWalk) and runs the default action on arrival
  ("use on arrival" in the log); a stop short or an area change drops it.
  Proven 16:25-18:08 (next line).
- 2026-11-20 16:24-18:10 (player, Steam, agent v34): walk then use proven: "use on
  arrival" for the exit Эльфинаж, Сорис and Дункан (talk), Валендриан, barrels and
  racks (loot). Problems: codex groups read "N / M, флажок", found entries by
  number only; money alert "Получено 6" (coins are pictures); "Предметы получены"
  without names. Fixed in agent v35 (Stages, Codex, money and pause v1).
- 2026-11-20 13:22-18:46 (tester, GOG build, release 0.2.0 agent v30; log in
  testers logs\, not in git): the same codex problems, plus: after opening a group
  the list jumped before the tabs and Up stuck on its first row (20 presses), the
  codex text was spoken twice and could only be heard again from the start, and
  "Пауза снята" / "Пауза" every 1-4 s while paused. Fixed in agent v35.
- 2026-11-21 00:20-00:41 (player, Steam, release 0.3.1 agent v38): end of the city
  elf origin (Эльфинаж, Лавка Аларита, Дом). Worked; the achievement popup read
  "без подписи" (Backlog).
- 2026-11-21 22:14 - 2026-11-22 02:59 (player, Steam, agent v38, log read by
  Claude 2026-11-22): new mage character, chargen, the Harrowing in the Fade, the
  Circle Tower (Комнаты учеников, Комнаты старших магов, Пещерные кладовые,
  Башня магов, подвал). No crash, memory up to 2115 of 4095 MB. Worked: fights
  with spells (quickbar, "Атакую", kills, "Бой окончен"), walk then use on chests
  and bodies, Take all, quests, codex alerts, conversations, level-up with the
  ability chains of agent v38 ("Начало цепочки"; the shared v37/v38 change works
  in Origins too). Problems in Backlog (2026-11-21/22): joined item tooltips,
  unlabeled achievement and chargen arrows, the silent locked door "Вторая дверь"
  (walks behind it stopped at 20 m, "Остановился"). The player's own report of
  this play (2026-11-22): Shock never worked, enemies dropped out of Враги when
  fights began (Sloth demon only in Задания), received items unnamed, door state
  and what can be used unclear; causes and fixes in Stages, Scanner, Fixes v9.
- 2026-11-22 15:54-16:25 (player, Steam, agent v40, Fixes v9 and the engine
  object list; log read by Claude): Ostagar, the Korcari Wilds fights (wolves,
  genlocks, hurlocks). No crash, memory up to 2240 of 4095 MB, 4 stalls (area
  loads). Worked: the engine object list, "Бой! Врагов рядом: N" at each fight,
  ambient lines and floating texts, "Обыскиваю: X", herbs picked after a walk.
  Failed: Shock and every aimed ability (fault -1, player heard "Нельзя применить
  на эту цель"), received item names (lists empty). Fixes v10 built for both.
- 2026-11-22 17:28-17:31 (player, Steam, Fixes v10): Shock and Lightning cast
  with Enter on a Hurlock in combat, both hit (cast pending -> 1). PASSED.
