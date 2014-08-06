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

	vb__socket_t          m_socket;

	std::map<unsigned long, CServer> m_aServers;

	static std::atomic<long> s_best_server;
	static std::atomic<bool> s_bShutdown;
};
