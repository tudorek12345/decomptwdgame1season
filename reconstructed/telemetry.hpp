#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
// Slots match re/replacements.json. Counters are diagnostic, not game state.
extern "C" volatile LONG twd_reconstruction_counters[14];
inline void twd_count(unsigned slot) noexcept { InterlockedIncrement(&twd_reconstruction_counters[slot]); }
