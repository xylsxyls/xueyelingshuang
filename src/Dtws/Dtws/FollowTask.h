#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class FollowTask : public CTask
{
public:
	FollowTask();

public:
	void DoTask();

	void setParam(bool isHeal);

private:
	bool m_isHeal;
};