#pragma once

#if defined(_WIN32)
#include "viewback_win32.h"
#elif defined(__ANDROID__)
/* Android runs Linux. */
#include "viewback_unix.h"
#elif defined(__linux__)
#include "viewback_unix.h"
#else
#error !
#endif

#define VB_DEFAULT_PORT 51072
#define VB_DEFAULT_MULTICAST_ADDRESS "239.127.251.37"

#ifdef _DEBUG

#ifdef __GNUC__
#include <csignal>
#define VBDebugBreak() ::raise(SIGTRAP);
#else
#define VBDebugBreak() __debugbreak();
#endif

// If you hit this, this code path was left incomplete because it wasn't
// necessary at the time. Since you're using it now you need to implement
// it yourself.
#define VBUnimplemented() \
{ \
	VBPrintf("Viewback unimplemented code reached in file " __FILE__ " line %d\n", __LINE__); \
	VBDebugBreak(); \
} \

#ifdef _MSC_VER
#define PRAGMA_WARNING_PUSH __pragma(warning(push))
#define PRAGMA_WARNING_DISABLE(n) __pragma(warning(disable:n))
#define PRAGMA_WARNING_POP __pragma(warning(pop))
#else
#define PRAGMA_WARNING_PUSH
#define PRAGMA_WARNING_DISABLE(n)
#define PRAGMA_WARNING_POP
#endif

#define VBAssert(x) \
{ \
	PRAGMA_WARNING_PUSH \
	PRAGMA_WARNING_DISABLE(4127) /* conditional expression is constant */ \
	if (!(x)) \
	PRAGMA_WARNING_POP \
	{ \
		VBPrintf("Viewback assert failed: " #x " file " __FILE__ " line %d\n", __LINE__); \
		VBDebugBreak(); \
	} \
} \

#else
#define VBAssert(x) {}
#endif

extern void vb__debug_printf(const char* format, ...);
#define VBPrintf vb__debug_printf

