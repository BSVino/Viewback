/*
Copyright (c) 2014, Jorge Rodriguez, bs.vino@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "viewback_client.h"

#include <sstream>
#include <sys/timeb.h>
#include <stdarg.h>

#include "../server/viewback_shared.h"

#include "viewback_servers.h"
#include "viewback_data.h"

using namespace std;
using namespace vb;

static CViewbackClient* VB = NULL;

void vb__debug_printf(const char* format, ...)
{
	if (!VB->GetDebugOutputCallback())
		return;

	char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	VB->GetDebugOutputCallback()(buf);
}

bool CViewbackClient::Initialize(RegistrationUpdateCallback pfnRegistration, ConsoleOutputCallback pfnConsoleOutput, DebugOutputCallback pfnDebugOutput)
{
	VB = this;

	m_flNextDataClear = 10;
	m_flDataClearTime = 0;

	m_pfnRegistrationUpdate = pfnRegistration;
	m_pfnConsoleOutput = pfnConsoleOutput;
	m_pfnDebugOutput = pfnDebugOutput;

	m_bDisconnected = false;

	return CViewbackServersThread::Run();
}

void CViewbackClient::Shutdown()
{
	CViewbackServersThread::Shutdown();
	CViewbackDataThread::Shutdown();
}

void CViewbackClient::Update()
{
	if (CViewbackDataThread::IsConnected())
	{
		size_t iStartPacket = 0;
		vector<Packet> aPackets = CViewbackDataThread::GetData();

		// Look for a data registration packet.
		for (size_t i = 0; i < aPackets.size(); i++)
		{
			if (aPackets[i].is_registration())
			{
				static VBVector3 aclrColors[] = {
					VBVector3(1, 0, 0),
					VBVector3(0, 1, 0),
					VBVector3(0, 0, 1),
					VBVector3(0, 1, 1),
					VBVector3(1, 0, 1),
					VBVector3(1, 1, 0),
				};
				int iColorsSize = sizeof(aclrColors) / sizeof(aclrColors[0]);

				// Disregard any data which came in before the registration packet, it may be from another server or old connection.
				m_aData.clear();
				m_aDataChannels.clear();
				m_aDataGroups.clear();
				m_aDataControls.clear();
				m_aMeta.clear();
				m_aUnhandledMessages.clear();
				m_flLatestDataTime = 0;
				m_flTimeReceivedLatestData = 0;
				iStartPacket = i + 1;

				m_aDataChannels.resize(aPackets[i].data_channels_size());
				m_aData.resize(aPackets[i].data_channels_size());
				m_aMeta.resize(aPackets[i].data_channels_size());

				for (size_t j = 0; j < (size_t)aPackets[i].data_channels_size(); j++)
				{
					auto& oChannelProtobuf = aPackets[i].data_channels(j);

					VBAssert(oChannelProtobuf.has_handle());
					VBAssert(oChannelProtobuf.has_name());
					VBAssert(oChannelProtobuf.has_type());
					VBAssert(oChannelProtobuf.handle() == j);

					auto& oChannel = m_aDataChannels[oChannelProtobuf.handle()];
					oChannel.m_iHandle = oChannelProtobuf.handle();
					oChannel.m_sName = oChannelProtobuf.name();
					oChannel.m_eDataType = oChannelProtobuf.type();

					if (oChannelProtobuf.has_range_min())
						oChannel.m_flMin = oChannelProtobuf.range_min();

					if (oChannelProtobuf.has_range_max())
						oChannel.m_flMax = oChannelProtobuf.range_max();

					m_aMeta[oChannelProtobuf.handle()].m_clrColor = aclrColors[j % iColorsSize];
				}

				VBPrintf("Installed %d channels.\n", aPackets[i].data_channels_size());

				for (int j = 0; j < aPackets[i].data_groups_size(); j++)
				{
					auto& oGroupProtobuf = aPackets[i].data_groups(j);

					VBAssert(oGroupProtobuf.has_name());

					m_aDataGroups.push_back(CViewbackDataGroup());
					auto& oGroup = m_aDataGroups.back();
					oGroup.m_sName = oGroupProtobuf.name();
					for (int k = 0; k < oGroupProtobuf.channels_size(); k++)
						oGroup.m_iChannels.push_back(oGroupProtobuf.channels(k));
				}

				VBPrintf("Installed %d groups.\n", aPackets[i].data_groups_size());

				for (int j = 0; j < aPackets[i].data_labels_size(); j++)
				{
					auto& oLabelProtobuf = aPackets[i].data_labels(j);

					VBAssert(oLabelProtobuf.has_label());
					VBAssert(oLabelProtobuf.has_channel());
					VBAssert(oLabelProtobuf.has_value());

					auto& oChannel = m_aDataChannels[oLabelProtobuf.channel()];
					oChannel.m_asLabels[oLabelProtobuf.value()] = oLabelProtobuf.label();
				}

				VBPrintf("Installed %d labels.\n", aPackets[i].data_controls_size());

				for (int j = 0; j < aPackets[i].data_controls_size(); j++)
				{
					auto& oControlProtobuf = aPackets[i].data_controls(j);

					VBAssert(oControlProtobuf.has_name());
					VBAssert(oControlProtobuf.has_type());

					if (!oControlProtobuf.has_name())
						continue;

					if (!oControlProtobuf.has_type())
						continue;

					if (oControlProtobuf.type() <= 0 || oControlProtobuf.type() >= VB_CONTROL_MAX)
					{
						VBPrintf("Unrecognized control type: %d Need to update your monitor?\n", oControlProtobuf.type());
						continue;
					}

					m_aDataControls.emplace_back();
					m_aDataControls.back().m_name = oControlProtobuf.name();
					m_aDataControls.back().m_type = oControlProtobuf.type();

					switch (m_aDataControls.back().m_type)
					{
					case VB_CONTROL_BUTTON:
						// No parameters.
						break;

					case VB_CONTROL_SLIDER_FLOAT:
						m_aDataControls.back().slider_float.range_min = oControlProtobuf.range_min_float();
						m_aDataControls.back().slider_float.range_max = oControlProtobuf.range_max_float();
						m_aDataControls.back().slider_float.steps = oControlProtobuf.num_steps();
						m_aDataControls.back().slider_float.initial_value = oControlProtobuf.value_float();
						break;

					case VB_CONTROL_SLIDER_INT:
						m_aDataControls.back().slider_int.range_min = oControlProtobuf.range_min_int();
						m_aDataControls.back().slider_int.range_max = oControlProtobuf.range_max_int();
						m_aDataControls.back().slider_int.step_size = oControlProtobuf.step_size();
						m_aDataControls.back().slider_int.initial_value = oControlProtobuf.value_int();
						break;

					default:
						VBUnimplemented();
						break;
					}
				}

				VBPrintf("Installed %d controls.\n", aPackets[i].data_controls_size());

				if (m_pfnRegistrationUpdate)
					m_pfnRegistrationUpdate();
			}
		}

		if (!m_aDataChannels.size() && !m_aDataControls.size() && !m_aDataGroups.size())
		{
			// We somehow don't have any data registrations yet, so stash these messages for later.
			// It might be possible if the server sends some messages between when the client connects and when it requests registrations.
			for (size_t i = 0; i < aPackets.size(); i++)
				m_aUnhandledMessages.push_back(aPackets[i]);

			return;
		}

		// If we've been saving any messages, stick them onto the beginning here.
		if (m_aUnhandledMessages.size())
			aPackets.insert(aPackets.begin(), m_aUnhandledMessages.begin(), m_aUnhandledMessages.end());

		m_aUnhandledMessages.clear();

		for (size_t i = iStartPacket; i < aPackets.size(); i++)
		{
			if (aPackets[i].has_data())
				StashData(&aPackets[i].data());

			if (aPackets[i].has_console_output() && m_pfnConsoleOutput)
				m_pfnConsoleOutput(aPackets[i].console_output().c_str());

			if (aPackets[i].has_status())
				m_sStatus = aPackets[i].status();

			if (aPackets[i].data_controls_size())
			{
				VBAssert(!aPackets[i].is_registration());

				for (size_t j = 0; j < m_aDataControls.size(); j++)
				{
					if (m_aDataControls[j].m_name != aPackets[i].data_controls(0).name())
						continue;

					if (m_pfnControlUpdatedCallback)
						m_pfnControlUpdatedCallback(j, aPackets[i].data_controls(0).value_float(), aPackets[i].data_controls(0).value_int());

					break;
				}
			}
		}

		while (m_sOutgoingCommands.size())
		{
			if (CViewbackDataThread::SendConsoleCommand(m_sOutgoingCommands.front()))
				// Message was received, we can remove this command from the list.
				m_sOutgoingCommands.pop_front();
			else
				// There's already a command waiting to be sent to the data thread, hold on for now.
				break;
		}

		// Clear out old data. First let's find the newest timestamp of any data.
		double flNewest = GetLatestDataTime();
		if (flNewest > m_flNextDataClear)
		{
			for (size_t i = 0; i < m_aData.size(); i++)
			{
				if (m_aDataChannels[i].m_eDataType == VB_DATATYPE_VECTOR)
				{
					while (m_aData[i].m_aVectorData.size() > 2 && m_aData[i].m_aVectorData.front().time < flNewest - m_aMeta[i].m_flDisplayDuration - 10)
						m_aData[i].m_aVectorData.pop_front();
				}
				else if (m_aDataChannels[i].m_eDataType == VB_DATATYPE_FLOAT)
				{
					while (m_aData[i].m_aFloatData.size() > 2 && m_aData[i].m_aFloatData.front().time < m_flDataClearTime)
						m_aData[i].m_aFloatData.pop_front();
				}
				else if (m_aDataChannels[i].m_eDataType == VB_DATATYPE_INT)
				{
					while (m_aData[i].m_aIntData.size() > 2 && m_aData[i].m_aIntData.front().time < m_flDataClearTime)
						m_aData[i].m_aIntData.pop_front();
				}
				else
					VBUnimplemented();
			}

			m_flNextDataClear = flNewest + 10;
		}
	}
	else
	{
		if (m_aData.size())
		{
			// We've been disconnected. Clear it all out.
			for (size_t i = 0; i < m_aData.size(); i++)
			{
				auto& oData = m_aData[i];

				oData.m_aFloatData.clear();
				oData.m_aIntData.clear();
				oData.m_aVectorData.clear();
			}

			m_aData.clear();

			m_aDataChannels.clear();
			m_aDataControls.clear();
			m_aDataGroups.clear();
			m_aMeta.clear();

			m_pfnRegistrationUpdate();
		}
	}
}

vector<CServerListing> CViewbackClient::GetServers()
{
	return CViewbackServersThread::GetServers();
}

bool CViewbackClient::HasConnection()
{
	return CViewbackDataThread::IsConnected();
}

void CViewbackClient::Connect(const char* pszIP, unsigned short iPort)
{
	CViewbackDataThread::Disconnect();
	m_bDisconnected = false;

	VBPrintf("Connecting to server at %s ...\n", pszIP);

	IN_ADDR address;
	inet_pton(AF_INET, pszIP, &address);
	bool bResult = CViewbackDataThread::Connect(ntohl(address.s_addr), iPort);

	if (bResult)
		VBPrintf("Success.\n");
	else
		VBPrintf("Failed.\n");

	if (!bResult)
		return;

	struct timeb now;
	now.time = 0;
	now.millitm = 0;

	ftime(&now);

	m_iServerConnectionTimeS = now.time;
	m_iServerConnectionTimeMS = now.millitm;
	m_flDataClearTime = 0;
}

void CViewbackClient::FindServer()
{
	vector<CServerListing> server_list = GetServers();
	if (!server_list.size())
		return;

	CViewbackDataThread::Disconnect();
	m_bDisconnected = false;

	in_addr in;
	in.s_addr = htonl(server_list[0].address);

	Connect(inet_ntoa(in), server_list[0].tcp_port);
}

void CViewbackClient::Disconnect()
{
	m_bDisconnected = true;
	CViewbackDataThread::Disconnect();
}

void CViewbackClient::ActivateChannel(size_t iChannel)
{
	char aoeu[10];
	sprintf(aoeu, "%d", iChannel);

	// This list is pumped into the data thread during the Update().
	m_sOutgoingCommands.push_back(std::string("activate: ") + aoeu);

	m_aDataChannels[iChannel].m_bActive = true;
}

void CViewbackClient::DeactivateChannel(size_t iChannel)
{
	char aoeu[10];
	sprintf(aoeu, "%d", iChannel);

	// This list is pumped into the data thread during the Update().
	m_sOutgoingCommands.push_back(std::string("deactivate: ") + aoeu);

	m_aDataChannels[iChannel].m_bActive = false;
}

void CViewbackClient::ActivateGroup(size_t iGroup)
{
	char aoeu[10];
	sprintf(aoeu, "%d", iGroup);

	// This list is pumped into the data thread during the Update().
	m_sOutgoingCommands.push_back(std::string("group: ") + aoeu);

	for (auto& oChannel : m_aDataChannels)
		oChannel.m_bActive = false;

	for (auto& i : m_aDataGroups[iGroup].m_iChannels)
		m_aDataChannels[i].m_bActive = true;
}

void CViewbackClient::ControlCallback(int iControl)
{
	char aoeu[10];
	sprintf(aoeu, "%d", iControl);

	// This list is pumped into the data thread during the Update().
	m_sOutgoingCommands.push_back(std::string("control: ") + aoeu);
}

void CViewbackClient::ControlCallback(int iControl, float value)
{
	char aoeu[100];
	sprintf(aoeu, "control: %d %f", iControl, value);

	// This list is pumped into the data thread during the Update().
	m_sOutgoingCommands.push_back(aoeu);
}

void CViewbackClient::ControlCallback(int iControl, int value)
{
	char aoeu[100];
	sprintf(aoeu, "control: %d %d", iControl, value);

	// This list is pumped into the data thread during the Update().
	m_sOutgoingCommands.push_back(aoeu);
}

void CViewbackClient::SendConsoleCommand(const string& sCommand)
{
	// This list is pumped into the data thread during the Update().
	m_sOutgoingCommands.push_back("console: " + sCommand);
}

vb_data_type_t CViewbackClient::TypeForHandle(size_t iHandle)
{
	return m_aDataChannels[iHandle].m_eDataType;
}

bool CViewbackClient::HasLabel(size_t iHandle, int iValue)
{
	auto it = m_aDataChannels[iHandle].m_asLabels.find(iValue);
	if (it == m_aDataChannels[iHandle].m_asLabels.end())
		return false;
	else
		return true;
}

string CViewbackClient::GetLabelForValue(size_t iHandle, int iValue)
{
	auto it = m_aDataChannels[iHandle].m_asLabels.find(iValue);
	if (it == m_aDataChannels[iHandle].m_asLabels.end())
		return static_cast<ostringstream*>(&(ostringstream() << iValue))->str();
	else
		return it->second;
}

void CViewbackClient::StashData(const Data* pData)
{
	VBAssert(pData->has_time_double() || pData->time_uint64());

	double flTime = 0;

	if (pData->has_time_double())
		flTime = pData->time_double();
	else if (pData->has_time_uint64())
		flTime = ((double)pData->time_uint64()) / 1000;

	double flMaintainTime = 0;

	if (pData->has_maintain_time_double())
		flMaintainTime = pData->maintain_time_double();
	else if (pData->has_maintain_time_uint64())
		flMaintainTime = ((double)pData->maintain_time_uint64()) / 1000;

	switch (TypeForHandle(pData->handle()))
	{
	case VB_DATATYPE_NONE:
	default:
		VBAssert(false);
		break;

	case VB_DATATYPE_INT:
		if (pData->has_maintain_time_double() || pData->has_maintain_time_uint64())
		{
			// We threw out some data to save on network data. Now the client needs to "fake it" by
			// maintaining the previous data value until flMaintainTime, which was the last time that
			// we got from the server of the duplicate value.
			if (m_aData[pData->handle()].m_aIntData.size() && flMaintainTime != m_aData[pData->handle()].m_aIntData.back().time)
				m_aData[pData->handle()].m_aIntData.push_back(CViewbackDataList::DataPair<int>(flMaintainTime, m_aData[pData->handle()].m_aIntData.back().data));
		}

		m_aData[pData->handle()].m_aIntData.push_back(CViewbackDataList::DataPair<int>(flTime, pData->data_int()));
		break;

	case VB_DATATYPE_FLOAT:
		if (pData->has_maintain_time_double() || pData->has_maintain_time_uint64())
		{
			// We threw out some data to save on network data. Now the client needs to "fake it" by
			// maintaining the previous data value until flMaintainTime, which was the last time that
			// we got from the server of the duplicate value.
			if (m_aData[pData->handle()].m_aFloatData.size() && flMaintainTime != m_aData[pData->handle()].m_aFloatData.back().time)
				m_aData[pData->handle()].m_aFloatData.push_back(CViewbackDataList::DataPair<float>(flMaintainTime, m_aData[pData->handle()].m_aFloatData.back().data));
		}

		m_aData[pData->handle()].m_aFloatData.push_back(CViewbackDataList::DataPair<float>(flTime, pData->data_float()));
		break;

	case VB_DATATYPE_VECTOR:
		if (pData->has_maintain_time_double() || pData->has_maintain_time_uint64())
		{
			// We threw out some data to save on network data. Now the client needs to "fake it" by
			// maintaining the previous data value until flMaintainTime, which was the last time that
			// we got from the server of the duplicate value.
			if (m_aData[pData->handle()].m_aVectorData.size() && flMaintainTime != m_aData[pData->handle()].m_aVectorData.back().time)
				m_aData[pData->handle()].m_aVectorData.push_back(CViewbackDataList::DataPair<VBVector3>(flMaintainTime, m_aData[pData->handle()].m_aVectorData.back().data));
		}

		m_aData[pData->handle()].m_aVectorData.push_back(CViewbackDataList::DataPair<VBVector3>(flTime, VBVector3(pData->data_float_x(), pData->data_float_y(), pData->data_float_z())));
		break;
	}

	if (flTime > m_flLatestDataTime)
	{
		m_flLatestDataTime = flTime;

		struct timeb now;
		now.time = 0;
		now.millitm = 0;

		ftime(&now);

		m_flTimeReceivedLatestData = (double)(now.time - m_iServerConnectionTimeS);
		m_flTimeReceivedLatestData += ((double)(now.millitm) - (double)m_iServerConnectionTimeMS)/1000;
	}
}

double CViewbackClient::PredictCurrentTime()
{
	struct timeb now;
	now.time = 0;
	now.millitm = 0;

	ftime(&now);

	double flTimeNow = (double)(now.time - m_iServerConnectionTimeS);
	flTimeNow += ((double)(now.millitm) - (double)m_iServerConnectionTimeMS) / 1000;

	double flTimeDifference = flTimeNow - m_flTimeReceivedLatestData;

	return m_flLatestDataTime + flTimeDifference;
}



