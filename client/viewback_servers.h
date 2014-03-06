#pragma once

#include <map>
#include <vector>
#include <atomic>

#include <pthread.h>

#include "../server/viewback_win32.h"

class CViewbackServersThread
{
public:
	class CServer
	{
	public:
		unsigned long address;
		time_t        last_ping;
	};

public:
	static bool Run();

	static unsigned long GetServer() { return s_best_server; }

private:
	CViewbackServersThread() {};

private:
	bool Initialize();

	static void ThreadMain(CViewbackServersThread* pThis);

	void Pump();

private:
	pthread_t m_iThread;

	vb_socket_t          m_socket;

	std::map<unsigned long, CServer> m_aServers;

	static std::atomic<long> s_best_server;
};
