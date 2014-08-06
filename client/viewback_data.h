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

#include <vector>

// This is probably way wrong.
#if defined(__linux__) && !defined(__ANDROID__)
#include <cstdatomic>
#else
#include <atomic>
#endif

#include <pthread.h>

#include "../protobuf/data.pb.h"

#include "../server/viewback_shared.h"

class CViewbackDataThread
{
public:
	static bool Connect(unsigned long address, unsigned short iPort = 0);
	static void Shutdown();
	static bool IsConnected() { return s_bConnected; }
	static void Disconnect();
	static std::vector<Packet> GetData();

	static bool SendConsoleCommand(const std::string& sCommand);

private:
	CViewbackDataThread();
	static CViewbackDataThread& DataThread();

private:
	bool Initialize(unsigned long address, unsigned short port);

	static void ThreadMain(CViewbackDataThread* pThis);

	void Pump();

	void MaintainDrops();

private:
	pthread_t m_iThread;

	vb__socket_t        m_socket;

	std::vector<Packet> m_aMessages;

	std::vector<char>   m_aLeftover;

	// Thread signalling.
	static std::atomic<bool> s_bRunning;

	static std::atomic<bool>   s_bConnected; // Read/write for the data thread, read only for others.

	static std::vector<Packet> s_aDataDrop;
	static std::atomic<bool>   s_bDataDropReady; // Data thread sets up s_aDataDrop and then flips this. Main thread clears s_aDataDrop and then flips it back.

	static std::string       s_sCommandDrop;
	static std::atomic<bool> s_bCommandDropReady; // Main thread sets up s_sCommandDrop and then flips this. Data thread clears s_sCommandDrop and then flips it back.

	static std::atomic<bool> s_bDisconnect; // Read/write for other threads, read only for the data thread. While this flag is on, data thread is to remain disconnected.
};

