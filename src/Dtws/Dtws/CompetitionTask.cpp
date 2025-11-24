#include "CompetitionTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"
#include "CompetitionAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

void CompetitionTask::DoTask()
{
	std::shared_ptr<CompetitionAtomicTask> spTask(new CompetitionAtomicTask);
	spTask->setAccountIndex(m_accountIndex);
	AssignThreadManager::instance().postTask(spTask);
}

AssignTask* CompetitionTask::copy()
{
	return new CompetitionTask;
}