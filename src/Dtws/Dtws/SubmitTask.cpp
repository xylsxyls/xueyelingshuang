#include "SubmitTask.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"
#include "Config.h"
#include "SubmitAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

SubmitTask::SubmitTask():
m_lineIndex(0)
{

}

void SubmitTask::DoTask()
{
	std::shared_ptr<SubmitAtomicTask> spTask(new SubmitAtomicTask);
	spTask->setAccountIndex(m_accountIndex);
	spTask->setParam(m_lineIndex);
	AssignThreadManager::instance().postTask(spTask);
}

void SubmitTask::setParam(int32_t lineIndex)
{
	m_lineIndex = lineIndex;
}

AssignTask* SubmitTask::copy()
{
	return new SubmitTask;
}