#include "stdafx.h"
#include "MoveTask.h"
#include "CMouse/CMouseAPI.h"

MoveTask::MoveTask():
m_time(0)
{

}

void MoveTask::DoTask()
{
	CMouse::MoveAbsolute(m_pos, m_time);
}

void MoveTask::setParam(const xyls::Point& pos, int32_t time)
{
	m_pos = pos;
	m_time = time;
}