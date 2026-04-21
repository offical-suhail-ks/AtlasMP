// cmake/atlas_global_fix.h
// This file is force-included into EVERY .cpp via /FI compiler flag.
// It must be the very first thing seen by the compiler.
// Fixes MSVC 2026 cmath / math.h ordering issue.

#pragma once

// Must be defined before ANY math header (cmath, math.h, or anything that includes them)
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

// Pull in math.h immediately so all float functions (cosf, sinf, etc.) are declared
// in the global namespace before <cmath> tries to reference them
#include <math.h>

// Suppress safe CRT warnings (fopen, sprintf, etc.) — we handle safety ourselves
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
