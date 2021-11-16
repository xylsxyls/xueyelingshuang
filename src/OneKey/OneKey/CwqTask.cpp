#include "stdafx.h"
#include "CwqTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CKeyTask.h"
#include "CSleepTask.h"

extern uint32_t g_threadId;

CwqTask::CwqTask():
m_isR(false)
{

}

void CwqTask::DoTask()
{
	if (m_isR)
	{
		KeyPress('W');
		Sleep(150);
		KeyPress('E');
		Sleep(150);
		KeyPress('Q');
		Sleep(300);
		KeyPress('E');
	}
	else
	{
		KeyPress('E');
		Sleep(250);
		KeyPress('W');
		Sleep(150);
		KeyPress('E');
	}
}

void CwqTask::setParam(bool isR)
{
	m_isR = isR;
}

void CwqTask::KeyPress(int32_t vkCode)
{
	{
		std::shared_ptr<CKeyTask> spKeyTask(new CKeyTask);
		spKeyTask->setParam(vkCode, true);
		CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->PostTask(spKeyTask);
	}

	{
		std::shared_ptr<CKeyTask> spKeyTask(new CKeyTask);
		spKeyTask->setParam(vkCode, false);
		CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->PostTask(spKeyTask);
	}
}

void CwqTask::Sleep(int32_t sleepTime)
{
	std::shared_ptr<CSleepTask> spSleepTask(new CSleepTask);
	spSleepTask->setParam(sleepTime);
	CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->PostTask(spSleepTask);
}