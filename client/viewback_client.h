#pragma once

#include "../protobuf/data.pb.h"

class CViewbackClient
{
public:
	bool Initialize();

	void Update();

private:
	std::vector<Packet> m_aMessages;
};
