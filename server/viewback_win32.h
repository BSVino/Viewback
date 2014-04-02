#pragma once

#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "wsock32")
#pragma comment(lib, "ws2_32")

typedef SOCKET vb_socket_t;
typedef int vb_socklen_t;

#if defined(__GNUC__)
#define VB_ALIGN(x) __attribute__((aligned(x)))
#else
#define VB_ALIGN(x) __declspec(align(x))
#endif

#define VB_INVALID_SOCKET INVALID_SOCKET
#define snprintf _snprintf

#pragma warning(disable:4505) // unreferenced local function has been removed

static int vb_socket_error(void)
{
	return WSAGetLastError();
}

static int vb_socket_set_blocking(vb_socket_t socket, int blocking)
{
	u_long val = !blocking;
	return ioctlsocket(socket, FIONBIO, &val);
}

static int vb_socket_valid(vb_socket_t socket)
{
	return INVALID_SOCKET != socket;
}

static void vb_socket_close(vb_socket_t socket)
{
	closesocket(socket);
}

static int vb_socket_is_blocking_error(int error)
{
	return WSAEWOULDBLOCK == error;
}
