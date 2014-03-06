#include "viewback_client.h"

#include "../server/viewback_shared.h"

#include "viewback_servers.h"
#include "viewback_data.h"

bool CViewbackClient::Initialize()
{
	return CViewbackServersThread::Run();
}

void CViewbackClient::Update()
{
	if (CViewbackDataThread::IsConnected())
	{
		std::vector<Packet> aData = CViewbackDataThread::GetData();

		for (size_t i = 0; i < aData.size(); i++)
		{
			m_aMessages.push_back(aData[i]);

			if (aData[i].data_descriptions_size())
				VBPrintf("Merged data descriptions packet.\n");
			else if (aData[i].has_data())
				VBPrintf("Merged data packet for handle %d.\n", aData[i].data().handle());
		}
	}
	else
	{
		unsigned long best_server = CViewbackServersThread::GetServer();

		if (best_server)
		{
			VBPrintf("Connecting to server at %u ...", best_server);

			bool bResult = CViewbackDataThread::Run(best_server);

			if (bResult)
				VBPrintf("Success.\n");
			else
				VBPrintf("Failed.\n");
		}
	}
}

