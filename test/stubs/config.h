#pragma once
#include <cstddef>

inline int passes_per_sec = 25;
inline size_t g_test_pulse = 0;
inline size_t thecore_pulse() noexcept { return g_test_pulse; }
inline void set_pulse(size_t p) noexcept { g_test_pulse = p; }
