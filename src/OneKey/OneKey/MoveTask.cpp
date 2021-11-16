#include "stdafx.h"
#include "MoveTask.h"
#include "CMouse/CMouseAPI.h"

void MoveTask::DoTask()
{
	CMouse::MoveAbsolute(m_pos, 0);
}

void MoveTask::setParam(const xyls::Point& pos)
{
	m_pos = pos;
}