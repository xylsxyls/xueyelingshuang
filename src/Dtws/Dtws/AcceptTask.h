#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class AcceptTask : public CTask
{
public:
	AcceptTask();

public:
	void DoTask();

	void setParam(const std::string& findPic, int32_t times);

private:
	std::string m_findPic;
	int32_t m_times;
};