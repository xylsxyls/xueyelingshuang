#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class LevelTask : public CTask
{
public:
	void DoTask();

	void setParam(const std::string& level);

private:
	std::string m_level;
};