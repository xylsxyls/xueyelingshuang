#include "AssignThreadHelper.h"
#include "Config.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

void AssignThreadHelper::postEveryAssignTask(const std::shared_ptr<AssignTask>& spTask)
{
	std::vector<std::shared_ptr<CTask>> vecTask;
	int32_t accountIndex = g_config.m_accountCount;
	while (accountIndex-- != 0)
	{
		if (accountIndex != 0)
		{
			std::shared_ptr<AssignTask> spAssignTask(spTask->copy());
			spAssignTask->setAccountIndex(accountIndex);
			vecTask.push_back(spAssignTask);
		}
		else
		{
			spTask->setAccountIndex(0);
			vecTask.push_back(spTask);
		}
	}
	AssignThreadManager::instance().postAssignTask(vecTask);
}