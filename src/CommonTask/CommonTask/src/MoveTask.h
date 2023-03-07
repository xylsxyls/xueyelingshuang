#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Rect/RectAPI.h"
#include "CommonTaskMacro.h"

class CommonTaskAPI MoveTask : public CTask
{
public:
	MoveTask();

public:
	void DoTask();

	void setParam(const xyls::Rect& pos, int32_t moveTime = 0);

private:
	xyls::Rect m_pos;
	int32_t m_moveTime;
};