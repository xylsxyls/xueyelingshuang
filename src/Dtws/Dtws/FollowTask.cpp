#include "FollowTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "Config.h"
#include "ClickAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"
#include "SelectTeamerAtomicTask.h"

void FollowTask::DoTask()
{
	std::shared_ptr<SelectTeamerAtomicTask> spTask;
	do 
	{
		spTask.reset(new SelectTeamerAtomicTask);
		spTask->setAccountIndex(m_accountIndex);
		spTask->heroBlood(true, false);
		spTask->heroBlood(false, true);
		AssignThreadManager::instance().postTask(spTask);
		spTask->waitForEnd();
	} while (spTask->isReDo() && !m_exit);
}

AssignTask* FollowTask::copy()
{
	return new FollowTask;
}