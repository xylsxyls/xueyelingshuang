#include "WaterTask.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "DtwsParam.h"
#include "Config.h"
#include "TalkHeroAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

void WaterTask::DoTask()
{
	Sleep(1000);
	while (!m_exit)
	{
		std::shared_ptr<TalkHeroAtomicTask> spTask(new TalkHeroAtomicTask);
		spTask->setAccountIndex(m_accountIndex);
		AssignThreadManager::instance().postTask(spTask);
		Sleep(10500);
	}
}

AssignTask* WaterTask::copy()
{
	return new WaterTask;
}