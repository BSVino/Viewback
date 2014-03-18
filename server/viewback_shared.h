#pragma once

#if defined(_WIN32)
#include "viewback_win32.h"
#elif defined(__ANDROID__)
/* Android runs Linux. */
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

#define VBAssert(x) \
{ \
	if (!(x)) \
	{ \
		VBDebugBreak(); \
	} \
} \

#else
#define VBAssert(x) {}
#endif

#ifdef _DEBUG
#ifdef __ANDROID__
#include <android/log.h>
#define VBPrintf android_printf
inline void android_printf(const char* pszFormat, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, pszFormat);
	vsnprintf(buf, sizeof(buf), pszFormat, ap);
	va_end(ap);

	__android_log_print(ANDROID_LOG_INFO, "Viewback", "%s", buf);
}
#else
#include <stdio.h>
#define VBPrintf printf
#endif
#else
#define VBPrintf no_printf
inline void no_printf(...) {}
#endif

// This quick class automates the cleanup of sockets in case the server creation fails for some reason.
class CCleanupSocket
{
public:
	CCleanupSocket(vb_socket_t socket)
	{
		_socket = socket;
		_success = false;
	}

	~CCleanupSocket()
	{
		if (!_success && vb_valid_socket(_socket))
			vb_close_socket(_socket);
	}

public:
	void Success()
	{
		_success = true;
	}

private:
	vb_socket_t _socket;
	bool        _success;
};
