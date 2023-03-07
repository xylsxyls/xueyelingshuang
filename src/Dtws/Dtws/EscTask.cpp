#include "EscTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"
#include "EscAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

void EscTask::DoTask()
{
	std::shared_ptr<EscAtomicTask> spTask(new EscAtomicTask);
	spTask->setAccountIndex(m_accountIndex);
	AssignThreadManager::instance().postTask(spTask);
}

AssignTask* EscTask::copy()
{
	return new EscTask;
}