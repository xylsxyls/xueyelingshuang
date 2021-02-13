#include "LevelTask.h"
#include "CMouse/CMouseAPI.h"

void LevelTask::DoTask()
{
	int32_t index = -1;
	while (index++ != m_level.size() - 1)
	{
		if (m_level[index] == 'q')
		{
			CMouse::MoveAbsolute(xyls::Point(756, 917), 100);
			CMouse::LeftClick();
		}
		else if (m_level[index] == 'w')
		{
			CMouse::MoveAbsolute(xyls::Point(829, 917), 100);
			CMouse::LeftClick();
		}
		else if (m_level[index] == 'e')
		{
			CMouse::MoveAbsolute(xyls::Point(890, 917), 100);
			CMouse::LeftClick();
		}
		else if (m_level[index] == 'r')
		{
			CMouse::MoveAbsolute(xyls::Point(957, 917), 100);
			CMouse::LeftClick();
		}
	}
}

void LevelTask::setParam(const std::string& level)
{
	m_level = level;
}