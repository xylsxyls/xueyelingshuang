#include "SubmitTask.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"
#include "Config.h"
#include "SubmitAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

void SubmitTask::DoTask()
{
	std::shared_ptr<SubmitAtomicTask> spTask(new SubmitAtomicTask);
	spTask->setAccountIndex(m_accountIndex);
	AssignThreadManager::instance().postTask(spTask);
}

AssignTask* SubmitTask::copy()
{
	return new SubmitTask;
}