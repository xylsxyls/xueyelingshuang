#include "JidiTask.h"
#include "GoTask.h"
#include "Rect/RectAPI.h"

extern uint32_t* g_threadId;
extern int32_t g_accountCount;
extern xyls::Rect g_rightTopRect[3];
extern xyls::Point g_clickTop[3];

void JidiTask::DoTask()
{
	Sleep(1500);
	int32_t index = -1;
	while (index++ != g_accountCount - 1)
	{
		std::shared_ptr<GoTask> spGoTask(new GoTask);
		spGoTask->setParam(g_clickTop[index], g_accountCount == 1 ? xyls::Rect(1869, 189, 1920, 249) : g_rightTopRect[index], "¸ÊÈª¹È½ÓÒýÈË", 1);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spGoTask);
	}
}