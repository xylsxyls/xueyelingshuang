#include "JidiTask.h"
#include "GoTask.h"

extern uint32_t* g_threadId;

void JidiTask::DoTask()
{
	Sleep(1500);
	std::shared_ptr<GoTask> spGoTask(new GoTask);
	spGoTask->setParam("¸ÊÈª¹È½ÓÒýÈË", 1);
	CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spGoTask);
}