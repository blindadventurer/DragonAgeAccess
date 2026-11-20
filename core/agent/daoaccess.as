// DAOAccess agent: ActionScript 2 helper loaded by the mod into each game menu movie
// (MovieClip.loadMovie into _root.__daoaccess). It lists the interactive widgets on
// screen and presses or adjusts them, so the C++ side never walks Flash objects.
// Shared by both games; tools/build_agent.py (FFDec) compiles it into each game's
// build/<game>/<agent file> (daoaccess.gfx, da2access.gfx).
//
// Widget kinds, from the game's own BioWare library (optionsmenu.gfx, startmenu2.gfx):
//   slider  : has SliderValue and UpArrow_mc/DownArrow_mc buttons, label Label_mc, value Value_mc
//   picker  : has PickerButton_mc (opens a list of PickerEntry buttons), label Label_mc
//   toggle  : Behaviour.ToggleButton (m_bToggleButtonEnabled), state SI_ItemSelected
//   tab     : Behaviour.StickyButton (m_bStickyButtonEnabled), state SI_ItemSelected
//   button  : any clip with SimulateButtonPush, or a plain onRelease handler

var A = new Object();
A.ver = 35;  // 35: codex names, long texts by paragraph, money alerts, pause state; 34: button labels not listed twice, DA2 ability trees; 33: DA2 attribute rows
_global.DAOAccess = A;

A.Clean = function(s) {
    if (s == undefined || s == null) return "";
    s = String(s);
    // split/join, not a character loop: building the string one character at a
    // time froze Dragon Age II for 45 s on its privacy policy (LoginViewTOS, a text
    // of tens of thousands of characters, log 2026-11-20 13:49).
    if (s.indexOf("\t") >= 0) s = s.split("\t").join(" ");
    if (s.indexOf("\r") >= 0) s = s.split("\r").join(" ");
    if (s.indexOf("\n") >= 0) s = s.split("\n").join(" ");
    // trim
    var a = 0;
    var b = s.length;
    while (a < b && s.charAt(a) == " ") a++;
    while (b > a && s.charAt(b - 1) == " ") b--;
    return (a == 0 && b == s.length) ? s : s.substring(a, b);
};

// Longest screen text handed to the mod (a privacy policy or licence is cut).
A.maxText = 3000;

// Text of a SharedLibrary.TextField clip or of a plain TextField.
// A button the agent can press (SimulateButtonPush or onRelease).
A.Pushable = function(b) {
    return b != undefined && (typeof(b.SimulateButtonPush) == "function" || typeof(b.onRelease) == "function");
};

A.TextOf = function(o) {
    if (o == undefined || o == null) return "";
    if (o.TextField != undefined && o.TextField.text != undefined) return A.Clean(o.TextField.text);
    if (o instanceof TextField) return A.Clean(o.text);
    return "";
};

// First visible non-empty text inside a clip, searched depth first.
A.FindText = function(mc, depth) {
    if (depth > 8) return "";
    var t = A.TextOf(mc.Label_mc);
    if (t != "") return t;
    t = A.TextOf(mc.ButtonText_MC);
    if (t != "") return t;
    if (mc.RollOver_mc != undefined) {
        t = A.TextOf(mc.RollOver_mc.ButtonText_MC);
        if (t != "") return t;
    }
    if (mc.Name != undefined && typeof(mc.Name) == "string" && mc.Name != "") return A.Clean(mc.Name);
    var k;
    var o;
    for (k in mc) {
        o = mc[k];
        if (o instanceof TextField) {
            if (o._visible != false) {
                t = A.Clean(o.text);
                if (t != "") return t;
            }
        } else if (typeof(o) == "movieclip" && o._parent == mc && o._visible) {
            t = A.FindText(o, depth + 1);
            if (t != "") return t;
        }
    }
    return "";
};

// Talk-table string (game language) through the movie's ExternalCommands class.
A.Tlk = function(id) {
    if (id == undefined || isNaN(Number(id)) || Number(id) <= 0) return "";
    if (_global.ExternalCommands == undefined) return "";
    return A.Clean(_global.ExternalCommands.GetTlkString(Number(id), ""));
};

// SharedLibrary.Ability: sNameStrRef holds the resolved name once
// LoadDeferredAbilityInfo ran ("?undefined" before).
A.AbilityName = function(ab) {
    var n = ab.sNameStrRef;
    if (typeof(n) != "string" || n == "" || n.charAt(0) == "?") {
        if (typeof(ab.LoadDeferredAbilityInfo) == "function") ab.LoadDeferredAbilityInfo();
        n = ab.sNameStrRef;
    }
    if (typeof(n) == "string" && n != "" && n.charAt(0) != "?") return A.Clean(n);
    if (ab.sName != undefined) return A.Clean(ab.sName);
    return "";
};

// "have" (learned), "avail" (can be learned), "locked" (requirements missing).
// The creature id comes from the open screen: CharGen or LevelUp.
A.AbilityState = function(ab) {
    // levelup.gfx carries the CharGen class too, with CreatureID 0: take the
    // first non-zero id (test 17:52: every level-up talent read "можно изучить").
    var id = undefined;
    if (_global.LevelUp != undefined && Number(_global.LevelUp.CreatureID) > 0) id = _global.LevelUp.CreatureID;
    else if (_global.CharGen != undefined && Number(_global.CharGen.CreatureID) > 0) id = _global.CharGen.CreatureID;
    if (id != undefined && _global.DAExternalCommands != undefined) {
        if (Number(_global.DAExternalCommands.HasAbility(id, ab.nID)) == 1) return "have";
        // The game tests this result for truth (ClassSpecializationScene), so
        // treat empty, "0" and "false" as missing requirements.
        var pr = _global.DAExternalCommands.HasAbilityPreRequisites(id, ab.nID);
        if (pr == undefined || pr == "" || pr == "0" || pr == "false" || pr == false) return "locked";
        return "avail";
    }
    if (ab.nDisplayState == 1) return "have";
    if (ab.nDisplayState == 3) return "locked";
    return "avail";
};

A.IsAttrRow = function(c) {
    return c.m_mcStatPlus != undefined && c.m_mcStatMinus != undefined && c.ListItemIndex != undefined &&
        _global.CharacterRecord != undefined && typeof(_global.CharacterRecord.ChangeAttribute) == "function";
};

// Value of an attribute row: "15, Урон 12, свободных очков 2" (the matching stat
// and, while points can be spent or taken back, the points left).
A.AttrValue = function(c) {
    var CR = _global.CharacterRecord;
    var v = String(c.m_nAttributeData);
    if (c.m_sMatchingStatName != undefined && c.m_sMatchingStatName != "")
        v += ", " + A.StripTags(c.m_sMatchingStatName) + " " + c.m_sMatchingStatData;
    if (CR.IsAttributeSpendingEnabled() || CR.HasCurrentCharSpentPoints()) {
        var left = (_global.CreatureUtils != undefined) ? _global.CreatureUtils.GetCharacterAttributePoints() : "?";
        v += ", свободных очков " + left;
    }
    return v;
};

A.Kind = function(c) {
    if (c._name.indexOf("DragArea") >= 0) return "";  // chargen model rotation area
    // Inventory, store and container items (SharedLibrary.Item): ObjectID, Name,
    // DoubleClickAction (equip / use / take). Equipment slots (EquipSlot).
    if (c.ObjectID != undefined && c.Name != undefined && typeof(c.DoubleClickAction) == "function") return "item";
    // Store rows (store.gfx StoreItemEntry): no DoubleClickAction; a double click
    // goes to onPressHitArea(true) -> Store.StartBuyTransaction / StartSellTransaction.
    if (c.ObjectID != undefined && c.Name != undefined && c.OwnedByStore != undefined &&
        typeof(c.onPressHitArea) == "function") return "item";
    // Specialization buttons (levelup.gfx ClassButton: sName, bSelected, bGrey,
    // nSpecializationAbilityID; chargen.gfx CharGenSpecButton: the same on oTarget).
    // Clicked only through onPressHitArea, so they were read as plain text before.
    if (typeof(c.onPressHitArea) == "function" &&
        (c.nSpecializationAbilityID != undefined || c.oTarget.nSpecializationAbilityID != undefined)) return "spec";
    // Companion approval bar (inventory.gfx, characterrecord.gfx SharedLibrary.ApprovalBar):
    // a picture with a tooltip only; read as a text line.
    if (typeof(c.UpdateApproval) == "function" && typeof(c.UpdateApprovalTooltip) == "function") return "approval";
    if (c.m_sSlotToken != undefined && typeof(c.GetContainedItemID) == "function") {
        // Only the slots of the current layout (inventory.gfx UpdateEquipmentLayout:
        // 0 default, 1 Shale slots 14-17, 2 dog slots 18-19); the others stay in the
        // movie even when hidden.
        var layout = Number(A.Val("GUI.SelectedChar.EquipmentLayout"));
        var sid = Number(c.m_nSlotID);
        var shale = sid >= 14 && sid <= 17;
        var dog = sid == 18 || sid == 19;
        if (layout == 1 ? !shale : (layout == 2 ? !dog : (shale || dog))) return "skip";
        return "slot";
    }
    if (c.SliderValue != undefined && c.UpArrow_mc != undefined) return "slider";
    // Quantity slider of the store's "how many to sell / buy" popup (PopupLayer
    // Slider_mc: ArrowDown_mc, ArrowUp_mc, SliderNumber_mc "1 / 2"); its arrows were
    // read as two unlabeled buttons (log 2026-11-18 17:35).
    if (c.SliderNumber_mc != undefined && A.Pushable(c.ArrowUp_mc) && A.Pushable(c.ArrowDown_mc)) return "spin";
    // Value with -/+ buttons (chargen AttributeAdjuster: Label_mc, PointsLabel_mc).
    if (typeof(c.Minus_mc.SimulateButtonPush) == "function" &&
        typeof(c.Plus_mc.SimulateButtonPush) == "function") return "spin";
    // Dragon Age II character record attribute row (characterrecord.gfx
    // AttributesEntry): its +/- buttons show only under the mouse and call
    // CharacterRecord.ChangeAttribute; Left / Right do the same through Adjust.
    if (A.IsAttrRow(c)) return "spin";
    // Dragon Age II ability tree (abilities.gfx AbilityClassContainer, name in
    // m_sClassName): a button while the trees are shown side by side
    // (AlmightyContainer.InChainSelection); once one is opened, its ability icons
    // are listed instead (it was one "без подписи" button hiding them, test 15:35).
    if (c.m_mcChainContainer != undefined && c.m_nGUIType != undefined && typeof(c.GetName) == "function") {
        if (typeof(c._parent.InChainSelection) == "function" && c._parent.InChainSelection() != true) return "";
        return "button";
    }
    if (c.PickerButton_mc != undefined) return "picker";
    // Codex (journal.gfx CodexEntry, CodexGroup): an entry not found yet (catalogue
    // <list>.N.Locked, the game greys it) cannot be read, so it is left out (player's
    // choice 2026-11-20; the group says how many are found). A group's
    // expand button is a "group", read with the group's name.
    if (A.IsCodexEntry(c)) {
        if (Number(A.Val(c.m_sEntryData + ".Locked")) != 0 || c.enabled == false) return "skip";
        return "radio";
    }
    if (A.IsCodexGroupButton(c)) return "group";
    if (typeof(c.SimulateButtonPush) == "function") {
        if (c.m_bToggleButtonEnabled != undefined) return "toggle";
        if (c.m_bStickyButtonEnabled != undefined) return (c._name.indexOf("Tab") >= 0) ? "tab" : "radio";
        return "button";
    }
    if (typeof(c.onRelease) == "function") return "button";
    return "";
};

A.IsCodexEntry = function(c) {
    return typeof(c.UpdateDescription) == "function" && c.m_sEntryData != undefined && c.m_sResRef != undefined;
};

A.IsCodexGroupButton = function(c) {
    var g = c._parent;
    return g.m_nCodexGroup != undefined && g.Button_mc == c && typeof(g.ShowChildren) == "function" &&
        g.m_sEntryData != undefined;
};

// The codex list scrolls inside a mask (ScrollingCodexList_mc.Scrollable_mc): its
// rows moved above the top of the screen were dropped and the whole list was read
// before the journal's tabs and Close (tester log 2026-11-20 18:33: stuck on the
// first row). Rows are placed as if the list were not scrolled. Returns the
// position, or undefined outside that list.
A.UnscrolledPos = function(c) {
    var s = c._parent;
    var d = 0;
    while (s != undefined && d < 8) {
        if (s._name == "Scrollable_mc" && s._parent._name == "ScrollingCodexList_mc") break;
        s = s._parent;
        d++;
    }
    if (s == undefined || d >= 8) return undefined;
    var p = new Object();
    p.x = 0;
    p.y = 0;
    c.localToGlobal(p);
    s.globalToLocal(p);
    s._parent.localToGlobal(p);
    return p;
};

// Label kept outside the button: a sibling named like the button with "Button"
// replaced by "Label" (newgame.gfx: GameButton0_mc + GameLabel0_mc).
A.SiblingLabel = function(c) {
    var n = c._name;
    var i = n.indexOf("Button");
    if (i < 0) return "";
    var other = c._parent[n.substr(0, i) + "Label" + n.substr(i + 6)];
    if (other == undefined) return "";
    if (other._visible == false) return "";
    var t = A.TextOf(other);
    if (t == "") t = A.FindText(other, 0);
    return t;
};

A.Line = function(c, kind) {
    var label = A.TextOf(c.Label_mc);
    if (label == "") label = A.FindText(c, 0);
    if (label == "") label = A.SiblingLabel(c);
    // Tooltip text of icon-only buttons (Behaviour.DisplayTooltip.TooltipSource).
    if (label == "" && typeof(c.TooltipSource) == "string") label = A.Clean(c.TooltipSource);
    if (label == "" && typeof(c.m_sClassName) == "string") label = A.StripTags(c.m_sClassName);  // DA2 ability tree
    // Character creation icons (CharGenButton): the choice's name on its target,
    // as text or as a talk-table string id resolved by the game's own lookup.
    if (label == "" && c.oTarget != undefined) {
        if (c.oTarget.sName != undefined) label = A.Clean(c.oTarget.sName);
        if (label == "" && c.oTarget.nNameStrRef != undefined) label = A.Tlk(c.oTarget.nNameStrRef);
    }
    if (label == "" && c.nNameStrRef != undefined) label = A.Tlk(c.nNameStrRef);
    var value = "";
    if (kind == "slider") value = A.TextOf(c.Value_mc);
    else if (kind == "spin") {
        if (c.SliderNumber_mc != undefined) {
            label = "Количество";
            value = A.TextOf(c.SliderNumber_mc).split(" / ").join(" из ");
        }
        if (A.IsAttrRow(c)) {
            if (c.m_sEntryName != undefined && c.m_sEntryName != "") label = A.StripTags(c.m_sEntryName);
            value = A.AttrValue(c);
        }
        if (value == "") value = A.TextOf(c.PointsLabel_mc);
        if (value == "") value = A.TextOf(c.Value_mc);
        if (label == "" && c.sName != undefined) label = A.Clean(c.sName);
    }
    else if (kind == "picker") {
        value = A.FindText(c.PickerButton_mc, 0);
        // DA2 SharedLibrary.Picker (popuplayer.gfx): PickedEntry of NumEntries,
        // changed by SelectPrevEntry / SelectNextEntry (its arrow buttons).
        if (Number(c.NumEntries) > 0 && c.PickedEntry != undefined)
            value += ", " + (Number(c.PickedEntry) + 1) + " из " + Number(c.NumEntries);
    }
    else if (kind == "toggle" || kind == "tab" || kind == "radio" || kind == "entry") value = (c.SI_ItemSelected == true) ? "1" : "0";
    if (kind == "item") {
        label = A.StripTags(c.Name);
        value = "";
        if (Number(c.StackSize) > 1) value += c.StackSize + " шт.";
        if (c.EquippedBy != undefined && c.EquippedBy != "") value += (value == "" ? "" : ", ") + "надето: " + A.StripTags(c.EquippedBy);
        // Gifts (base item 109, Item.as BITM_BASETYPEID_GIFT): given to a companion.
        if (Number(c.BaseItemID) == 109) value += (value == "" ? "" : ", ") + "подарок";
        // Feastday Gifts / Pranks (addins dao_prc_cp_3, dao_prc_cp_7 designeritems.erf):
        // the DLC replaces a given gift (val_im_gift_X, base 109) by a keepsake with the
        // same name: val_im_qck_X (usable from the quickbar) or a wearable item
        // (val_im_arm_..., val_im_wep_..., val_arm_...). Seen in the log 2026-11-18:
        // the given gift stays in the backpack without the gift mark.
        else if (c.TokenName != undefined) {
            var itag = String(A.Val(c.TokenName + ".Tag")).toLowerCase();
            if ((itag.indexOf("val_im_") == 0 || itag.indexOf("val_arm_") == 0) && itag.indexOf("val_im_gift_") != 0) {
                value += (value == "" ? "" : ", ") + "праздничный подарок, уже вручён";
                if (itag.indexOf("val_im_qck_") == 0 && Number(c.AbilityID) > 0 && Number(c.AbilityID) != 200000)
                    value += ", можно применить";
            }
        }
        // Store rows: the price the game shows (StoreItemEntry.GetCost, copper).
        if (c.OwnedByStore != undefined && typeof(c.GetCost) == "function") {
            value += (value == "" ? "" : ", ") + (c.OwnedByStore ? "цена " : "продать за ") + A.Money(c.GetCost());
            if (typeof(c.IsTooExpensive) == "function" && c.IsTooExpensive()) value += ", не хватает денег";
        }
        if (c.enabled == false || (typeof(c.IsEnabled) == "function" && !c.IsEnabled())) value += (value == "" ? "" : ", ") + "нельзя использовать";
    } else if (kind == "spec") {
        // have = chosen, avail = can be chosen now, closed = not unlocked yet (a
        // trainer or a manual opens it: DAExternalCommands.HasAchievement), locked =
        // no point or requirements missing.
        var o = (c.nSpecializationAbilityID != undefined) ? c : c.oTarget;
        if (o.sName != undefined && o.sName != "") label = A.StripTags(o.sName);
        if (o.bSelected == true) value = "have";
        else if (o.bGrey == true) {
            value = "locked";
            if (o.sAchievement != undefined && o.sAchievement != null && o.sAchievement != "" &&
                _global.DAExternalCommands != undefined &&
                !_global.DAExternalCommands.HasAchievement(o.sAchievement)) value = "closed";
        } else value = "avail";
    } else if (kind == "approval") {
        // The bar hides itself for the hero (UpdateApproval): visible = a companion.
        var ai = A.ApprovalOf(A.Val("GUI.SelectedChar.ID"));
        if (ai == "") return "";
        var pa = new Object();
        pa.x = 0;
        pa.y = 0;
        c.localToGlobal(pa);
        return targetPath(c) + "\ttext\tОдобрение: " + ai + "\t\t" + Math.round(pa.x) + "\t" + Math.round(pa.y) + "\t1\n";
    } else if (kind == "slot") {
        label = A.SlotName(c.m_nSlotID);
        value = (Number(c.m_nItemID) != 0) ? A.StripTags(A.Val(c.m_sSlotToken + ".Name")) : "";
    }
    // Codex: the entry's name (the game shows only its number, the name is the
    // tooltip), the group's name with how many entries are found and whether it is
    // open (read before as "1 / 69, флажок", tester log 2026-11-20 18:32).
    if (A.IsCodexEntry(c)) {
        var cn = A.StripTags(A.Val(c.m_sEntryData + ".Name"));
        if (cn != "") label = cn;
        if (Number(A.Val(c.m_sEntryData + ".Updated")) != 0) label += ", новое";
    } else if (kind == "group") {
        var g = c._parent;
        var gn = A.StripTags(A.Val(g.m_sEntryData + ".Name"));
        if (gn != "") label = gn;
        var total = Number(_global.ExternalCommands.GetNumMembers(g.m_sEntryData + ".CodexList"));
        var found = 0;
        var gi = 0;
        while (gi < total) {
            if (Number(A.Val(g.m_sEntryData + ".CodexList." + gi + ".Locked")) == 0) found++;
            gi++;
        }
        value = "найдено " + found + " из " + total + (g.m_bShowingChildren == true ? ", развёрнуто" : ", свёрнуто");
        if (Number(A.Val(g.m_sEntryData + ".Updated")) != 0) value += ", есть новое";
    }
    // Notifications (notifications.gfx Alert): the money alert writes the coins as
    // pictures ("Получено 1 <silver> 76 <copper>"), so only the bare numbers were
    // heard (player 2026-11-20). The amount in copper is <Token>.IntegerArg. The value
    // carries "type|token" for the mod's log (what an items alert holds is not known yet).
    if (c.Token != undefined && c.Type != undefined && typeof(c.StartFadingOut) == "function") {
        var at = Number(c.Type);
        if (at == 5) {
            var amount = Number(A.Val(c.Token + ".IntegerArg"));
            var word = label;
            var cut = 0;
            while (cut < word.length && (word.charAt(cut) < "0" || word.charAt(cut) > "9")) cut++;
            word = A.Clean(word.substr(0, cut));
            if (word == "") word = (amount < 0) ? "Потеряно" : "Получено";
            if (!isNaN(amount) && amount != 0) label = word + " " + A.Money(Math.abs(amount));
        }
        value = at + "|" + c.Token;
    }
    if (label == "" && typeof(c.sName) == "string") label = A.StripTags(c.sName);
    // Party portraits (DA2 characterrecord / inventory SharedLibrary.Portrait, token
    // Party.Members.N set by GUIPortraits.MakePortraitInitFunction): a picture only,
    // read "без подписи" (log 2026-11-19 21:41). Clicking selects that character.
    if (label == "" && typeof(c.m_sCreatureToken) == "string" && c.m_sCreatureToken != "") {
        label = A.StripTags(A.Val(c.m_sCreatureToken + ".Name"));
        var pid = A.Val(c.m_sCreatureToken + ".ID");
        if (pid != "" && pid == A.Val("GUI.SelectedChar.ID")) value = "выбран";
        if (Number(A.Val(c.m_sCreatureToken + ".PortraitLevelUp")) == 1)
            value += (value == "" ? "" : ", ") + "можно повысить уровень";
    }
    if (label == "") label = A.NameWord(c._name);
    // HTML in labels ("Переключить наборы оружия <b>(/)</b>", test 16:55).
    if (label.indexOf("<") >= 0) label = A.StripTags(label);
    // Skill, talent and spell icons (SharedLibrary.AbilityIcon in an AbilityChain):
    // the ability object hangs on the icon as m_oAbility.
    if (c.m_oAbility != undefined) {
        kind = "ability";
        label = A.AbilityName(c.m_oAbility);
        value = A.AbilityState(c.m_oAbility);
    }
    // World map pins (worldmap.gfx SharedLibrary.MapPinIcon): the name alone (the
    // tooltip text joined name and description), the pin's state and marks.
    // States 2 active, 3 grayed out, 4 unlockable, 5 completed, 6 destroyed; a
    // read-only map (WorldMap.m_bReadOnly) allows no travel at all.
    if (c.m_sPinToken != undefined) {
        var st = Number(A.Val(c.m_sPinToken + ".State"));
        var nm = A.StripTags(A.Val(c.m_sPinToken + ".Name"));
        if (nm != "") label = nm;
        var WM = _global.WorldMap;
        if (st == 2) value = (WM != undefined && WM.m_bReadOnly == true) ? "сейчас отсюда не отправиться" : "можно отправиться";
        else if (st == 3) value = "закрыто";
        else if (st == 4) value = "пока не открыто";
        else if (st == 5) value = "пройдено";
        else if (st == 6) value = "разрушено порождениями тьмы";
        if (WM != undefined && c.m_nIndex != undefined) {
            if (c.m_nIndex == WM.m_nCurrentLocIndex) value += (value == "" ? "" : ", ") + "вы здесь";
            if (c.m_nIndex == WM.m_nActiveQuestIndex) value += (value == "" ? "" : ", ") + "цель задания";
        }
    }
    var p = A.UnscrolledPos(c);
    if (p == undefined) {
        p = new Object();
        p.x = 0;
        p.y = 0;
        c.localToGlobal(p);
    }
    var en = (c.enabled == false || c.m_bStickyButtonEnabled == false ||
              c.m_bToggleButtonEnabled == false) ? "0" : "1";
    if (c.m_sPinToken != undefined) en = "1";  // the pin's state is in its value
    // DA2 accordion buttons (chargen_stage2 "НАЧАТЬ ИГРУ", appearance groups):
    // ElementContainer.InitializeElementContainer makes every button non-sticky
    // (m_bStickyButtonEnabled false) on purpose; the container says whether it
    // is enabled (ElementContainer.IsEnabled, Enable/Disable).
    if (A.IsAccordion(c)) en = c.m_mcElementContainer.IsEnabled() ? "1" : "0";
    // Rows of a SharedLibrary.OptimizedList (inventory, containers, store): only the
    // rows on screen exist, reused while scrolling; their place in the whole list
    // (ListItemIndex of m_nNumVisibleEntries, which holds the full count) goes along.
    var lp = "";
    if (c.List != undefined && c.ListItemIndex != undefined && c.List.m_nNumVisibleEntries != undefined)
        lp = c.ListItemIndex + "/" + c.List.m_nNumVisibleEntries;
    return targetPath(c) + "\t" + kind + "\t" + label + "\t" + value + "\t" +
        Math.round(p.x) + "\t" + Math.round(p.y) + "\t" + en + "\t" + lp + "\n";
};

// Scrolls the OptimizedList of the row at path so that entry number index is on
// screen (YPosition, whose listeners move the list and re-fill the reused rows:
// OptimizedList.OnScroll -> SynchronizeList), then returns that entry's path, or
// "edge" when index is outside the list, or "missing".
A.ListGo = function(path, index) {
    var c = eval(path);
    if (c == undefined || c.List == undefined) return "missing";
    var L = c.List;
    var n = Number(L.m_nNumVisibleEntries);
    var i = Number(index);
    if (isNaN(i) || i < 0 || i >= n) return "edge";
    var pitch = Number(L.ItemHeight) + 2;
    var view = L._parent.Viewport_mc._height;
    var y = Number(L.YPosition.GetValue());
    var ny = y;
    if (i * pitch < y) ny = i * pitch;
    else if ((i + 1) * pitch > y + view) ny = (i + 1) * pitch - view;
    if (ny != y) L.YPosition.SetWithConstraints(ny);
    var k = 0;
    var e;
    while (k < L.m_aVisibleEntries.length) {
        e = L.m_aVisibleEntries[k];
        if (e != undefined && e != null && Number(e.ListItemIndex) == i) return targetPath(e);
        k++;
    }
    return "missing";
};

// A text field outside any widget: plain screen text (titles, descriptions,
// dialog messages) or an input box. Empty, hidden or off-screen ones give "".
// The label clip of a button kept beside it (newgame.gfx GameLabel0_mc next to
// GameButton0_mc): the button already reads that text (SiblingLabel), so the label
// itself is not listed again. Listed twice, the player pressed Enter on the label
// and nothing happened (Origins new game, 2026-11-20 15:57).
A.IsButtonLabel = function(mc) {
    var n = mc._name;
    var i = n.indexOf("Label");
    if (i < 0) return false;
    var b = mc._parent[n.substr(0, i) + "Button" + n.substr(i + 5)];
    return b != undefined && b._visible != false && A.Kind(b) != "";
};

// A long text with line breaks (a codex entry, a quest description, a tutorial) is
// handed out one paragraph per line, so the arrows step through it paragraph by
// paragraph; read whole, the player could only hear it again from the start (codex,
// tester log 2026-11-20 18:33). Paragraph 1 keeps the field's own path, the others
// get "#2", "#3"... (never eval'ed: text lines are only read).
A.paraMin = 300;

A.Paragraphs = function(raw) {
    var s = String(raw);
    if (s.indexOf("\r") >= 0) s = s.split("\r").join("\n");
    var parts = s.split("\n");
    var out = new Array();
    var i = 0;
    var p;
    while (i < parts.length) {
        p = A.Clean(parts[i]);
        if (p != "") out.push(p);
        i++;
    }
    return out;
};

A.TextLine = function(mc, k, tf) {
    if (tf._visible == false || tf._alpha == 0) return "";
    if (tf.type != "input" && A.IsButtonLabel(mc)) return "";
    var t = A.Clean(tf.text);
    // The same TextField under two member names (codex description: TextField and
    // m_mcTextField) was listed and spoken twice (tester log 2026-11-20 18:32).
    if (tf.type != "input" && t != "") {
        var tkey = targetPath(mc) + "\t" + t;
        if (A.seenText[tkey] == true) return "";
        A.seenText[tkey] = true;
    }
    // Bare numbers next to an icon get the meaning of their clip (inventory stats).
    var lab = A.StatLabel(mc._name);
    if (lab != "" && t != "") t = lab + " " + t;
    var kind = (tf.type == "input") ? "edit" : "text";
    if (kind == "text" && lab == "" && t.length >= A.paraMin) {
        var paras = A.Paragraphs(tf.text);
        if (paras.length > 1) return A.ParaLines(mc, k, tf, paras);
    }
    if (kind == "text" && t.length > A.maxText)
        t = t.substr(0, A.maxText) + " ... текст обрезан, всего " + t.length + " знаков";
    // A password box (Dragon Age II EA sign-in) never hands its text out: only the
    // number of characters, as the value.
    var val = "";
    if (kind == "edit" && tf.password == true) {
        kind = "password";
        val = String(tf.text.length);
        t = "";
    }
    if (t == "" && kind == "text") return "";
    var path = targetPath(mc) + "." + k;
    if (A.seen[path] == true) return "";
    A.seen[path] = true;
    var p = new Object();
    p.x = tf._x;
    p.y = tf._y;
    mc.localToGlobal(p);
    if (p.x + tf._width / 2 <= 0 || p.y + tf._height / 2 <= 0) return "";
    return path + "\t" + kind + "\t" + t + "\t" + val + "\t" + Math.round(p.x) + "\t" + Math.round(p.y) + "\t1\n";
};

// One text line per paragraph, all at the field's position (the mod keeps their
// order: equal positions stay in the order given).
A.ParaLines = function(mc, k, tf, paras) {
    var path = targetPath(mc) + "." + k;
    if (A.seen[path] == true) return "";
    A.seen[path] = true;
    var p = new Object();
    p.x = tf._x;
    p.y = tf._y;
    mc.localToGlobal(p);
    if (p.x + tf._width / 2 <= 0 || p.y + tf._height / 2 <= 0) return "";
    var out = "";
    var i = 0;
    var t;
    while (i < paras.length) {
        t = paras[i];
        if (t.length > A.maxText) t = t.substr(0, A.maxText) + " ... абзац обрезан, всего " + t.length + " знаков";
        out += path + (i == 0 ? "" : "#" + (i + 1)) + "\ttext\t" + t + "\t\t" + Math.round(p.x) + "\t" +
            Math.round(p.y) + "\t1\n";
        i++;
    }
    return out;
};

// inList: inside an open picker list, where every button is a list entry.
A.Walk = function(mc, depth, skip, inList) {
    if (depth > 40) return "";
    var out = "";
    var k;
    var c;
    var kind;
    var path;
    var b;
    for (k in mc) {
        c = mc[k];
        if (c instanceof TextField) {
            if (c._parent == mc) out += A.TextLine(mc, k, c);
            continue;
        }
        if (typeof(c) != "movieclip") continue;
        if (c._parent != mc) continue;
        if (k == "__daoaccess" || k == skip) continue;
        if (k.indexOf("Scrollbar") >= 0) continue;  // scroll arrows: the text is read whole
        // DA2 picker arrows: Left / Right on the picker itself press them (Adjust);
        // listed alone they read "без подписи" (difficulty dialog, log 2026-11-19 21:36).
        if (k == "PickerArrowLeft_mc" || k == "PickerArrowRight_mc") continue;
        if (c._visible != true) continue;
        if (c._alpha <= 0) continue;  // faded out (inventory "full" overlay, test 16:55)
        if (c.MovieHidden == true) continue;
        // The same clip can sit under two member names (a group keeps its
        // selected child in a field): list it once.
        path = targetPath(c);
        if (A.seen[path] == true) continue;
        A.seen[path] = true;
        kind = A.Kind(c);
        if (kind == "skip") continue;
        if (kind != "") {
            // Off the left or top edge of the screen (slid-out panels).
            b = c.getBounds(_level0);
            if (((b.xMin + b.xMax) / 2 <= 0 || (b.yMin + b.yMax) / 2 <= 0) && A.UnscrolledPos(c) == undefined) {
                A.offscreen++;
                continue;
            }
            if (inList && (kind == "button" || kind == "tab" || kind == "radio")) kind = "entry";
            out += A.Line(c, kind);
            if (kind == "picker") out += A.Walk(c, depth + 1, "PickerButton_mc", true);
            continue;
        }
        out += A.Walk(c, depth + 1, "", inList);
    }
    return out;
};

// Returns one line per visible widget or text: path, kind, label, value, x, y, enabled.
A.Scan = function() {
    A.seen = new Object();
    A.seenText = new Object();
    A.offscreen = 0;
    return A.Walk(_level0, 0, "", false);
};

// Same lines as Scan, for one clip's subtree only (FloatyLayer: the pause sign,
// without the floating names of every creature in a big battle).
A.ScanUnder = function(path) {
    A.seen = new Object();
    A.seenText = new Object();
    A.offscreen = 0;
    var c = eval(path);
    if (c == undefined || c._visible != true || c._alpha <= 0) return "";
    return A.Walk(c, 1, "", false);
};

// The pause sign of FloatyLayer, from the game's own state: the sign follows
// Client.Paused (floatylayer.gfx PauseIndicator_mc observer), but its animation
// made the scan lose it for a moment, so "Пауза снята" / "Пауза" came every few
// seconds while paused (tester log 2026-11-20 18:46). Returns the sign's line when
// paused, "" when not, "?" when the state cannot be read (then the mod scans the sign).
A.PauseLine = function() {
    var v = _global.ExternalCommands.GetValue("Client.Paused");
    if (v == undefined || v == null || String(v) == "") return "?";
    if (!(v == true || String(v) == "1" || String(v) == "true")) return "";
    var t = A.FindText(_level0.PauseIndicator_mc, 0);
    if (t == "") t = "Пауза";
    return "_level0.PauseIndicator_mc\ttext\t" + t + "\t\t0\t0\t1\n";
};

// A button of a SharedLibrary.Accordion list (ElementButton: knows its container).
A.IsAccordion = function(c) {
    return c.m_mcElementContainer != undefined && typeof(c.m_mcElementContainer.IsEnabled) == "function" &&
        typeof(c.onReleaseHitArea) == "function";
};

A.Press = function(path) {
    var c = eval(path);
    if (c == undefined) return "missing";
    var kind = A.Kind(c);
    if (kind == "item") {
        // A gift in the backpack: its double click does nothing (PlayerItemList), the
        // game gives it through the radial menu or a drop on the paperdoll, both
        // sending "GameModeController.GiftItem <selected char> <item>" when the
        // selected character has an approval entry (a companion).
        if (Number(c.BaseItemID) == 109 && Number(A.Val("GUIInventory.ShowContainer")) == 0 &&
            String(c._parent.ListAttributeName).indexOf("Backpack") != -1) {
            var who = A.Val("GUI.SelectedChar.ID");
            if (A.ApprovalOf(who) == "") return "gift-hero";
            _global.ExternalCommands.SendMessage("GameModeController.GiftItem " + who + " " + c.ObjectID);
            return "gift\t" + A.StripTags(c.Name) + "\t" + A.StripTags(A.Val("GUI.SelectedChar.Name"));
        }
        if (typeof(c.DoubleClickAction) == "function") c.DoubleClickAction();
        else c.onPressHitArea(true);  // store row: buy or sell
        return "item";
    }
    if (kind == "spec") {
        c.onPressHitArea();
        return "spec";
    }
    if (A.Kind(c) == "slot") {
        if (Number(c.m_nItemID) == 0) return "empty";
        _global.ExternalCommands.SendMessage("GUI.SelectedChar.UnequipItem " + c.GetContainedItemID());
        return "unequip";
    }
    if (c.PickerButton_mc != undefined) {
        c.PickerButton_mc.SimulateButtonPush();
        return "picker";
    }
    // DA2 accordion button: a mouse click runs the release handler that
    // ListContainer.ReportElementContainerLoaded chains on: select the element
    // (open its group) or, for a plain entry such as "НАЧАТЬ ИГРУ", call its
    // OnSelected (CharGen_Stage2.FinalizeConfirmAppearance). SimulateButtonPush
    // does nothing there, the button not being sticky.
    if (A.IsAccordion(c)) {
        if (!c.m_mcElementContainer.IsEnabled()) return "disabled";
        c.onPressHitArea();
        c.onReleaseHitArea();
        return "accordion";
    }
    if (typeof(c.SimulateButtonPush) == "function") {
        c.SimulateButtonPush();
        return "push";
    }
    if (typeof(c.onPress) == "function") c.onPress();
    if (typeof(c.onRelease) == "function") {
        c.onRelease();
        return "release";
    }
    return "none";
};

// The game's own mouse-over: shows tooltips and fills description panels.
// enter = true enters the widget, false leaves it.
A.Hover = function(path, enter) {
    var c = eval(path);
    if (c == undefined) return "missing";
    if (c.Plus_mc != undefined) c = c.Plus_mc;  // spin: its +/- buttons carry the hover
    var f = enter ? c.onRollOverHitArea : c.onRollOutHitArea;
    if (typeof(f) != "function") f = enter ? c.onRollOver : c.onRollOut;
    if (typeof(f) != "function") return "none";
    f.call(c);
    return "ok";
};

// Keyboard focus into a text input (path of the TextField), all text selected so
// typing replaces it; mode "end" puts the caret after the text instead (the cursor
// arriving on a field). Unfocus() gives the focus back.
A.Focus = function(path, mode) {
    var tf = eval(path);
    if (tf == undefined) return "missing";
    Selection.setFocus(tf);
    if (mode == "end") Selection.setSelection(tf.text.length, tf.text.length);
    else Selection.setSelection(0, tf.text.length);
    return (Selection.getFocus() != null) ? "ok" : "nofocus";
};

A.Unfocus = function() {
    Selection.setFocus(null);
    return "ok";
};

// dir > 0 raises a slider, dir < 0 lowers it.
A.Adjust = function(path, dir) {
    var c = eval(path);
    if (c == undefined) return "missing";
    if (c.SliderValue != undefined) {
        if (dir > 0) c.UpArrow_mc.SimulateButtonPush();
        else c.DownArrow_mc.SimulateButtonPush();
        return A.TextOf(c.Value_mc);
    }
    if (A.IsAttrRow(c)) {
        // As the row's own +/- buttons (AttributesEntry.StatPlus_mc / StatMinus_mc),
        // with the checks that decide whether the game shows them.
        var CR = _global.CharacterRecord;
        var i = Number(c.ListItemIndex) - 1;
        if (dir > 0) {
            if (!CR.IsAttributeSpendingEnabled()) return "nopoints";
            CR.ChangeAttribute(i, CR.POINTS_INCREMENT);
        } else {
            if (!CR.AttributeHasPointsSpent(i)) return "nospent";
            CR.ChangeAttribute(i, CR.POINTS_DECREMENT);
        }
        return "ok " + A.AttrValue(c);
    }
    if (c.Plus_mc != undefined) {
        // Spin: the game's own +/- press. The result names the button state, so a
        // refused press shows why in the log.
        var b = (dir > 0) ? c.Plus_mc : c.Minus_mc;
        b.SimulateButtonPush();
        return "pressed " + ((dir > 0) ? "plus" : "minus") + " enabled=" + b.enabled +
            " pbe=" + b.m_bPushButtonEnabled + " down=" + b.m_ButtonDown;
    }
    if (c.PickerButton_mc != undefined) {
        // DA2 picker: previous / next entry, as its arrow buttons do.
        if (typeof(c.SelectNextEntry) != "function") return "none";
        var before = Number(c.PickedEntry);
        if (dir > 0) c.SelectNextEntry();
        else c.SelectPrevEntry();
        if (Number(c.PickedEntry) == before) return "edge";
        return A.FindText(c.PickerButton_mc, 0);
    }
    if (c.SliderNumber_mc != undefined) {  // quantity slider (store popup)
        var a = (dir > 0) ? c.ArrowUp_mc : c.ArrowDown_mc;
        if (typeof(a.SimulateButtonPush) == "function") a.SimulateButtonPush();
        else {
            if (typeof(a.onPress) == "function") a.onPress();
            if (typeof(a.onRelease) == "function") a.onRelease();
        }
        return A.TextOf(c.SliderNumber_mc);
    }
    return "none";
};

// ---------------------------------------------------------------- data explorer
// The game's C++ data catalogue ("SmartData"), reached through the movie's own
// ExternalCommands: GetValue, GetNumMembers, ListMemberNames (fscommands answered
// synchronously in _root.fsc_retval). Explore walks it breadth first and returns
// one line per node: path, value, member count, raw member list.

A.SplitNames = function(raw) {
    if (raw == undefined || raw == null) return new Array();
    var s = String(raw);
    if (s == "") return new Array();
    var sep = " ";
    if (s.indexOf(",") >= 0) sep = ",";
    else if (s.indexOf(";") >= 0) sep = ";";
    else if (s.indexOf("\n") >= 0) sep = "\n";
    var parts = s.split(sep);
    var out = new Array();
    var i = 0;
    var p;
    while (i < parts.length) {
        p = A.Clean(parts[i]);
        if (p != "") out.push(p);
        i++;
    }
    return out;
};

A.Explore = function(root, maxDepth, maxNodes) {
    var EC = _global.ExternalCommands;
    if (EC == undefined) return "no ExternalCommands\n";
    var out = "";
    var queue = new Array();
    queue.push({path: root, depth: 0});
    var count = 0;
    var node;
    var path;
    var v;
    var n;
    var raw;
    var names;
    var i;
    var child;
    var limit;
    while (queue.length > 0 && count < maxNodes) {
        node = queue.shift();
        path = node.path;
        v = EC.GetValue(path);
        n = EC.GetNumMembers(path);
        raw = EC.ListMemberNames(path);
        count++;
        out += path + "\t" + A.Clean(v).substr(0, 120) + "\t" + n + "\t" + A.Clean(raw).substr(0, 400) + "\n";
        if (node.depth >= maxDepth) continue;
        names = A.SplitNames(raw);
        if (names.length == 0 && Number(n) > 0) {
            // A list without names: members are indices; look at the first two.
            limit = Math.min(Number(n), 2);
            i = 0;
            while (i < limit) { names.push(String(i)); i++; }
        } else if (names.length > 3 && !isNaN(Number(names[0])) && !isNaN(Number(names[1]))) {
            names = names.slice(0, 2);
        }
        i = 0;
        while (i < names.length) {
            child = (path == "") ? names[i] : path + "." + names[i];
            queue.push({path: child, depth: node.depth + 1});
            i++;
        }
    }
    return out + "explored " + count + " nodes, " + queue.length + " left\n";
};

// The minimap's creature batch, filled by the game each frame:
// ids, then x y orientation per creature, then flag bits.
A.Batch = function() {
    var M = _global.SharedLibrary.SharedMap;
    if (M == undefined) return "no SharedMap\n";
    return "ids=" + M.BatchCreatureIDs.join(",") + "\npos=" + M.BatchCreaturePositionsOrientations.join(",") +
        "\nflags=" + M.BatchCreatureFlags.join(",") + "\n";
};

// ---------------------------------------------------------------- world scanner
// Sources found in the F10 dump of 2026-11-17 (catalogue paths read by map.gfx):
// map notes, quest destinations, quest givers, party members. Positions are
// world X/Y in metres (HYPOTHESIS from running speed). One line per entry:
// kind, id, name, x, y, extra. The first line is the player: id, name, x, y,
// camera orientation (degrees; forward = (-sin, cos)).

A.StripTags = function(s) {
    s = A.Clean(s);
    if (s.indexOf("<") < 0) return s;
    // Text before the first tag, then after each tag's ">" (no character loop: see Clean).
    var parts = s.split("<");
    var out = parts[0];
    var i = 1;
    var gt;
    while (i < parts.length) {
        gt = parts[i].indexOf(">");
        if (gt >= 0) out += parts[i].substr(gt + 1);
        i++;
    }
    return A.Clean(out);
};

A.Val = function(path) {
    var v = _global.ExternalCommands.GetValue(path);
    if (v == undefined || v == null) return "";
    return String(v);
};

A.WorldLine = function(kind, base, name, extra) {
    return kind + "\t" + A.Val(base + ".ID") + "\t" + A.StripTags(name) + "\t" +
        A.Val(base + ".WorldPositionX") + "\t" + A.Val(base + ".WorldPositionY") + "\t" + extra + "\n";
};

// The character the player controls: GUI.SelectedChar (F1..F4), else the leader.
// Party.Leader stays the hero when the dog is selected (log, test 20:22).
A.Me = function() {
    var id = A.Val("GUI.SelectedChar.ID");
    if (id == "" || id == "0") return "Party.Leader";
    return "GUI.SelectedChar";
};

A.World = function() {
    var EC = _global.ExternalCommands;
    if (EC == undefined) return "";
    var me = A.Me();
    var out = "player\t" + A.Val(me + ".ID") + "\t" + A.StripTags(A.Val(me + ".Name")) + "\t" +
        A.Val(me + ".WorldPositionX") + "\t" + A.Val(me + ".WorldPositionY") + "\t" +
        A.Val("GameMode.CameraOrientationY") + "\n";
    var n;
    var i;
    var p;
    var name;
    n = Number(EC.GetNumMembers("MapNoteList.MapNotes"));
    i = 0;
    while (i < n) {
        p = "MapNoteList.MapNotes." + i;
        name = A.Val(p + ".MapNote");
        if (A.StripTags(name) == "") name = A.Val(p + ".Name");
        out += A.WorldLine("note", p, name, A.Val(p + ".MapNoteVisible") + "|" + A.Val(p + ".Tag") + "|" +
            A.Val(p + ".Icon") + "|" + A.StripTags(A.Val(p + ".Name")));
        i++;
    }
    n = Number(EC.GetNumMembers("Journal.PlotGivers"));
    i = 0;
    while (i < n) {
        p = "Journal.PlotGivers." + i;
        out += A.WorldLine("giver", p, A.Val(p + ".Name"), A.Val(p + ".MapNoteVisible"));
        i++;
    }
    n = Number(EC.GetNumMembers("Journal.PlotDestinations"));
    i = 0;
    while (i < n) {
        p = "Journal.PlotDestinations." + i;
        out += A.WorldLine("quest", p + ".Object", A.Val(p + ".Object.Name"),
            A.Val(p + ".ActiveQuest") + "|" + A.StripTags(A.Val(p + ".Tooltip")));
        i++;
    }
    n = Number(EC.GetNumMembers("Party.Members"));
    i = 0;
    while (i < n) {
        p = "Party.Members." + i;
        if (A.Val(p + ".ID") != "" && A.Val(p + ".ID") != A.Val(me + ".ID"))
            out += A.WorldLine("party", p, A.Val(p + ".Name"), "");
        i++;
    }
    return out;
};

// Journal quests, the way journal.gfx reads them (QuestList.as, QuestEntry.as):
// Journal.PlotList.N.{ResRef, ParentPlot, Name, Completed, Updated, Description}.
// First line: "active" and Journal.ActivePlot.ResRef. Then one line per plot:
// resref, parent, name, completed, updated, description (a bold heading <emp>..</emp>
// becomes a sentence of its own).
A.Quests = function() {
    var EC = _global.ExternalCommands;
    if (EC == undefined) return "";
    var out = "active\t" + A.Val("Journal.ActivePlot.ResRef") + "\n";
    var n = Number(EC.GetNumMembers("Journal.PlotList"));
    var i = 0;
    var p;
    var d;
    while (i < n) {
        p = "Journal.PlotList." + i;
        d = A.Val(p + ".Description").split("</emp>").join(". ");
        out += A.Val(p + ".ResRef") + "\t" + A.Val(p + ".ParentPlot") + "\t" + A.StripTags(A.Val(p + ".Name")) +
            "\t" + A.Val(p + ".Completed") + "\t" + A.Val(p + ".Updated") + "\t" + A.StripTags(d) + "\n";
        i++;
    }
    return out;
};

// Combat state in one call: game mode (GameModeController.CurrentGameMode, 1 =
// combat, GUISystem.GM_COMBAT), the leader's health (Attributes.7 Current/Total,
// as portraits.gfx and inventory.gfx read it), the leader's CombatTarget, and the
// health of object id (HYPOTHESIS: Objects.<id>.Attributes.7 resolves like
// Party.Leader.Attributes.7). Tab separated.
A.Combat = function(id) {
    var t = "Objects." + id;
    var me = A.Me();  // the controlled character
    // DA2: me.CombatTarget read empty all session (test 19:55, never a "combat
    // target from game" line); the exe has the catalogue name "CombatTarget", so
    // try its .ID too (HYPOTHESIS: it is an object node).
    var ct = A.Val(me + ".CombatTarget.ID");
    if (ct == "" || ct == "0") ct = A.Val(me + ".CombatTarget");
    return A.Val("GameModeController.CurrentGameMode") + "\t" + A.Val(me + ".Attributes.7.Current") + "\t" +
        A.Val(me + ".Attributes.7.Total") + "\t" + ct + "\t" +
        A.Val(t + ".Attributes.7.Current") + "\t" + A.Val(t + ".Attributes.7.Total");
};

// Party selection (F1..F4, Shift+F1..F4): first line the controlled character
// (GUI.SelectedChar.ID, Name), Party.SelectedCount and Party.Leader.ID; then one
// line per Party.Members.N: id, name, Party.MemberSelected.N, health current,
// health total (Attributes.7).
// Injuries of a creature token (portraits.gfx CoreStats / EffectIcon): effects in
// <token>.EffectList.N with AbilityID >= 500000 (EffectIcon.INJURY_ABILITY_ID);
// the row of the injuries table (2DA 160, _2DA.TABLE_INJURIES) is AbilityID -
// 500000, its NameStrRef column (hash 846460685) names it, Label (3030163814) is
// the designer's word. Names joined by "; ".
A.Injuries = function(p) {
    var EC = _global.ExternalCommands;
    if (EC == undefined) return "";
    var n = Number(EC.GetNumMembers(p + ".EffectList"));
    var out = "";
    var i = 0;
    var id;
    var row;
    var nm;
    while (i < n) {
        id = Number(A.Val(p + ".EffectList." + i + ".AbilityID"));
        if (id >= 500000) {
            row = id - 500000;
            nm = "";
            if (typeof(EC.Get2DAValue) == "function") {
                nm = A.Tlk(EC.Get2DAValue(160, row, 846460685));
                if (nm == "") nm = A.Clean(String(EC.Get2DAValue(160, row, 3030163814)));
            }
            if (nm == "" || nm == "undefined") nm = "травма " + row;
            if (out.indexOf(nm) < 0) out += (out == "" ? "" : "; ") + nm;
        }
        i++;
    }
    return out;
};

A.Party = function() {
    var EC = _global.ExternalCommands;
    if (EC == undefined) return "";
    var out = A.Val("GUI.SelectedChar.ID") + "\t" + A.StripTags(A.Val("GUI.SelectedChar.Name")) + "\t" +
        A.Val("Party.SelectedCount") + "\t" + A.Val("Party.Leader.ID") + "\t" + A.Val("Party.HoldPositions") + "\n";
    var n = Number(EC.GetNumMembers("Party.Members"));
    var i = 0;
    var p;
    while (i < n) {
        p = "Party.Members." + i;
        out += A.Val(p + ".ID") + "\t" + A.StripTags(A.Val(p + ".Name")) + "\t" + A.Val("Party.MemberSelected." + i) +
            "\t" + A.Val(p + ".Attributes.7.Current") + "\t" + A.Val(p + ".Attributes.7.Total") + "\t" +
            A.Injuries(p) + "\n";
        i++;
    }
    return out;
};

// Quickbar slots 0..9 (keys 1..0) of the selected character, as quickbar.gfx
// reads them (QuickBarIconSlot.SetQuickSlotIndex: <player>.Stats.QuickSlots.N is
// the ability instance; AbilityIcon / PieCooldown / QuickBarAbilityIcon fields).
// QuickState: the fast-changing part, one line per slot: slot value (0 = empty),
// CooldownPercentage (0 ready .. 1 just used), Status ("1" = sustained on),
// PendingActivation (waiting for a target).
// DA2 (quickbar.gfx of DA2, QuickBarIconSlot.ShowIcon / AbilityIcon): the slot
// node itself reads empty (test 19:55: every slot ""), a filled slot has
// .AbilityTemplate.ID, and the cooldown is .Cooldown.CooldownPercentage.
A.QuickState = function() {
    var out = "";
    var i = 0;
    var t;
    var v;
    var cd;
    while (i < 10) {
        t = "GUI.SelectedChar.Stats.QuickSlots." + i;
        v = A.Val(t + ".AbilityTemplate.ID");
        if (v == "" || v == "0") v = A.Val(t);
        cd = A.Val(t + ".Cooldown.CooldownPercentage");
        if (cd == "") cd = A.Val(t + ".CooldownPercentage");
        out += v + "\t" + cd + "\t" + A.Val(t + ".Status") + "\t" +
            A.Val(t + ".PendingActivation") + "\n";
        i++;
    }
    return out;
};

// QuickNames: one line per slot: name (talk table NameStrRef, else template
// Name, else the source item's name), source item stack size, IsError, IsWarning,
// AbilityType.
A.QuickNames = function() {
    var out = "";
    var i = 0;
    var t;
    var n;
    while (i < 10) {
        t = "GUI.SelectedChar.Stats.QuickSlots." + i;
        n = "";
        if (Number(A.Val(t + ".AbilityTemplate.ID")) != 0 || Number(A.Val(t)) != 0) {
            n = A.Tlk(A.Val(t + ".AbilityTemplate.NameStrRef"));
            if (n == "") n = A.StripTags(A.Val(t + ".AbilityTemplate.Name"));
            if (A.Val(t + ".SourceItemName") != "") n = A.StripTags(A.Val(t + ".SourceItemName"));
        }
        out += n + "\t" + A.Val(t + ".SourceItem.StackSize") + "\t" + A.Val(t + ".IsError") + "\t" +
            A.Val(t + ".IsWarning") + "\t" + A.Val(t + ".AbilityTemplate.AbilityType") + "\n";
        i++;
    }
    return out;
};

// Sends one message string to the game, the way the menus do
// (ExternalCommands.SendMessage -> fscommand "SendMessage"). Used for the
// click-to-move experiment.
A.Send = function(msg) {
    _global.ExternalCommands.SendMessage(msg);
    return "sent";
};

// Names of engine objects by id, from the catalogue's Objects node (HYPOTHESIS:
// "Objects.<id>.<field>" resolves an object the way Party.Leader does). ids is a
// comma separated list; the answer is one line per id: name, tag, hostile, dead.
A.Names = function(ids) {
    var list = String(ids).split(",");
    var out = "";
    var i = 0;
    var p;
    while (i < list.length) {
        p = "Objects." + list[i];
        out += A.StripTags(A.Val(p + ".Name")) + "\t" + A.Val(p + ".Tag") + "\t" + A.Val(p + ".IsHostile") +
            "\t" + A.Val(p + ".IsDead") + "\t" + A.Val(p + ".Attributes.7.Total") + "\n";
        i++;
    }
    return out;
};

// Russian names of the inventory equipment slots (inventory.gfx SetEquipSlotID).
A.SlotName = function(id) {
    var n = Number(id);
    if (n == 0) return "Оружие, основная рука";
    if (n == 1) return "Вторая рука";
    if (n == 2) return "Боеприпасы";
    if (n == 4) return "Доспех";
    if (n == 5) return "Шлем";
    if (n == 6) return "Сапоги";
    if (n == 7) return "Перчатки";
    if (n == 9) return "Кольцо 1";
    if (n == 10) return "Кольцо 2";
    if (n == 11) return "Амулет";
    if (n == 12) return "Пояс";
    if (n == 18) return "Боевая раскраска";
    if (n == 19) return "Ошейник";
    return "Ячейка " + n;
};

// Longer description of a widget: an item's own tooltip (SetupItemTooltip fills
// TooltipSource), or any tooltip text the widget carries.
A.Describe = function(path) {
    var c = eval(path);
    if (c == undefined) return "";
    if (A.Kind(c) == "slot" && Number(c.m_nItemID) != 0) {
        return A.StripTags(A.Val(c.m_sSlotToken + ".Name")) + ". " + A.StripTags(A.Val(c.m_sSlotToken + ".Description"));
    }
    if (A.Kind(c) == "spec") {
        if (typeof(c.sDescription) == "string") return A.StripTags(c.sDescription);
        if (c.oTarget != undefined) return A.Tlk(c.oTarget.nDescriptionStrRef);
    }
    if (typeof(c.SetupItemTooltip) == "function") c.SetupItemTooltip();
    var t = "";
    if (typeof(c.TooltipSource) == "string") t = c.TooltipSource;
    if (t == "" && typeof(c.Description) == "string") t = c.Description;
    return A.StripTags(t);
};

// Puts an item (its ability: potions, poultices, grenades) or an ability icon on
// quickbar slot `slot` (0-based), with the message the quickbar itself sends when
// something is dropped on it (quickbar.gfx QuickBarIconSlot.SetQuickslot):
// "GUI.SelectedChar.SetQuickslot <charId> <slot> <abilityId> <itemTag> <refTemplate>".
A.Quickslot = function(path, slot) {
    var c = eval(path);
    if (c == undefined) return "missing";
    var ability = undefined;
    var tag = "";
    var name = "";
    if (A.Kind(c) == "item") {
        ability = Number(c.AbilityID);
        tag = (c.Tag != undefined) ? String(c.Tag) : "";
        name = A.StripTags(c.Name);
        if (isNaN(ability) || ability == 200000 || ability == 0) return "noability";  // ITEM_ABILITY_INVALID
    } else if (c.m_oAbility != undefined) {
        ability = Number(c.m_oAbility.nID);
        name = A.AbilityName(c.m_oAbility);
    } else {
        return "noability";
    }
    var id = A.Val("GUI.SelectedChar.ID");
    _global.ExternalCommands.SendMessage("GUI.SelectedChar.SetQuickslot " + id + " " + slot + " " + ability + " " + tag + " ");
    return "ok\t" + name;
};

// Equip a backpack item into its second slot (off hand, second ring): the same
// message the game's drag onto a paperdoll slot and its radial "equip alt" send
// (inventory.gfx EquipSlot.EquipItem, RadialMenu RADIAL_BUTTON_EQUIP_ALT).
// Returns "ok\t<slot>\t<name>", or why not: missing, notitem, container,
// notbackpack, locked, heavy (one-hand weapon kept to the main hand without the
// dual-wield master talent, Item.as), one (a single slot), none.
A.EquipAlt = function(path) {
    var c = eval(path);
    if (c == undefined) return "missing";
    if (A.Kind(c) != "item") return "notitem";
    if (Number(A.Val("GUIInventory.ShowContainer")) != 0) return "container";
    var list = String(c._parent.ListAttributeName);
    if (list.indexOf("Backpack") == -1) return "notbackpack";
    if (Number(A.Val("GUI.SelectedChar.CanChangeEquipment")) == 0) return "locked";
    var slots = (typeof(c.GetEquipSlotIndices) == "function") ? c.GetEquipSlotIndices() : [];
    if (slots.length == 0) return "none";
    if (slots.length < 2) {
        if (Number(c.EquippableSlots) == 1 && Number(A.Val(c.TokenName + ".EquippableSlots")) == 3) return "heavy";
        return "one";
    }
    _global.ExternalCommands.SendMessage("GUI.SelectedChar.EquipItem " + c.ObjectID + " " + slots[1]);
    return "ok\t" + slots[1] + "\t" + A.StripTags(c.Name);
};

// Approval of one companion, as the game's ApprovalBar tooltip gives it
// (CreatureUtils.GetApprovalInfo: Party.Approval.N.{ObjectID, Approval, Desc}):
// "+12, Тёплое". "" when the creature has no approval entry (the hero).
A.ApprovalOf = function(id) {
    var EC = _global.ExternalCommands;
    if (EC == undefined || id == "" || id == "0") return "";
    var n = Number(EC.GetNumMembers("Party.Approval"));
    var i = 0;
    var p;
    var r;
    var d;
    while (i < n) {
        p = "Party.Approval." + i;
        if (Number(A.Val(p + ".ObjectID")) == Number(id)) {
            r = Number(A.Val(p + ".Approval"));
            d = A.StripTags(A.Val(p + ".Desc"));
            return (r > 0 ? "+" : "") + r + (d != "" ? ", " + d : "");
        }
        i++;
    }
    return "";
};

// Every approval entry: one line per companion: id, name, rating, description.
A.Approval = function() {
    var EC = _global.ExternalCommands;
    if (EC == undefined) return "";
    var n = Number(EC.GetNumMembers("Party.Approval"));
    var out = "";
    var i = 0;
    var p;
    var id;
    while (i < n) {
        p = "Party.Approval." + i;
        id = A.Val(p + ".ObjectID");
        out += id + "\t" + A.StripTags(A.Val("Objects." + id + ".Name")) + "\t" + A.Val(p + ".Approval") + "\t" +
            A.StripTags(A.Val(p + ".Desc")) + "\n";
        i++;
    }
    return out;
};

// Copper to "2 зол. 5 сер. 30 мед." (100 copper = 1 silver, 100 silver = 1 gold).
A.Money = function(v) {
    var c = Math.floor(Number(v));
    if (isNaN(c) || c <= 0) return "0 мед.";
    var g = Math.floor(c / 10000);
    var s = Math.floor((c % 10000) / 100);
    var m = c % 100;
    var out = "";
    if (g > 0) out += g + " зол.";
    if (s > 0) out += (out == "" ? "" : " ") + s + " сер.";
    if (m > 0) out += (out == "" ? "" : " ") + m + " мед.";
    return out;
};

// Labels for value fields that show only a number beside an icon (inventory.gfx Stats_mc).
A.StatLabel = function(n) {
    if (n == "HealthText_mc") return "Здоровье";
    if (n == "EnergyText_mc") return (Number(A.Val("GUI.SelectedChar.Stats.ClassHasMana")) == 1) ? "Мана" : "Выносливость";
    if (n == "FatigueText_mc") return "Усталость";
    if (n == "ArmorText_mc") return "Броня";
    if (n == "DefenseText_mc") return "Защита";
    if (n == "AttackText_mc") return "Атака";
    if (n == "DamageText_mc") return "Урон";
    if (n == "CapacityNumber_mc") return "Рюкзак";
    return "";
};

// Words for unlabeled buttons, from their instance names.
A.NameWord = function(n) {
    if (n == undefined) return "";
    if (n.indexOf("Previous") >= 0 || n.indexOf("Prev") == 0) return "Предыдущий";
    if (n.indexOf("Next") >= 0) return "Следующий";
    if (n.indexOf("Close") >= 0) return "Закрыть";
    if (n.indexOf("WeaponSet") >= 0) return "Сменить набор оружия";
    if (n.indexOf("TakeAll") >= 0) return "Взять все";
    if (n.indexOf("Sort") >= 0) return "Сортировка";
    return "";
};
