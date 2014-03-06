#include "viewback_client.h"

#ifdef _WIN32
#include <winsock2.h>
#include <pthread.h>
#endif

#include <stdio.h>

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

	if (!vb.Initialize())
		return 1;

	time_t last_time;
	time(&last_time);

	while (true)
	{
		vb.Update();

		time_t now;
		time(&now);
		if (now > last_time && vb.HasConnection())
		{
			for (size_t i = 0; i < vb.GetRegistrations().size(); i++)
			{
				auto& oRegistration = vb.GetRegistrations()[i];
				auto& oData = vb.GetData()[i];
				printf("%s (%d):", oRegistration.m_sFieldName.c_str(), oData.m_aIntData.size() + oData.m_aFloatData.size() + oData.m_aVectorData.size());

				switch (oRegistration.m_eDataType)
				{
				case VB_DATATYPE_INT:
					for (size_t j = oData.m_aIntData.size()>=10?oData.m_aIntData.size()-10:0; j < oData.m_aIntData.size(); j++)
						printf(" %d", oData.m_aIntData[j]);
					break;

				case VB_DATATYPE_FLOAT:
					for (size_t j = oData.m_aFloatData.size() >= 10 ? oData.m_aFloatData.size() - 10 : 0; j < oData.m_aFloatData.size(); j++)
						printf(" %.1f", oData.m_aFloatData[j]);
					break;

				case VB_DATATYPE_VECTOR:
					for (size_t j = oData.m_aVectorData.size() >= 10 ? oData.m_aVectorData.size() - 10 : 0; j < oData.m_aVectorData.size(); j++)
						printf(" (%.0f, %.0f, %.0f)", oData.m_aVectorData[j].x, oData.m_aVectorData[j].y, oData.m_aVectorData[j].z);
					break;
				}

				printf("\n");
			}

			last_time = now;
		}
	}

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}
