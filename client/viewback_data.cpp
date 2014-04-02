#include "viewback_data.h"

#include "../server/viewback_shared.h"

using namespace std;

atomic<bool> CViewbackDataThread::s_bRunning;
atomic<bool> CViewbackDataThread::s_bConnected;
vector<Packet> CViewbackDataThread::s_aDataDrop;
atomic<bool> CViewbackDataThread::s_bDataDropReady;
string CViewbackDataThread::s_sCommandDrop;
atomic<bool> CViewbackDataThread::s_bCommandDropReady;
atomic<bool> CViewbackDataThread::s_bDisconnect;

CViewbackDataThread::CViewbackDataThread()
{
	s_bRunning = false;
}

CViewbackDataThread& CViewbackDataThread::DataThread()
{
	static CViewbackDataThread t;

	return t;
}

bool CViewbackDataThread::Connect(unsigned long address, int port)
{
	if (s_bRunning)
	{
		// Another data thread is still running. That is bad. Hopefully we told it to disconnect and we're just waiting on that.
		VBAssert(!s_bConnected);

		// If not, make sure that we do.
		s_bDisconnect = true;

		pthread_join(DataThread().m_iThread, NULL);
	}

	// We are being ordered to connect to something. Thus, we should not remain disconnected any longer.
	s_bDisconnect = false;

	return DataThread().Initialize(address, port);
}

void CViewbackDataThread::Shutdown()
{
	if (s_bRunning)
	{
		s_bDisconnect = true;

		pthread_join(DataThread().m_iThread, NULL);
	}
}

bool CViewbackDataThread::Initialize(unsigned long address, int port)
{
	u_int yes=1;

	if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		VBPrintf("Could not create data socket.\n");
		return false;
	}

	CCleanupSocket c(m_socket);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(address);
	addr.sin_port=htons(port == 0?VB_DEFAULT_PORT:port);

	if (connect(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		VBPrintf("Could not connect to viewback server.\n");
		return false;
	}

	VBPrintf("Connected to Viewback server at %s:%d.\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	// Start by requesting a list of data registrations from the server.
	const char registrations[] = "registrations";
	int bytes_sent = send(m_socket, (const char*)registrations, sizeof(registrations), 0);

	// Any data drops are lying around from last time, so clear them out.
	s_aDataDrop.clear();
	s_sCommandDrop.clear();

	s_bConnected = false;
	s_bDataDropReady = false;
	s_bCommandDropReady = true;
	s_bDisconnect = false;

	if (pthread_create(&m_iThread, NULL, (void *(*) (void *))&CViewbackDataThread::ThreadMain, (void*)this) != 0)
	{
		VBPrintf("Could not create data thread.\n");
		return false;
	}

	c.Success();

	s_bConnected = true;
	return true;
}

void CViewbackDataThread::ThreadMain(CViewbackDataThread* pThis)
{
	s_bRunning = true;

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	while (s_bConnected && !s_bDisconnect)
		pThis->Pump();

	s_bConnected = false;
	vb_socket_close(pThis->m_socket);

	google::protobuf::ShutdownProtobufLibrary();

	s_bRunning = false;
}

#define MSGBUFSIZE 1024

void CViewbackDataThread::Pump()
{
	MaintainDrops();

	char msgbuf[MSGBUFSIZE];

	int iBytesRead;

	vb_socket_set_blocking(m_socket, 0);

	iBytesRead = recv(m_socket, msgbuf, MSGBUFSIZE, 0);

	int iError = vb_socket_error();

	vb_socket_set_blocking(m_socket, 1);

	// 0 always means the connection was lost.
	if (iBytesRead == 0)
	{
		vb_socket_close(m_socket);
		s_bConnected = false;
		return;
	}

	if (iBytesRead < 0)
	{
		// It would have blocked, meaning there's no data available.
		if (vb_socket_is_blocking_error(iError))
			return;

		// There was a real error, we're not connected anymore.
		vb_socket_close(m_socket);
		s_bConnected = false;
		return;
	}

	vector<char> aMsgBuf;

	// There's a partial packet left over from last time, tack it onto the front.
	if (m_aLeftover.size())
	{
		aMsgBuf.insert(aMsgBuf.end(), m_aLeftover.begin(), m_aLeftover.end());
		m_aLeftover.clear();
	}

	vector<char> aBytes(msgbuf, msgbuf + iBytesRead);
	aMsgBuf.insert(aMsgBuf.end(), aBytes.begin(), aBytes.end());

	size_t iCurrentPacket = 0;
	char* pMsgBuf = aMsgBuf.data();

	while (iCurrentPacket < aMsgBuf.size())
	{
		// The first item will be the packet size.
		size_t iPacketSize = ntohl(*(size_t*)(&pMsgBuf[iCurrentPacket]));

		// For some reason we didn't receive all of the bytes for this packet.
		// Stuff it in the leftover and bail.
		if (iCurrentPacket + sizeof(size_t)+iPacketSize > aMsgBuf.size())
		{
			m_aLeftover.insert(m_aLeftover.end(), aMsgBuf.begin() + iCurrentPacket, aMsgBuf.end());
			break;
		}

		// Fast forward sizeof(size_t) bytes to skip the packet size, then parse the next item.
		Packet packet;
		packet.ParseFromArray(&pMsgBuf[iCurrentPacket] + sizeof(size_t), iPacketSize);

		m_aMessages.push_back(packet);

		// Fast forward past the packet size and the packet itself.
		iCurrentPacket += sizeof(size_t)+iPacketSize;
	}
}

void CViewbackDataThread::MaintainDrops()
{
	if (!s_bDataDropReady && m_aMessages.size())
	{
		VBAssert(!s_aDataDrop.size());

		for (size_t i = 0; i < m_aMessages.size(); i++)
			s_aDataDrop.push_back(m_aMessages[i]);

		s_bDataDropReady = true;

		m_aMessages.clear();
	}

	if (!s_bCommandDropReady)
	{
		VBAssert(s_sCommandDrop.length());

		// Stash it away and let the main thread have it back.
		string sCommand = s_sCommandDrop;
		s_bCommandDropReady = true;

		string sMessage = sCommand + "\0";

		// Send it to the server
		int bytes_sent = send(m_socket, sMessage.c_str(), sMessage.length()+1, 0); // +1 length for the terminal null
	}
}

// This function runs as part of the main thread.
void CViewbackDataThread::Disconnect()
{
	s_bDisconnect = true;

	// Don't bother joining the thread here, let it die slowly.
	// We'll join it if it's still running when we try to connect again.
}

// This function runs as part of the main thread.
vector<Packet> CViewbackDataThread::GetData()
{
	if (s_bDataDropReady)
	{
		vector<Packet> temp = s_aDataDrop; // Make a copy.
		s_aDataDrop.clear();
		s_bDataDropReady = false; // The data thread can have it back now.
		return temp;
	}

	return vector<Packet>();
}

// This function runs as part of the main thread.
bool CViewbackDataThread::SendConsoleCommand(const string& sCommand)
{
	if (s_bCommandDropReady)
	{
		s_sCommandDrop = sCommand;
		s_bCommandDropReady = false; // The data thread can have it back now.
		return true;
	}

	return false;
}
