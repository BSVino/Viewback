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

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <string.h>

#pragma comment(lib, "wsock32")
#pragma comment(lib, "ws2_32")

typedef SOCKET vb__socket_t;
typedef int vb__socklen_t;

#if defined(__GNUC__)
#define VB_ALIGN(x) __attribute__((aligned(x)))
#else
#define VB_ALIGN(x) __declspec(align(x))
#endif

#define VB_INVALID_SOCKET INVALID_SOCKET
#define snprintf _snprintf

#pragma warning(disable:4505) // unreferenced local function has been removed

static int vb__socket_error(void)
{
	return WSAGetLastError();
}

static int vb__socket_set_blocking(vb__socket_t socket, int blocking)
{
	u_long val = !blocking;
	return ioctlsocket(socket, FIONBIO, &val);
}

static int vb__socket_valid(vb__socket_t socket)
{
	return INVALID_SOCKET != socket;
}

static void vb__socket_close(vb__socket_t socket)
{
	closesocket(socket);
}

static int vb__socket_is_blocking_error(int error)
{
	return WSAEWOULDBLOCK == error;
}

static void vb__strcat(char* dest, size_t size, const char* src)
{
	strcat_s(dest, size, src);
}

static void vb__thread_yield()
{
	Sleep(0);
}
