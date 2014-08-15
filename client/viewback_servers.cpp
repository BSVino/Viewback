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

#include "viewback_servers.h"

#ifdef __linux__
#include <string.h>
#endif

#include <algorithm>

#include "../server/viewback_shared.h"

using namespace std;
using namespace vb;

atomic<bool> CViewbackServersThread::s_bShutdown;
std::vector<CServerListing> CViewbackServersThread::s_servers_drop;
pthread_mutex_t CViewbackServersThread::s_servers_drop_mutex;

CViewbackServersThread& CViewbackServersThread::ServersThread()
{
	static CViewbackServersThread t;

	return t;
}

bool CViewbackServersThread::Run()
{
	pthread_mutex_init(&s_servers_drop_mutex, nullptr);

	return ServersThread().Initialize();
}

void CViewbackServersThread::Shutdown()
{
	s_bShutdown = true;

	pthread_join(ServersThread().m_iThread, NULL);

	ServersThread().m_aServers.clear();
	vb__socket_close(ServersThread().m_socket);

	pthread_mutex_destroy(&s_servers_drop_mutex);
}

bool CViewbackServersThread::Initialize()
{
	m_aServers.clear();
	m_iNextServerListUpdate = 0;

	struct ip_mreq mreq;

	u_int yes=1;

	/* create what looks like an ordinary UDP socket */
	if ((m_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		VBPrintf("Could not create multicast socket.\n");
		return false;
	}

	/* allow multiple sockets to use the same PORT number */
	if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes)) < 0)
	{
		VBPrintf("Could not change multicast socket options.\n");
		return false;
	}

	/* set up destination address */
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
	addr.sin_port=htons(VB_DEFAULT_PORT);

	/* bind to receive address */
	if (bind(m_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		VBPrintf("Could not bind multicast socket.\n");
		return false;
	}

	/* use setsockopt() to request that the kernel join a multicast group */
	inet_pton(AF_INET, VB_DEFAULT_MULTICAST_ADDRESS, &mreq.imr_multiaddr);
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);
	if (setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0)
	{
		VBPrintf("Could not join multicast group.\n");
		return false;
	}

	if (pthread_create(&m_iThread, NULL, (void *(*) (void *))&CViewbackServersThread::ThreadMain, (void*)this) != 0)
	{
		VBPrintf("Could not create multicast listener thread.\n");
		return false;
	}

	VBPrintf("Now listening for multicast packets on %s:%d.\n", inet_ntoa(mreq.imr_multiaddr), ntohs(addr.sin_port));

	return true;
}

void CViewbackServersThread::ThreadMain(CViewbackServersThread* pThis)
{
	while (!s_bShutdown)
		pThis->Pump();
}

bool last_ping_sort(const CServerListing& l, const CServerListing& r)
{
	return l.last_ping > r.last_ping;
}

#define MSGBUFSIZE 1024

void CViewbackServersThread::Pump()
{
	vb__thread_yield();

	time_t now;
	time(&now);

	if (now >= m_iNextServerListUpdate)
	{
		std::vector<CServerListing> server_list;
		for (map<unsigned long, CServerListing>::iterator it = m_aServers.begin(); it != m_aServers.end(); it++)
		{
			if (now - it->second.last_ping > 10)
				continue;

			server_list.push_back(it->second);
		}

		std::sort(server_list.begin(), server_list.end(), &last_ping_sort);

		pthread_mutex_lock(&s_servers_drop_mutex);

		s_servers_drop = server_list;

		pthread_mutex_unlock(&s_servers_drop_mutex);

		m_iNextServerListUpdate = now + 1;
	}

	char msgbuf[MSGBUFSIZE];

	struct sockaddr_in addr;
	vb__socklen_t addrlen = sizeof(addr);

	vb__socket_set_blocking(m_socket, 0);

	int iBytesRead;
	if ((iBytesRead = recvfrom(m_socket, msgbuf, MSGBUFSIZE, 0, (struct sockaddr *)&addr, &addrlen)) < 0)
	{
		int error = vb__socket_error();
		if (!vb__socket_is_blocking_error(error))
		{
			VBPrintf("Error reading multicast socket: %d.\n", error);
			return;
		}
	}

	vb__socket_set_blocking(m_socket, 1);

	VBAssert(iBytesRead < MSGBUFSIZE);

	if (strncmp(msgbuf, "VB", 2) != 0)
		// This must be some other packet.
		return;

	if (msgbuf[2] > 1)
		// Version is too new.
		return;

	time(&now);

	unsigned long server_address = ntohl(addr.sin_addr.s_addr);
	unsigned short server_port;
	std::string server_name;

	if (msgbuf[2] == 1)
	{
		server_port = ntohs(*((unsigned short*)(&msgbuf[3])));
		server_name = std::string(msgbuf + 5);
	}
	else
	{
		VBUnimplemented();
		return;
	}

	unsigned long index = server_port ^ server_address;

	CServerListing& server = m_aServers[index];

	server.address = server_address;
	server.last_ping = now;
	server.name = server_name;
	server.tcp_port = server_port;

	// Force update now.
	m_iNextServerListUpdate = 0;
}

vector<CServerListing> CViewbackServersThread::GetServers()
{
	vector<CServerListing> server_list;

	pthread_mutex_lock(&s_servers_drop_mutex);

	server_list = s_servers_drop;

	pthread_mutex_unlock(&s_servers_drop_mutex);

	// A bit inefficient since we make a copy once during the mutex lock and a copy again when we return the value.
	// Don't think it's ever going to be a perf problem though.
	return server_list;
}
