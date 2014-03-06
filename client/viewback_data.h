#pragma once

#include <vector>
#include <atomic>

#include <pthread.h>

#include "../server/viewback_win32.h"

#include "../protobuf/data.pb.h"

class CViewbackDataThread
{
public:
	static bool Run(unsigned long address);
	static bool IsConnected() { return s_bConnected; }
	static std::vector<Packet> GetData();

private:
	CViewbackDataThread() {};

private:
	bool Initialize(unsigned long address);

	static void ThreadMain(CViewbackDataThread* pThis);

	void Pump();

private:
	pthread_t m_iThread;

	vb_socket_t        m_socket;

	std::vector<Packet> m_aMessages;

	// Thread signalling.
	static std::atomic<bool>   s_bConnected;
	static std::vector<Packet> s_aDataDrop;
	static std::atomic<bool>   s_bDropReady;
};

