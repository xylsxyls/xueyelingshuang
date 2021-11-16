#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CqMoreTask : public CTask
{
public:
	CqMoreTask();

public:
	void DoTask();

	void setParam(bool isW);

private:
	bool m_isW;
};