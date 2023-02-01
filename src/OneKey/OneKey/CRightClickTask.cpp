#include "stdafx.h"
#include "CRightClickTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

CRightClickTask::CRightClickTask():
m_hasDown(true)
{

}

void CRightClickTask::DoTask()
{
	//Sleep(20);
	int32_t count = 1;
	while (count-- != 0)
	{
		CMouse::RightUp();
		if (m_hasDown)
		{
			CMouse::RightDown();
		}
		//CMouse::RightClick(0);
		//Sleep(50);
	}
}

void CRightClickTask::setParam(bool hasDown)
{
	m_hasDown = hasDown;
}