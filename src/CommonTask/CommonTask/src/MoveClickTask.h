#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Rect/RectAPI.h"
#include "CommonType.h"
#include "CommonTaskMacro.h"

class CommonTaskAPI MoveClickTask : public CTask
{
public:
	MoveClickTask();

public:
	void DoTask();

	void setParam(const xyls::Rect& pos, int32_t moveTime = 0);

	void setParam(MouseType mouseType, ClickType clickType, int32_t sleepTime = -1);

private:
	xyls::Rect m_pos;
	int32_t m_moveTime;
	MouseType m_mouseType;
	ClickType m_clickType;
	int32_t m_sleepTime;
};