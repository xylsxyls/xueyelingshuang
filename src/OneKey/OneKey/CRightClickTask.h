#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CRightClickTask : public CTask
{
public:
	CRightClickTask();

public:
	void DoTask();

	void setParam(bool hasDown);

private:
	bool m_hasDown;
};