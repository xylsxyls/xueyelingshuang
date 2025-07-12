#include "ConfirmTask.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "DtwsParam.h"
#include "Config.h"
#include "ConfirmAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

void ConfirmTask::DoTask()
{
	std::shared_ptr<ConfirmAtomicTask> spTask(new ConfirmAtomicTask);
	spTask->setConfirmParam(m_vecConfirm);
	spTask->setAccountIndex(m_accountIndex);
	AssignThreadManager::instance().postTask(spTask);
}

AssignTask* ConfirmTask::copy()
{
	return new ConfirmTask;
}

void ConfirmTask::setConfirmParam(const std::vector<xyls::Point>& vecConfirm)
{
	m_vecConfirm = vecConfirm;
}