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
	static bool Connect(unsigned long address, int iPort = 0);
	static bool IsConnected() { return s_bConnected; }
	static void Disconnect();
	static std::vector<Packet> GetData();

	static bool SendConsoleCommand(const std::string& sCommand);

private:
	CViewbackDataThread();
	static CViewbackDataThread& DataThread();

private:
	bool Initialize(unsigned long address, int port);

	static void ThreadMain(CViewbackDataThread* pThis);

	void Pump();

	void MaintainDrops();

private:
	pthread_t m_iThread;

	vb_socket_t        m_socket;

	std::vector<Packet> m_aMessages;

	// Thread signalling.
	static std::atomic<bool> s_bRunning;

	static std::atomic<bool>   s_bConnected; // Read/write for the data thread, read only for others.

	static std::vector<Packet> s_aDataDrop;
	static std::atomic<bool>   s_bDataDropReady; // Data thread sets up s_aDataDrop and then flips this. Main thread clears s_aDataDrop and then flips it back.

	static std::string       s_sCommandDrop;
	static std::atomic<bool> s_bCommandDropReady; // Main thread sets up s_sCommandDrop and then flips this. Data thread clears s_sCommandDrop and then flips it back.

	static std::atomic<bool> s_bDisconnect; // Read/write for other threads, read only for the data thread. While this flag is on, data thread is to remain disconnected.
};

