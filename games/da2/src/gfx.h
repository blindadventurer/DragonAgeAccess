#pragma once
#include <string>
#include <initializer_list>

// Access to the game's Scaleform GFx 3.x movies (the Flash menus).
// Proven layout (static analysis of DragonAge2.exe 1.04.8524.0, see NOTEBOOK.md):
//   movie vtable 0xC2E514 (no RTTI; found through its error strings)
//   slot 22 GetVariable(GFxValue* out, const char* path)        ret 8
//   slot 28 Invoke(const char* method, GFxValue* result, const GFxValue* args, UInt n)  ret 0x10
//   slot 43 Advance(Float dt, UInt catchUp) -> Float            ret 8
// GFxValue (GFx 3.x): object interface pointer at +0, type at +4, value at +8.
// Types 0 undefined, 1 null, 2 bool, 3 number, 4 string, 5 wide string, 6 object,
// 7 array, 8 display object; 0x40 = managed (must be released through the object
// interface), 0x80 on input = convert to the low type. A managed string's value is
// a string node whose first field is the UTF-8 char*.

struct GFxValue {
    void* iface = nullptr;
    int type = 0;
    union {
        double n;
        bool b;
        const char* s;
        const wchar_t* ws;
        void* p;
    } v{};
};

enum GFxType { GFX_UNDEFINED = 0, GFX_NULL = 1, GFX_BOOL = 2, GFX_NUMBER = 3, GFX_STRING = 4, GFX_STRINGW = 5,
               GFX_OBJECT = 6, GFX_ARRAY = 7, GFX_DISPLAY = 8, GFX_MANAGED = 0x40, GFX_CONVERT = 0x80 };

// Checks the vtable against the proven build and replaces the Advance slot.
bool GfxInstall();

// Only valid while inside OnMovieAdvance for that movie (game thread, movie alive).
// Reads an ActionScript path; false when it is undefined, null or unresolved.
// Objects come back as their string form (a movie clip gives its target path).
bool GfxGet(void* movie, const char* path, std::wstring& out);
// Calls an ActionScript function by path with no arguments.
bool GfxInvoke(void* movie, const char* method);

// One argument for GfxCall: a string (UTF-8) or a number.
struct GfxArg {
    bool isNumber = false;
    std::string s;
    double n = 0;
    GfxArg(const std::string& v) : s(v) {}
    GfxArg(const char* v) : s(v) {}
    GfxArg(double v) : isNumber(true), n(v) {}
};
// Calls an ActionScript function with arguments; *result gets its return value as
// text (empty when undefined). False when the call failed.
bool GfxCall(void* movie, const char* method, std::initializer_list<GfxArg> args,
             std::wstring* result = nullptr);

// Implemented by the menu reader: runs on the game thread after each movie's Advance.
void OnMovieAdvance(void* movie);

// The movie's _root.Filename as last read by the menu reader ("?" if unknown).
const wchar_t* MovieFileName(void* movie);
