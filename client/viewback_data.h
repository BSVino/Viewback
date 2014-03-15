#pragma once

#include <vector>
#include <atomic>

#include <pthread.h>

#include "../protobuf/data.pb.h"

#include "../server/viewback_shared.h"

class CViewbackDataThread
{
public:
	static bool Run(unsigned long address);
	static bool IsConnected() { return s_bConnected; }
	static std::vector<Packet> GetData();

	static bool SendConsoleCommand(const std::string& sCommand);

private:
	CViewbackDataThread() {};

private:
	bool Initialize(unsigned long address);

	static void ThreadMain(CViewbackDataThread* pThis);

	void Pump();

	void MaintainDrops();

private:
	pthread_t m_iThread;

	vb_socket_t        m_socket;

	std::vector<Packet> m_aMessages;

	// Thread signalling.
	static std::atomic<bool>   s_bConnected;

	static std::vector<Packet> s_aDataDrop;
	static std::atomic<bool>   s_bDataDropReady;

	static std::string       s_sCommandDrop;
	static std::atomic<bool> s_bCommandDropReady;
};

