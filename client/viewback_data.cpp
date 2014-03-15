#include "viewback_data.h"

#include "../server/viewback_shared.h"

using namespace std;

atomic<bool> CViewbackDataThread::s_bConnected;
vector<Packet> CViewbackDataThread::s_aDataDrop;
atomic<bool> CViewbackDataThread::s_bDataDropReady;
string CViewbackDataThread::s_sCommandDrop;
atomic<bool> CViewbackDataThread::s_bCommandDropReady;

bool CViewbackDataThread::Run(unsigned long address)
{
	static CViewbackDataThread t;

	return t.Initialize(address);
}

bool CViewbackDataThread::Initialize(unsigned long address)
{
	u_int yes=1;

	if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return false;

	CCleanupSocket c(m_socket);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=inet_addr("192.168.1.12");//htonl(address);
	addr.sin_port=htons(VB_DEFAULT_PORT);

	if (connect(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return false;

	// Start by requesting a list of data registrations from the server.
	const char registrations[] = "registrations";
	int bytes_sent = send(m_socket, (const char*)registrations, sizeof(registrations), 0);

	s_bConnected = false;
	s_bDataDropReady = false;
	s_bCommandDropReady = true;

	if (pthread_create(&m_iThread, NULL, (void *(*) (void *))&CViewbackDataThread::ThreadMain, (void*)this) != 0)
		return false;

	c.Success();

	s_bConnected = true;
	return true;
}

void CViewbackDataThread::ThreadMain(CViewbackDataThread* pThis)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	while (s_bConnected)
		pThis->Pump();

	google::protobuf::ShutdownProtobufLibrary();
}

#define MSGBUFSIZE 1024

void CViewbackDataThread::Pump()
{
	char msgbuf[MSGBUFSIZE];

	vector<char> aMsgBuf;

	int iBytesRead;
	while (true)
	{
		vb_set_blocking(m_socket, 0);

		if ((iBytesRead = recv(m_socket, msgbuf, MSGBUFSIZE, 0)) < 0)
		{
			if (!vb_is_blocking_error(vb_socket_error()))
			{
				s_bConnected = false;
				return;
			}

			MaintainDrops();
			continue;
		}

		vb_set_blocking(m_socket, 1);

		if (iBytesRead == 0)
		{
			if (!vb_is_blocking_error(vb_socket_error()))
			{
				s_bConnected = false;
				return;
			}

			MaintainDrops();
			continue;
		}

		vector<char> aCharsRead(msgbuf, msgbuf + iBytesRead);
		aMsgBuf.insert(aMsgBuf.end(), aCharsRead.begin(), aCharsRead.end());

		if (iBytesRead < MSGBUFSIZE)
			break;
	}

	size_t iCurrentPacket = 0;
	char* pMsgBuf = aMsgBuf.data();

	while (iCurrentPacket < aMsgBuf.size())
	{
		// The first item will be the packet size.
		size_t iPacketSize = ntohl(*(size_t*)(&pMsgBuf[iCurrentPacket]));

		// Fast forward sizeof(size_t) bytes to skip the packet size, then parse the next item.
		Packet packet;
		packet.ParseFromArray(&pMsgBuf[iCurrentPacket] + sizeof(size_t), iPacketSize);

		m_aMessages.push_back(packet);

		// Fast forward past the packet size and the packet itself.
		iCurrentPacket += sizeof(size_t)+iPacketSize;
	}

	VBAssert(iCurrentPacket == aMsgBuf.size());

	MaintainDrops();
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

		string sMessage = "console: " + sCommand + "\0";

		// Send it to the server
		int bytes_sent = send(m_socket, sMessage.c_str(), sMessage.length()+1, 0); // +1 length for the terminal null
	}
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
