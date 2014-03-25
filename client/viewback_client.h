#pragma once

#include "../protobuf/data.pb.h"

#include "vector3.h"

#include <deque>

class CViewbackDataRegistration
{
public:
	CViewbackDataRegistration()
	{
		m_flMin = m_flMax = 0;
	}

public:
	unsigned int   m_iHandle;
	std::string    m_sFieldName;
	vb_data_type_t m_eDataType;
	float          m_flMin;
	float          m_flMax;

	std::map<int, std::string> m_asLabels;
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
	}

public:
	VBVector3 m_vecMaxValue;
	VBVector3 m_clrColor;
	float     m_flDisplayDuration; // In the 2D view, how many seconds worth of data should the monitor show?
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

	void Update();

	bool HasConnection();
	void Connect(const char* pszIP, int iPort); // Does not resolve hostnames, pass an IP.
	void FindServer(); // Connect to the first server you can find by multicast.
	void Disconnect();

	void SendConsoleCommand(const std::string& sCommand);
	DebugOutputCallback GetDebugOutputCallback() { return m_pfnDebugOutput; }

	inline const std::vector<CViewbackDataRegistration>& GetRegistrations() const { return m_aDataRegistrations; }
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

	std::vector<CViewbackDataRegistration> m_aDataRegistrations;
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
