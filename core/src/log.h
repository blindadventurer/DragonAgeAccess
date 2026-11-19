#pragma once
#include <string>
#include <vector>

// Mod log next to the .asi (DAOAccess.log), written by a background thread.
void LogInit(const std::wstring& logPath);
std::wstring LogPath();
void LogLine(const std::wstring& text);
// Several lines in one write (bulk listings).
void LogLines(const std::vector<std::wstring>& lines);
// Written at once with everything queued before it: for the crash path.
void LogLineNow(const std::wstring& text);
std::string Utf8(const std::wstring& w);
std::wstring Widen(const char* utf8);
