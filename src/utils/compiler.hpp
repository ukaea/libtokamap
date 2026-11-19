#pragma once

#if defined(__GNUC__) || defined(__clang__)
#  define LIBTOKAMAP_UNREACHABLE __builtin_unreachable();
#elif defined(_MSC_VER)
#  define LIBTOKAMAP_UNREACHABLE __assume(0);
#else
#  define LIBTOKAMAP_UNREACHABLE
#endif

// C++20 or later. MSVC reports __cplusplus as 199711L unless built with
// /Zc:__cplusplus, so check _MSVC_LANG as well
#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#  define LIBTOKAMAP_HAS_CPP20 1
#else
#  define LIBTOKAMAP_HAS_CPP20 0
#endif
