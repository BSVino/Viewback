#include "viewback_servers.h"

#ifdef __linux__
#include <string.h>
#endif

#include "../server/viewback_shared.h"

using namespace std;

atomic<bool> CViewbackServersThread::s_bShutdown;

CViewbackServersThread& CViewbackServersThread::ServersThread()
{
	static CViewbackServersThread t;

	return t;
}

bool CViewbackServersThread::Run()
{
	return ServersThread().Initialize();
}

void CViewbackServersThread::Shutdown()
{
	s_bShutdown = true;

	pthread_join(ServersThread().m_iThread, NULL);

	ServersThread().m_aServers.clear();
	vb_socket_close(ServersThread().m_socket);
	s_best_server = 0;
}

bool CViewbackServersThread::Initialize()
{
	m_aServers.clear();
	s_best_server = 0;

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
	mreq.imr_multiaddr.s_addr=inet_addr(VB_DEFAULT_MULTICAST_ADDRESS);
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

#define MSGBUFSIZE 1024

void CViewbackServersThread::Pump()
{
	char msgbuf[MSGBUFSIZE];

	struct sockaddr_in addr;
	vb_socklen_t addrlen = sizeof(addr);

	vb_socket_set_blocking(m_socket, 0);

	int iBytesRead;
	if ((iBytesRead = recvfrom(m_socket, msgbuf, MSGBUFSIZE, 0, (struct sockaddr *)&addr, &addrlen)) < 0)
	{
		int error = vb_socket_error();
		if (!vb_socket_is_blocking_error(error))
		{
			VBPrintf("Error reading multicast socket: %d.\n", error);
			return;
		}
	}

	vb_socket_set_blocking(m_socket, 1);

	VBAssert(iBytesRead < MSGBUFSIZE);

	if (strncmp(msgbuf, "VB: ", 4) != 0)
		// This must be some other packet.
		return;

	time_t now;
	time(&now);

	CServer& server = m_aServers[ntohl(addr.sin_addr.s_addr)];
	server.address = ntohl(addr.sin_addr.s_addr);
	server.last_ping = now;

	time_t most_recent = 0;
	CServer most_recent_server;

	for (map<unsigned long, CServer>::iterator it = m_aServers.begin(); it != m_aServers.end(); it++)
	{
		if (it->second.last_ping > most_recent)
		{
			most_recent = it->second.last_ping;
			most_recent_server = it->second;
		}
	}

	// Servers more than so many seconds old are dead.
	// No sync worries, writing a single long is atomic.
	if (now - most_recent < 10)
		s_best_server = most_recent_server.address;
	else
		s_best_server = 0;

	return;
}

std::atomic<long> CViewbackServersThread::s_best_server(0);
