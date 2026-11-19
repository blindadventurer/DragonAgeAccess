#pragma once
#include <string>

// The only way the mod speaks. Starts the speech thread, which loads
// <modDir>\prism.dll by full path and owns the Prism backend.
void SpeechStart(const std::wstring& modDir);

// interrupt = true cuts what is being said; false queues after it.
void Speak(const std::wstring& text, bool interrupt);
