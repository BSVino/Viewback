#pragma once

#include "../protobuf/data.pb.h"

#include "vector3.h"

#include <deque>

class CViewbackDataRegistration
{
public:
	unsigned int   m_iHandle;
	std::string    m_sFieldName;
	vb_data_type_t m_eDataType;

	std::map<int, std::string> m_asLabels;
};

// Holds all of the data associated with one handle.
class CViewbackDataList
{
public:
	// Only one of these will be used at a time.
	std::vector<int>       m_aIntData;
	std::vector<float>     m_aFloatData;
	std::vector<VBVector3> m_aVectorData;
};

typedef void(*ConsoleOutputCallback)(const char*);

class CViewbackClient
{
public:
	bool Initialize(ConsoleOutputCallback pfnConsoleOutput);

	void Update();

	bool HasConnection();

	void SendConsoleCommand(const std::string& sCommand);

	inline const std::vector<CViewbackDataRegistration>& GetRegistrations() const { return m_aDataRegistrations; }
	inline const std::vector<CViewbackDataList>& GetData() const { return m_aData; }

	vb_data_type_t TypeForHandle(size_t iHandle);

	bool HasLabel(size_t iHandle, int iValue);
	std::string GetLabelForValue(size_t iHandle, int iValue);

private:
	void StashData(const Data* pData);

private:
	std::vector<Packet> m_aUnhandledMessages;

	std::vector<CViewbackDataRegistration> m_aDataRegistrations;
	std::vector<CViewbackDataList> m_aData;

	std::deque<std::string> m_sOutgoingCommands;

	ConsoleOutputCallback m_pfnConsoleOutput;
};
