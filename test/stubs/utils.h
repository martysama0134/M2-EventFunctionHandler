#pragma once
#include <cstddef>
#include <cstdint>

#ifndef _MSC_VER
#ifndef __forceinline
#define __forceinline inline
#endif
#endif

#include "singleton.h"

using DWORD = std::uint32_t;

inline std::size_t g_test_global_time = 0;
inline std::size_t get_global_time() noexcept { return g_test_global_time; }
inline void set_global_time(std::size_t t) noexcept { g_test_global_time = t; }
