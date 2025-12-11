#include "CollectTask.h"
#ifndef _WIN64
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#endif
#include "Point/PointAPI.h"

CollectTask::CollectTask():
m_count(0),
m_exit(false)
{

}

void CollectTask::DoTask()
{
	std::vector<xyls::Point> vec = { { 1046, 742 }, { 1076, 990 }, { 1078, 649 }, { 1072, 601 } };
	while (!m_exit && (m_count-- != 0))
	{
#ifndef _WIN64
		CKeyboard::KeyPress(CKeyboard::PageUp, 0);
#endif
		Sleep(800);
		size_t index = -1;
		while (!m_exit && (index++ != vec.size() - 1))
		{
#ifndef _WIN64
			CMouse::MoveClick(vec[index], 0);
#endif
			Sleep(800);
		}
	}
}

void CollectTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void CollectTask::setParam(int32_t count)
{
	m_count = count;
}

void CollectTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}
