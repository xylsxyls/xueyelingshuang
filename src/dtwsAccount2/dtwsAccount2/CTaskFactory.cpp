#include "CTaskFactory.h"
#include "dtwsAccount2.h"
#include "ConfigInfo.h"

void CLoginTask::DoTask()
{
	auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(WORK_THREAD);
	int32_t taskLevel = taskThread->GetCurTaskLevel();
	std::shared_ptr<CTask> spTask;
	spTask.reset(new CClickPicTask("", GetConfig(RECT_NINE_1, xyls::Rect)));
	taskThread->PostTask(spTask, taskLevel + 1);
}

xyls::Point& CFindPicTask::GetPoint()
{
	return m_pointResult;
}

CMoveMouseTask::CMoveMouseTask(const xyls::Point& point) :
m_point(point)
{

}

CClickPicTask::CClickPicTask(const std::string& picPath, const xyls::Rect& rect) :
m_picPath(picPath),
m_picRect(rect)
{
	CFindPicTask* pFindPicTask = new CFindPicTask;
	std::shared_ptr<CTask> spTask;
	spTask.reset(pFindPicTask);
	m_taskList.push_back(spTask);
	spTask.reset(new CMoveMouseTask(pFindPicTask->GetPoint()));
	m_taskList.push_back(spTask);
	spTask.reset(new CClickTask);
	m_taskList.push_back(spTask);
}

void CClickPicTask::DoTask()
{
	//底层思路，在确定大任务的基础上轮番更换小任物到m_spCurTask中，当出现顶掉时将大任务停掉
	for (auto itTask = m_taskList.begin(); itTask != m_taskList.end(); ++itTask)
	{
		if (m_hasExitSignal)
		{
			return;
		}
		(*itTask)->DoTask();
	}
}