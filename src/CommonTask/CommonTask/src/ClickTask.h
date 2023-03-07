#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CommonType.h"
#include "CommonTaskMacro.h"

class CommonTaskAPI ClickTask : public CTask
{
public:
	ClickTask();

public:
	void DoTask();

	void setParam(MouseType mouseType, ClickType clickType, int32_t sleepTime = -1);

private:
	MouseType m_mouseType;
	ClickType m_clickType;
	int32_t m_sleepTime;
};