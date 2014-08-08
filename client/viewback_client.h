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

#pragma once

#include <deque>

#include "../protobuf/data.pb.h"

#include "vector3.h"

#define NOMINMAX

#include "../server/viewback_shared.h"

namespace vb
{

// This quick class automates the cleanup of sockets in case the server creation fails for some reason.
class CCleanupSocket
{
public:
	CCleanupSocket(vb__socket_t socket)
	{
		_socket = socket;
		_success = false;
	}

	~CCleanupSocket()
	{
		if (!_success && vb__socket_valid(_socket))
			vb__socket_close(_socket);
	}

public:
	void Success()
	{
		_success = true;
	}

private:
	vb__socket_t _socket;
	bool         _success;
};

class CViewbackDataChannel
{
public:
	CViewbackDataChannel()
	{
		m_flMin = m_flMax = 0;
		m_bActive = false;
	}

public:
	unsigned int   m_iHandle;
	std::string    m_sName;
	vb_data_type_t m_eDataType;

	float m_flMin;
	float m_flMax;

	// If the channel is active, the server will send data. All channels
	// start as inactive. Changing the status is done with server commands.
	// The server never informs the client of which channels are active or
	// inactive, it's the client's job to keep track of that.
	bool m_bActive;

	std::map<int, std::string> m_asLabels;
};

class CViewbackDataGroup
{
public:
	std::string m_sName;
	std::vector<size_t> m_iChannels;
};

class CViewbackDataControl
{
public:
	std::string  m_name;
	vb_control_t m_type;

	union
	{
		struct
		{
			float range_min;
			float range_max;
			int   steps;
		} slider_float;
	};
};

// Holds all of the data associated with one handle.
class CViewbackDataList
{
public:
	template <typename T>
	class DataPair
	{
	public:
		DataPair(double _time, T _data)
		{
			time = _time;
			data = _data;
		}

	public:
		double time;
		T      data;
	};

public:
	// Only one of these will be used at a time.
	std::deque<DataPair<int>>       m_aIntData;
	std::deque<DataPair<float>>     m_aFloatData;
	std::deque<DataPair<VBVector3>> m_aVectorData;
};

// This data may be initialized by the server, but after that can be edited
// by the monitor as per user preferences.
class CDataMetaInfo
{
public:
	CDataMetaInfo()
	{
		m_vecMaxValue = VBVector3(0, 0, 0);
		m_clrColor = VBVector3(1, 1, 1);
		m_flDisplayDuration = 1;
		m_bVisible = true;
	}

public:
	VBVector3 m_vecMaxValue;
	VBVector3 m_clrColor;
	float     m_flDisplayDuration; // In the 2D view, how many seconds worth of data should the monitor show?
	bool      m_bVisible;
};

class CServerListing
{
public:
	std::string    name;
	unsigned long  address;
	unsigned short tcp_port;
	time_t         last_ping;
};

typedef void(*ConsoleOutputCallback)(const char*);
typedef void(*DebugOutputCallback)(const char*);
typedef void(*RegistrationUpdateCallback)();

class CViewbackClient
{
public:
	// Note that on Android you need to explicitly enable Multicast over WiFi.
	// http://anandtechblog.blogspot.com.es/2011/11/multicast-udp-reciever-in-android.html
	// http://codeisland.org/2012/udp-multicast-on-android/
	// Viewback will not do this for you.
	bool Initialize(RegistrationUpdateCallback pfnRegistration, ConsoleOutputCallback pfnConsoleOutput, DebugOutputCallback pfnDebugOutput = NULL);
	void Shutdown();

	void Update();

	std::vector<CServerListing> GetServers();

	bool HasConnection();
	void Connect(const char* pszIP, unsigned short iPort); // Does not resolve hostnames, pass an IP.
	void FindServer(); // Connect to the first server you can find by multicast.
	void Disconnect();

	// Deactivated channels will not be sent to the client.
	// All channels are deactivated by default.
	void ActivateChannel(size_t iChannel);
	void DeactivateChannel(size_t iChannel);
	void ActivateGroup(size_t iGroup);

	void ControlCallback(int iControl);
	void ControlCallback(int iControl, float);

	void SendConsoleCommand(const std::string& sCommand);
	DebugOutputCallback GetDebugOutputCallback() { return m_pfnDebugOutput; }

	inline const std::vector<CViewbackDataChannel>& GetChannels() const { return m_aDataChannels; }
	inline const std::vector<CViewbackDataGroup>& GetGroups() const { return m_aDataGroups; }
	inline const std::vector<CViewbackDataControl>& GetControls() const { return m_aDataControls; }
	inline const std::vector<CViewbackDataList>& GetData() const { return m_aData; } // DO NOT STORE without copying, this may be wiped at any time.
	inline std::vector<CDataMetaInfo>& GetMeta() { return m_aMeta; }

	vb_data_type_t TypeForHandle(size_t iHandle);

	bool HasLabel(size_t iHandle, int iValue);
	std::string GetLabelForValue(size_t iHandle, int iValue);

	std::string GetStatus() { return m_sStatus; }

	double GetLatestDataTime() { return m_flLatestDataTime; }
	double PredictCurrentTime();

	// Time view data (type float or int) with time stamps before this time may be cleared from memory.
	// It's important to update this periodically or else old data will never be deleted.
	void SetDataClearTime(double flTime) { m_flDataClearTime = flTime; }

private:
	void StashData(const Data* pData);

private:
	std::vector<Packet> m_aUnhandledMessages;

	std::vector<CViewbackDataChannel> m_aDataChannels;
	std::vector<CViewbackDataGroup> m_aDataGroups;
	std::vector<CViewbackDataControl> m_aDataControls;
	std::vector<CViewbackDataList> m_aData;
	std::vector<CDataMetaInfo> m_aMeta;

	std::deque<std::string> m_sOutgoingCommands;

	RegistrationUpdateCallback m_pfnRegistrationUpdate;
	ConsoleOutputCallback      m_pfnConsoleOutput;
	DebugOutputCallback        m_pfnDebugOutput;

	std::string m_sStatus;

	time_t         m_iServerConnectionTimeS;
	unsigned short m_iServerConnectionTimeMS;

	double m_flNextDataClear;
	double m_flLatestDataTime;
	double m_flTimeReceivedLatestData;

	double m_flDataClearTime;

	bool m_bDisconnected; // Remain disconnected while this is on.
};

}
