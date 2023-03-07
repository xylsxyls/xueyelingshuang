#include "PlantTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"
#include "PlantAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"

void PlantTask::DoTask()
{
	std::shared_ptr<PlantAtomicTask> spTask(new PlantAtomicTask);
	spTask->setAccountIndex(m_accountIndex);
	AssignThreadManager::instance().postTask(spTask);
}

AssignTask* PlantTask::copy()
{
	return new PlantTask;
}