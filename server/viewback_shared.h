/*
Copyright (c) 2014, Jorge Rodriguez, bs.vino@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

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
do { \
	VBPrintf("Viewback unimplemented code reached in file " __FILE__ " line %d\n", __LINE__); \
	VBDebugBreak(); \
} while(0) \

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
do { \
	PRAGMA_WARNING_PUSH \
	PRAGMA_WARNING_DISABLE(4127) /* conditional expression is constant */ \
	if (!(x)) \
	PRAGMA_WARNING_POP \
	{ \
		VBPrintf("Viewback assert failed: " #x " file " __FILE__ " line %d\n", __LINE__); \
		VBDebugBreak(); \
	} \
} while (0) \

#else
#define VBAssert(x) do {} while(0)
#define VBUnimplemented() do {} while(0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void vb__debug_printf(const char* format, ...);
#define VBPrintf vb__debug_printf

#ifdef __cplusplus
}
#endif
