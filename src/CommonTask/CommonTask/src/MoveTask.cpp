#include "MoveTask.h"
#include "CMouse/CMouseAPI.h"

MoveTask::MoveTask():
m_moveTime(0)
{

}

void MoveTask::DoTask()
{
	CMouse::MoveAbsolute(m_pos, m_moveTime);
}

void MoveTask::setParam(const xyls::Rect& pos, int32_t moveTime)
{
	m_pos = pos;
	m_moveTime = moveTime;
}