#include "viewback_data.h"

#include "../server/viewback_shared.h"

using namespace std;

std::atomic<bool> CViewbackDataThread::s_bConnected = false;
std::vector<Packet> CViewbackDataThread::s_aDataDrop;
std::atomic<bool> CViewbackDataThread::s_bDropReady(false);

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

	// Start by requesting a list of data descriptions from the server.
	const char registrations[] = "registrations";
	int bytes_sent = send(m_socket, (const char*)registrations, sizeof(registrations), 0);

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
		if ((iBytesRead = recv(m_socket, msgbuf, MSGBUFSIZE, 0)) < 0)
		{
			s_bConnected = false;
			return;
		}

		if (iBytesRead == 0)
		{
			s_bConnected = false;
			return;
		}

		std::vector<char> aCharsRead(msgbuf, msgbuf + iBytesRead);
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

		if (packet.data_descriptions_size())
			VBPrintf("Received a data registration packet, %u bytes read.\n", iPacketSize);
		else if (packet.has_data())
			VBPrintf("Received a data packet, %u bytes read, handle %d.\n", iPacketSize, packet.data().handle());

		m_aMessages.push_back(packet);

		// Fast forward past the packet size and the packet itself.
		iCurrentPacket += sizeof(size_t)+iPacketSize;
	}

	VBAssert(iCurrentPacket == aMsgBuf.size());

	if (!s_bDropReady)
	{
		VBAssert(!s_aDataDrop.size());

		for (size_t i = 0; i < m_aMessages.size(); i++)
			s_aDataDrop.push_back(m_aMessages[i]);

		s_bDropReady = true;

		m_aMessages.clear();
	}
}

// This function runs as part of the main thread.
std::vector<Packet> CViewbackDataThread::GetData()
{
	if (s_bDropReady)
	{
		std::vector<Packet> temp = s_aDataDrop; // Make a copy.
		s_aDataDrop.clear();
		s_bDropReady = false; // The data thread can have it back now.
		return temp;
	}

	return std::vector<Packet>();
}
