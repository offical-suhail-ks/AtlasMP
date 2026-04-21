/* cmake/msvc_compat.h
 * Force-included into every .cpp via /FI flag in CompilerFlags.cmake
 * This MUST be the very first thing compiled in every translation unit.
 * Fixes MSVC 19.50 (VS2026) where <string> needs strtol/strtod
 * already declared before it can be included.
 */
#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#  define _CRT_NONSTDC_NO_WARNINGS
#endif
#ifdef _MSC_VER
#  include <corecrt_math.h>   /* cosf/sinf/sqrtf in global namespace */
#endif
#include <stdlib.h>            /* strtol/strtod/strtof in global namespace */
#include <math.h>              /* C math functions in global namespace */
