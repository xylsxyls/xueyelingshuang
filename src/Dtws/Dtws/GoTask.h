#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class GoTask : public CTask
{
public:
	GoTask();

public:
	void DoTask();

	void setParam(const std::string& placeName, int32_t index);

private:
	std::string m_placeName;
	int32_t m_index;
};