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

		// Look for a data registration packet.
		for (size_t i = 0; i < aData.size(); i++)
		{
			if (aData[i].data_descriptions_size())
			{
				m_aDataRegistrations.resize(aData[i].data_descriptions_size());
				m_aData.resize(aData[i].data_descriptions_size());
				for (int j = 0; j < aData[i].data_descriptions_size(); j++)
				{
					auto& oRegistrationProtobuf = aData[i].data_descriptions(j);
					auto& oRegistration = m_aDataRegistrations[oRegistrationProtobuf.handle()];

					VBAssert(oRegistrationProtobuf.has_handle());
					VBAssert(oRegistrationProtobuf.has_field_name());
					VBAssert(oRegistrationProtobuf.has_type());
					VBAssert(oRegistrationProtobuf.handle() == j);

					oRegistration.m_iHandle = oRegistrationProtobuf.handle();
					oRegistration.m_sFieldName = oRegistrationProtobuf.field_name();
					oRegistration.m_eDataType = oRegistrationProtobuf.type();
				}

				VBPrintf("Installed %d registrations.\n", aData[i].data_descriptions_size());
			}
		}

		if (!m_aDataRegistrations.size())
		{
			// We somehow don't have any data registrations yet, so stash these messages for later.
			// It might be possible if the server sends some messages between when the client connects and when it requests registrations.
			for (size_t i = 0; i < aData.size(); i++)
				m_aUnhandledMessages.push_back(aData[i]);

			return;
		}

		// If we've been saving any messages, stick them onto the beginning here.
		if (m_aUnhandledMessages.size())
			aData.insert(aData.begin(), m_aUnhandledMessages.begin(), m_aUnhandledMessages.end());

		m_aUnhandledMessages.clear();

		for (size_t i = 0; i < aData.size(); i++)
			StashData(&aData[i].data());
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

vb_data_type_t CViewbackClient::TypeForHandle(size_t iHandle)
{
	return m_aDataRegistrations[iHandle].m_eDataType;
}

void CViewbackClient::StashData(const Data* pData)
{
	switch (TypeForHandle(pData->handle()))
	{
	case VB_DATATYPE_NONE:
	default:
		VBAssert(false);
		break;

	case VB_DATATYPE_INT:
		m_aData[pData->handle()].m_aIntData.push_back(pData->data_int());
		break;

	case VB_DATATYPE_FLOAT:
		m_aData[pData->handle()].m_aFloatData.push_back(pData->data_float());
		break;

	case VB_DATATYPE_VECTOR:
		m_aData[pData->handle()].m_aVectorData.push_back(VBVector3(pData->data_float_x(), pData->data_float_y(), pData->data_float_z()));
		break;
	}
}
