// This code is in the public domain. No warranty implied, use at your own risk.

#include "viewback_client.h"

#ifdef _WIN32
#include <winsock2.h>
#include <pthread.h>
#endif

#include <stdio.h>
#include <random>

#pragma warning(disable:4702) // unreachable code. The last part of main() is unreachable, which is okay since this is just a sample.

using namespace vb;

void ConsoleOutput(const char* pszOutput)
{
	printf("Console output: %s", pszOutput);
}

void DebugOutput(const char* pszOutput)
{
	printf("Console output: %s", pszOutput);
}

int main()
{
#ifdef _WIN32
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
		return 1;

#ifdef PTW32_STATIC_LIB
	pthread_win32_process_attach_np();
#endif
#endif

	CViewbackClient vb;

	if (!vb.Initialize(NULL, &ConsoleOutput, DebugOutput))
		return 1;

	time_t last_time;
	time(&last_time);

	srand((int)last_time);

	for (;;)
	{
		vb.Update();

		time_t now;
		time(&now);
		if (now > last_time && vb.HasConnection())
		{
			for (size_t i = 0; i < vb.GetChannels().size(); i++)
			{
				auto& oRegistration = vb.GetChannels()[i];
				auto& oData = vb.GetData()[i];
				printf("%s (%d):", oRegistration.m_sName.c_str(), oData.m_aIntData.size() + oData.m_aFloatData.size() + oData.m_aVectorData.size());

				switch (oRegistration.m_eDataType)
				{
				case VB_DATATYPE_INT:
					for (size_t j = oData.m_aIntData.size()>=10?oData.m_aIntData.size()-10:0; j < oData.m_aIntData.size(); j++)
						printf(" %.2f: %s", oData.m_aIntData[j].time, vb.GetLabelForValue(i, oData.m_aIntData[j].data).c_str());
					break;

				case VB_DATATYPE_FLOAT:
					for (size_t j = oData.m_aFloatData.size() >= 10 ? oData.m_aFloatData.size() - 10 : 0; j < oData.m_aFloatData.size(); j++)
						printf(" %.2f: %.1f", oData.m_aFloatData[j].time, oData.m_aFloatData[j].data);
					break;

				case VB_DATATYPE_VECTOR:
					for (size_t j = oData.m_aVectorData.size() >= 10 ? oData.m_aVectorData.size() - 10 : 0; j < oData.m_aVectorData.size(); j++)
						printf(" %.2f: (%.0f, %.0f, %.0f)", oData.m_aVectorData[j].time, oData.m_aVectorData[j].data.x, oData.m_aVectorData[j].data.y, oData.m_aVectorData[j].data.z);
					break;
				}

				printf("\n");
			}

			printf("Current status: %s\n", vb.GetStatus().c_str());

			if (rand() % 2 == 0)
			{
				const char* pszCommand;
				switch (rand() % 5)
				{
				case 0:
				default:
					pszCommand = "sv_cheats 1";
					break;

				case 1:
					pszCommand = "map de_dust";
					break;

				case 2:
					pszCommand = "connect 192.168.1.100:27015";
					break;

				case 3:
					pszCommand = "status";
					break;

				case 4:
					pszCommand = "rcon kick GabeN";
					break;
				}

				printf("Sending command: %s\n", pszCommand);
				vb.SendConsoleCommand(pszCommand);
			}

			if (vb.GetChannels().size() && rand() % 2)
			{
				int iChannel = rand() % vb.GetChannels().size();
				if (vb.GetChannels()[iChannel].m_bActive)
				{
					printf("Deactivating channel %d - \"%s\"\n", iChannel, vb.GetChannels()[iChannel].m_sName.c_str());
					vb.DeactivateChannel(iChannel);
				}
				else
				{
					printf("Activate channel %d - \"%s\"\n", iChannel, vb.GetChannels()[iChannel].m_sName.c_str());
					vb.ActivateChannel(iChannel);
				}
			}

			last_time = now;
		}
	}

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}
