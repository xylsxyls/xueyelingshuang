#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class OpenProcessTask : public CTask
{
public:
	OpenProcessTask();

public:
	void DoTask();

	void setParam(const std::string& path, int32_t findTime = 0, int32_t unfindTime = 0);

private:
	std::string m_path;
	int32_t m_findTime;
	int32_t m_unfindTime;
};