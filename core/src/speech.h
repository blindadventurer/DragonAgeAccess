#pragma once
#include <string>

// The only way the mod speaks. Starts the speech thread, which loads
// <modDir>\prism.dll by full path and owns the Prism backend.
void SpeechStart(const std::wstring& modDir);

// interrupt = true cuts what is being said; false queues after it.
void Speak(const std::wstring& text, bool interrupt);

// The same, but the log gets "<скрыто>" instead of the text: what the player types
// into input fields (an e-mail address, a name) never goes into the log file.
void SpeakPrivate(const std::wstring& text, bool interrupt);
