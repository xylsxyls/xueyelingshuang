#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class SayTask : public CTask
{
public:
	void DoTask();

	void setParam(const std::string& say);

private:
	std::string m_say;
};