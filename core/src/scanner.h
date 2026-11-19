#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// World scanner keys and speech. Call from the scanner HUD movie's Advance (game
// thread, agent ready); menuOpen silences it while a menu screen is active.
void ScannerTick(void* movie, DWORD now, bool menuOpen);

// F10 research dump: every engine object (class, type, position, usable flag)
// with the catalogue names tried for the first 150. Game thread, scanner HUD movie.
void ScannerDumpObjects(void* movie);
