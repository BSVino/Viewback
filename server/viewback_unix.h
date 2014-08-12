#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

typedef int vb__socket_t;
typedef socklen_t vb__socklen_t;

#define VB_ALIGN(x) __attribute__((aligned(x)))
#define VB_INVALID_SOCKET (-1)

static int vb__socket_error(void)
{
	return errno;
}

static int vb__socket_valid(vb__socket_t socket)
{
	return socket > 0;
}

static void vb__socket_close(vb__socket_t socket)
{
	close(socket);
}

static int vb__socket_is_blocking_error(int error)
{
	return EAGAIN == error;
}

static int vb__socket_set_blocking(vb__socket_t socket, int blocking)
{
	return fcntl(socket, F_SETFL, O_NONBLOCK, !blocking);
}

static void vb__strcat(char* dest, size_t size, const char* src)
{
	strcat(dest, src);
}

static void vb__thread_yield()
{
	sched_yield();
}
