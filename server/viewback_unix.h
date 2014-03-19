#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

typedef int vb_socket_t;
typedef socklen_t vb_socklen_t;

#define VB_ALIGN(x) __attribute__((aligned(x)))
#define VB_INVALID_SOCKET (-1)

static int vb_socket_error(void)
{
	return errno;
}

static int vb_valid_socket(vb_socket_t socket)
{
	return socket > 0;
}

static void vb_close_socket(vb_socket_t socket)
{
	close(socket);
}

static int vb_is_blocking_error(int error)
{
	return EAGAIN == error;
}

static int vb_set_blocking(vb_socket_t socket, int blocking)
{
	return fcntl(socket, F_SETFL, O_NONBLOCK, !blocking);
}


