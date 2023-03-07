#include "RiseTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"
#include "RiseAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

void RiseTask::DoTask()
{
	std::shared_ptr<RiseAtomicTask> spTask(new RiseAtomicTask);
	spTask->setAccountIndex(m_accountIndex);
	AssignThreadManager::instance().postTask(spTask);
}

AssignTask* RiseTask::copy()
{
	return new RiseTask;
}