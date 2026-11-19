# Dragon Age Access

Моды, которые делают Dragon Age: Origins и Dragon Age II доступными для незрячих игроков: игра говорит через экранный диктор.

English below.

## Что это

- Два мода с общим ядром: DAO Access для Dragon Age: Origins и DA2 Access для Dragon Age II. Обе игры сделаны на одном движке, поэтому сканер, ходьба, чтение меню и речь у них общие, а своё у каждой игры только то, что зависит от её файла exe.
- Мод подключается к игре и озвучивает её через ваш экранный диктор с помощью библиотеки Prism. Проверено с NVDA.
- Мод говорит по-русски. Тексты самой игры звучат на языке, выбранном в игре.
- Это ранние тестовые версии. Пожалуйста, сообщайте о проблемах во вкладке Issues и пишите, в какой игре.

## Что уже умеет

Dragon Age: Origins (пройдены пролог, Остагар, Дикие земли Коркари, Лотеринг):

- Читает меню: главное меню, настройки, создание персонажа целиком (раса, класс, внешность, характеристики, навыки, таланты, имя), диалоги с вариантами ответа.
- Инвентарь, лист персонажа, повышение уровня, умения, специализации, магазин: разделы переключаются стрелками влево и вправо, предметы читаются с описанием, покупка и продажа работают.
- Подарки спутникам, одобрение спутников, травмы, выбор отряда в лагере.

Dragon Age II (пройдены создание персонажа и пролог):

- Читает меню: стартовый экран, главное меню, создание персонажа (выбор героя, внешность, предыстория, сложность), паузу, сохранение, инвентарь, лист персонажа, обучающие подсказки, реплики в диалогах.

В обеих играх:

- Сканер: список того, что вокруг, с расстоянием и направлением, по категориям: персонажи, статисты, добыча, объекты, выходы, места, задания, отряд, враги.
- Автоходьба к выбранному объекту, отмена клавишами движения.
- Текст текущего задания.
- Бой: список ближайших врагов, атака клавишей Enter, здоровье врагов и отряда, сообщения об убитых.
- Панель быстрого доступа: названия умений, перезарядка, включённые режимы.

## Что нужно

- Windows, экранный диктор (NVDA рекомендуется).
- Dragon Age: Origins - Ultimate Edition. Мод проверен на DAOrigins.exe версии 1.05.13263.0 (GOG).
- Dragon Age II версии 1.04 (дополнения поддерживаются). Мод проверен на DragonAge2.exe версии 1.04.8524.0.
- На других сборках игры (Steam, EA app) мод может не заработать. Установщик предупредит об этом. Если всё же попробуете, приложите журнал мода к сообщению об ошибке.

## Установка

Один установщик для обеих игр. Он сам скачивает последнюю версию мода из интернета, поэтому нужен интернет.

1. В разделе Releases справа на странице скачайте файл DragonAgeAccess-Setup.exe.
2. Запустите его. Windows может предупредить, что издатель неизвестен: у установщика нет платной цифровой подписи. Тогда нажмите «Подробнее», затем «Выполнить в любом случае». Потом Windows спросит разрешение на изменения: нажмите «Да». Установщику нужны права администратора, потому что игры часто стоят в Program Files.
3. Установщик сам найдёт игры (GOG, Steam, EA app, обычная установка) и спросит, для какой ставить мод. Можно выбрать одну игру, обе сразу или указать папку игры вручную.
4. Дальше установщик задаёт вопросы в обычных окнах с кнопками. Везде можно отвечать «Да». Он:
   - скачивает последнюю версию мода для выбранной игры;
   - копирует файлы мода в папку игры и в папку override в Документах, а потом проверяет, что они на месте;
   - включает игре доступ к 4 ГБ памяти, иначе игра может вылетать в больших локациях (копия исходного exe сохраняется);
   - снимает с камеры клавиши Page Up и Page Down, они нужны сканеру; колесо мыши продолжит управлять камерой (копия файла настроек сохраняется). Если игра ещё ни разу не запускалась, мод сделает это сам после её первого запуска;
   - предупреждает, если ваша версия игры ещё не проверялась с модом.
5. Запустите игру как обычно. При запуске вы услышите «Dragon Age Access загружен» в Origins или «Dragon Age 2 Access загружен» в Dragon Age II.

Обновление: просто запустите тот же установщик ещё раз. Он сравнит установленную версию с последней и предложит кнопку «Обновить до версии…». Скачивать установщик заново не нужно.

Если установщик сообщит, что файлы мода пропали сразу после установки, их удалил антивирус: загрузчик модов dinput8.dll часто ошибочно считают угрозой. Добавьте папку игры в исключения антивируса и запустите установщик ещё раз.

Установка вручную, без установщика: в архиве DAOAccess или DA2Access с номером версии есть папки bin_ship и override. Содержимое bin_ship скопируйте в папку bin_ship игры. Файл из override скопируйте в Документы\BioWare\Dragon Age\packages\core\override для Origins или в Документы\BioWare\Dragon Age 2\packages\core\override для Dragon Age II. Если этой папки нет, создайте её.

## Удаление

Запустите установщик ещё раз, выберите игру и нажмите «Удалить мод». Он удалит мод и предложит вернуть исходный exe игры и клавиши камеры.

Вручную: из папки bin_ship игры удалите файл dinput8.dll, файл scripts\DAOAccess.asi или scripts\DA2Access.asi и папку scripts\DAOAccess или scripts\DA2Access. Из папки override в Документах удалите daoaccess.gfx или da2access.gfx.

## Основные клавиши

В меню:

- Стрелки вверх и вниз: двигаться по пунктам.
- Enter: нажать.
- Tab и Shift+Tab: следующая и предыдущая группа пунктов.
- Стрелки влево и вправо: менять значение (списки выбора, ползунки, очки характеристик). В инвентаре, листе персонажа, повышении уровня и магазине: переключать разделы.
- Home и End в списке: первый и последний пункт раздела.
- Пробел: описание предмета или умения. В диалоге: повторить реплику собеседника.
- Shift+Enter на оружии в рюкзаке: надеть во вторую руку.
- Dragon Age II, выбор героя: пункт «Выбор героя», стрелки влево и вправо переключают героев, Enter выбирает.

В мире:

- Page Down и Page Up: следующий и предыдущий объект в сканере.
- Ctrl+Page Down и Ctrl+Page Up: категория сканера.
- Home: повторить текущий объект. В бою: состояние боя.
- Shift+Home: здоровье, травмы и расстояние до каждого в отряде.
- End: идти к выбранному объекту. Клавиши движения отменяют ходьбу.
- Shift+End: позвать отставших спутников к себе.
- Enter: действие с выбранным объектом: поговорить, открыть, взять, атаковать.
- Правая и левая квадратная скобка (Ъ и Х): следующий и предыдущий ближайший враг.
- Delete: текст текущего задания, повторное нажатие: следующее задание.
- Shift+Delete: одобрение спутников (Origins).
- Backspace: панель быстрого доступа.
- Клавиши 1-0: умения игры, мод называет умение или его состояние.
- F1-F4: выбор персонажа отряда, мод называет его и здоровье.

## Журнал

Мод пишет журнал в папку bin_ship\scripts игры: DAOAccess.log для Origins, DA2Access.log для Dragon Age II. Если что-то не так, приложите этот файл к сообщению об ошибке.

## Устройство проекта и сборка

- core: общее ядро обеих игр.
  - core\src: речь, журнал, чтение меню, сканер, автоходьба, бой.
  - core\agent: Flash-агент, который работает внутри меню игры.
  - core\installer: установщик.
- games\dao и games\da2: то, что у каждой игры своё.
  - game.h: названия, папки и проверенные версии игры.
  - src: работа с Flash-движком меню своей версии, объекты мира и ходьба по адресам своего exe.
  - NOTEBOOK.md: рабочий журнал этой игры: что доказано, что нет, история тестов.
  - tools: скрипты на Python для изучения exe и файлов этой игры.
- NOTEBOOK.md в корне: общие решения и правила проекта.
- Для сборки нужны Visual Studio с инструментами C++ для x86, Java, Python 3 и FFDec 26.5.0 (https://github.com/jindrapetrik/jpexs-decompiler), распакованный в папку vendor\ffdec-26.5.0.
- build.ps1 собирает обе игры в папку build, по одной: build.ps1 -Game dao или -Game da2.
- deploy.ps1 собирает и копирует мод в игру. Пути к играм записаны в начале файла.
- make_release.ps1 -Version X собирает архивы обеих игр DAOAccess-X.zip и DA2Access-X.zip и установщик DragonAgeAccess-Setup.exe в папку release. Релиз на GitHub публикуется как обычный (не «пререлиз») с тегом vX: установщик находит последнюю версию по странице последнего релиза и скачивает архив по имени.

## Лицензия

MIT, см. LICENSE. Prism распространяется по лицензии MPL-2.0, Ultimate ASI Loader по лицензии MIT.

Dragon Age: Origins и Dragon Age II принадлежат Electronic Arts и BioWare. Этот проект с ними не связан.

---

# Dragon Age Access (English)

Mods that make Dragon Age: Origins and Dragon Age II playable for blind players: the game speaks through your screen reader.

## What it is

- Two mods with one shared core: DAO Access for Dragon Age: Origins and DA2 Access for Dragon Age II. Both games run on the same engine, so the scanner, walking, menu reading and speech are shared; only what depends on each game's exe is separate.
- The mod hooks into the game and speaks it through your screen reader with the Prism library. Tested with NVDA.
- The mod's own messages are in Russian for now; the game's texts are spoken in the game's language. Translations are welcome.
- These are early test versions. Please report problems in Issues and say which game.

## Features

Dragon Age: Origins (the prologue, Ostagar, the Korcari Wilds and Lothering have been played): menus, the whole character creation, conversations with their replies; inventory, character record, level-up, abilities, specialisations and the store with sections on Left and Right, item descriptions, buying and selling; gifts, approval, injuries, party selection at camp.

Dragon Age II (character creation and the prologue have been played): start screens, main menu, character creation (hero, appearance, background, difficulty), pause menu, saving, inventory, character record, tutorial popups, conversation lines.

Both games: an object scanner with distance and direction by category (characters, extras, loot, objects, exits, places, quests, party, enemies); walking to the selected object (movement keys cancel); the current quest text; combat with the nearest enemies, attack with Enter, enemy and party health, kill reports; the quickbar with ability names, cooldowns and sustained modes.

## Requirements

- Windows and a screen reader (NVDA recommended).
- Dragon Age: Origins - Ultimate Edition, proven on DAOrigins.exe 1.05.13263.0 (GOG).
- Dragon Age II 1.04 (DLC supported), proven on DragonAge2.exe 1.04.8524.0.
- Other builds (Steam, EA app) may not work; the installer warns about it. Please attach the mod's log to a report.

## Install

One installer for both games; it downloads the newest mod itself, so it needs the internet.

1. From Releases, download DragonAgeAccess-Setup.exe.
2. Run it. Windows may warn about an unknown publisher (the installer has no paid code signature): choose "More info", then "Run anyway", then allow the changes (the games often sit in Program Files, so the installer needs administrator rights).
3. It finds the games (GOG, Steam, EA app, plain installs) and asks which one: one game, both, or a game folder chosen by hand.
4. It asks its questions in standard dialogs with named buttons; Yes everywhere is fine. It downloads the newest mod for that game, copies it into the game and into the override folder in Documents and checks the files are there, gives the game access to 4 GB of memory (Large Address Aware; the original exe is backed up), frees Page Up and Page Down from the camera for the scanner (ZoomIn_1 / ZoomOut_1 in KeyBindings.ini; backed up, the mouse wheel still zooms; if the game was never started, the mod does it itself after the first start), and warns when your game version has not been tested.
5. Start the game as usual. You hear "Dragon Age Access загружен" (Origins) or "Dragon Age 2 Access загружен" (Dragon Age II) at start.

To update, run the same installer again: it compares the installed version with the newest and offers "Update to version ...". If it reports that mod files vanished right after installing, an antivirus removed them (the mod loader dinput8.dll is often taken for a threat by mistake): add the game folder to the antivirus exclusions and run the installer again.

Manual install: the DAOAccess-<version>.zip and DA2Access-<version>.zip archives hold bin_ship and override. Copy the contents of bin_ship into the game's bin_ship, and the .gfx file from override into Documents\BioWare\Dragon Age\packages\core\override (Origins) or Documents\BioWare\Dragon Age 2\packages\core\override (Dragon Age II).

## Uninstall

Run the installer again, pick the game and choose "Remove the mod": it removes the mod and offers to put back the original exe and camera keys. By hand: delete dinput8.dll, scripts\<Mod>.asi and the scripts\<Mod> folder from the game's bin_ship (<Mod> is DAOAccess or DA2Access), and the .gfx file from the override folder.

## Keys

Menus: Up and Down move, Enter presses, Tab and Shift+Tab jump between groups, Left and Right change values (pickers, sliders, attribute points) and switch sections on the inventory, record, level-up and store screens, Home and End first and last item of a section, Space item or ability description (in a conversation: repeat the other person's line), Shift+Enter on a backpack weapon equips it in the off hand. Dragon Age II hero choice: the "Выбор героя" item, Left and Right switch heroes, Enter picks.

World: Page Down / Page Up next and previous object, Ctrl+Page Down / Page Up category, Home repeat (in a fight: combat status), Shift+Home party health, injuries and distance, End walk to the object (movement keys cancel), Shift+End call lagging companions, Enter act on the object (talk, open, take, attack), ] and [ next and previous nearest enemy, Delete current quest text (again: next quest), Shift+Delete companion approval (Origins), Backspace quickbar, 1-0 the game's abilities (named by the mod), F1-F4 select a party member.

## Log

The mod writes bin_ship\scripts\DAOAccess.log (Origins) or bin_ship\scripts\DA2Access.log (Dragon Age II). Attach it to bug reports.

## Layout and building

core holds the shared code (core\src: speech, log, menu reader, scanner, walking, combat; core\agent: the Flash agent that runs inside the game's menus; core\installer: the installer). games\dao and games\da2 hold what differs per game: game.h (names, folders, proven builds), src (the GFx layer of that game's Scaleform version, world objects and walking at that exe's addresses), NOTEBOOK.md (that game's working log) and tools (Python scripts that study that game's exe and files). The root NOTEBOOK.md holds the shared decisions.

Building needs Visual Studio with the x86 C++ tools, Java, Python 3 and FFDec 26.5.0 extracted to vendor\ffdec-26.5.0. build.ps1 builds both games into build (or one: -Game dao / -Game da2), deploy.ps1 builds and copies into the games (paths at the top), make_release.ps1 -Version X makes DAOAccess-X.zip, DA2Access-X.zip and DragonAgeAccess-Setup.exe in release. Publish the GitHub release as a normal release (not a prerelease) tagged vX: the installer finds the newest version through the latest-release page and downloads the zip by name.

## Licence

MIT, see LICENSE. Prism is MPL-2.0, Ultimate ASI Loader is MIT. Dragon Age: Origins and Dragon Age II belong to Electronic Arts and BioWare; this project is not affiliated with them.
