#pragma once
#include <string>
#include <vector>

// World objects: the interface the shared scanner and menu reader use. Each game
// implements it with its own proven addresses in games\<game>\src\objects.cpp.
// Offsets quoted below are Origins' (DAOrigins.exe); Dragon Age II's are in its
// objects.cpp.

// One engine world object (see objects.cpp for the proven layout).
struct RawObject {
    unsigned id = 0;
    void* ptr = nullptr;       // valid only during the frame it was read
    std::string cls;           // RTTI class, e.g. "CCCreature"
    std::wstring name, tag;    // read from the object's catalogue attributes (HYPOTHESIS layout)
    unsigned char type = 0;    // 6 creature, 7 item, 10 placeable
    float x = 0, y = 0, z = 0;
    bool posOk = true;
    bool usable = false;       // placeable usable flag (+0x42C)
    int flag7f0 = 0;           // creature field +0x7F0 (non-zero: no cursor action; meaning unproven)
    bool dead = false;         // creature with health <= 0
    bool lootable = false;     // dead creature whose body can be searched
    bool talk = false;         // creature byte +0x860: has a conversation (cursor code
                               // 0x70CAD0 returns 3 "talk" for it when friendly)
    // CCObject byte +0x23C: the game's own "can be targeted" flag (vtable slot 0x134
    // sets it, 0x138 reads it; the Tab target filter 0x70BFA0 skips objects with 0).
    // HYPOTHESIS: 0 for creatures the designers keep hidden (camp emissaries).
    unsigned char sel23c = 1, sel23d = 1;
    // Placeables: the two actions of the current state (placeable_interactions.gda:
    // 1 Open, 2 Close, 3 Unlock, 5 Use, 6 Examine, 10 Dialog, 11 Loot, 15 Area Transition).
    unsigned short act0 = 0, act1 = 0;
    // The object is in play. False for objects switched off by the designers or a
    // plot: reserve waves, ambushes, people of a later scene, companions parked
    // outside the party, chargen fake heroes.
    //   Origins: CCObject byte +0x2C5 ("active"; constructor 0x699670 sets 1, setter
    //   0x698CD0, the script event dispatcher 0x6976B0 skips 0; matches Active=0/1 in
    //   lot100ar_lothering.are, proven 2026-11-19).
    //   Dragon Age II: object byte +0x2E7 bit 0x80 clear (the "dormant" flag; proven
    //   2026-11-19 21:32-21:35 by "scan woke:" lines exactly as the waves came).
    bool inPlay = true;
};

// Copies n bytes of the object's memory (raw dumps for finding flags); false on a fault.
bool ReadObjectBytes(unsigned id, unsigned char* buf, size_t n);

// Dragon Age II finds where objects keep their world x, y, z by searching object id
// for the catalogue position (x, y) once per session; PosOffset() is negative until
// then. Origins knows its layout: PosOffset() is never negative there.
bool CalibratePos(unsigned id, double x, double y);
int PosOffset();

// Checks the code bytes and installs the explore/combat mode capture thunks.
bool ObjectsInstall();

// Game thread only. Reads every object with an id up to maxId.
size_t EnumObjects(std::vector<RawObject>& out, unsigned maxId);

// One object by id (position, dead flag); false when the id is free.
bool ReadObject(unsigned id, RawObject& out);
std::string ObjectClass(void* obj);
std::wstring ObjectName(void* obj);
std::wstring ObjectTag(void* obj);

// Items the party received lately (Origins: a hook on the game's own item-added
// handler), oldest first, at most maxAgeMs old; taken out of the queue. Empty in a
// game without the hook.
struct ReceivedItem {
    std::wstring name;
    int count = 1;
};
std::vector<ReceivedItem> TakeReceivedItems(unsigned maxAgeMs);

// Captured mode instances (nullptr until the mode ran a virtual call).
void* ExploreMode();
void* CombatMode();

// Party selection screen (GameModePartyPicker): the candidates standing in the
// scene, chosen in the game by a mouse click on their model.
struct PartyCandidate {
    unsigned id = 0;        // creature object id
    int state = 0;          // 1 in the party, 2 free (other values: animating)
    bool locked = false;    // the story keeps this one in (or out)
    std::wstring name;      // memory name, may be empty
};
// False when the mode was never seen. count = members chosen now (max 4).
bool PartyCandidates(std::vector<PartyCandidate>& out, int& count);
// Adds a free candidate or removes a chosen one, as a click on the model does.
// Returns "added", "removed", "full", "locked", "busy", "missing" or "error".
std::string PartyToggle(unsigned id);

// The game's default action on an object (talk, use, open, pick up, attack),
// walking there first, as a click on it. Returns the action code the game chose
// (0 = none, 3 talk, 4 attack, 8 use), or a negative value on failure. inCombat
// picks the combat mode instance. Game thread only.
int InteractWith(unsigned id, bool inCombat);

// The ability the game waits to aim (its targeting state after a quickbar key, e.g.
// Shock); 0 when none or unknown in this game. Game thread only.
unsigned PendingAbility();
// Casts that pending ability on the object, as a click on it would (else at its
// place, for abilities aimed at the ground). Returns 1 cast on it, 3 cast at its
// place, 0 nothing pending, -3 not a valid target, -4 the game kept it waiting,
// -1 fault (logged); ability = the ability id.
int CastPendingOn(unsigned id, bool inCombat, unsigned* ability);
