#pragma once

#include <map>
#include <vector>

// This is probably way wrong.
#if defined(__linux__) && !defined(__ANDROID__)
#include <cstdatomic>
#else
#include <atomic>
#endif

#include <pthread.h>

#include "../server/viewback_shared.h"

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
	static void Shutdown();

	static unsigned long GetServer() { return s_best_server; }

private:
	CViewbackServersThread() {};
	static CViewbackServersThread& ServersThread();

private:
	bool Initialize();

	static void ThreadMain(CViewbackServersThread* pThis);

	void Pump();

private:
	pthread_t m_iThread;

	vb_socket_t          m_socket;

	std::map<unsigned long, CServer> m_aServers;

	static std::atomic<long> s_best_server;
	static std::atomic<bool> s_bShutdown;
};
