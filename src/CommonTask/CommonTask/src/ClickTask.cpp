#include "ClickTask.h"
#include "CMouse/CMouseAPI.h"

ClickTask::ClickTask():
m_mouseType(MOUSE_TYPE_INIT),
m_clickType(CLICK_TYPE_INIT)
{

}

void ClickTask::DoTask()
{
	if (m_mouseType == MOUSE_LEFT)
	{
		if (m_clickType == MOUSE_DOWN)
		{
			CMouse::LeftDown();
		}
		else if (m_clickType == MOUSE_UP)
		{
			CMouse::LeftUp();
		}
		else if (m_clickType == MOUSE_CLICK)
		{
			CMouse::LeftClick(m_sleepTime);
		}
		else if (m_clickType == MOUSE_DOUBLECLICK)
		{
			CMouse::LeftDoubleClick(m_sleepTime);
		}
	}
	else if (m_mouseType == MOUSE_RIGHT)
	{
		if (m_clickType == MOUSE_DOWN)
		{
			CMouse::RightDown();
		}
		else if (m_clickType == MOUSE_UP)
		{
			CMouse::RightUp();
		}
		else if (m_clickType == MOUSE_CLICK)
		{
			CMouse::RightClick(m_sleepTime);
		}
		else if (m_clickType == MOUSE_DOUBLECLICK)
		{
			CMouse::RightDoubleClick(m_sleepTime);
		}
	}
	else if (m_mouseType == MOUSE_MIDDLE)
	{
		if (m_clickType == MOUSE_DOWN)
		{
			
		}
		else if (m_clickType == MOUSE_UP)
		{

		}
		else if (m_clickType == MOUSE_CLICK)
		{
			CMouse::MiddleClick();
		}
		else if (m_clickType == MOUSE_DOUBLECLICK)
		{
			CMouse::MiddleClick();
			::Sleep(m_sleepTime);
			CMouse::MiddleClick();
		}
	}
}

void ClickTask::setParam(MouseType mouseType, ClickType clickType, int32_t sleepTime)
{
	m_mouseType = mouseType;
	m_clickType = clickType;
	m_sleepTime = sleepTime;
}