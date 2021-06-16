#include "ClientReceive.h"
#include "FollowTask.h"
#include "HealTask.h"
#include "RiseTask.h"
#include "AttackTask.h"
#include "JidiTask.h"
#include "AssignThreadTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

extern uint32_t* g_taskThreadId;
extern uint32_t* g_threadId;
extern int32_t g_accountCount;

ClientReceive::ClientReceive()
{

}

void ClientReceive::clientInitResponse(int32_t serverId, const char* buffer, int32_t length)
{
	RCSend("CLIENT_INIT_RESPONSE serverId = %d, buffer = %s", serverId, buffer);
}

void ClientReceive::ServerMessage(int32_t serverId, const char* buffer, int32_t length)
{
	RCSend("serverId = %d, buffer = %s, length = %d", serverId, buffer, length);
	switch (atoi(buffer))
	{
	case DTWS_STOP:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
	}
	break;
	case DTWS_FOLLOW:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
		std::shared_ptr<FollowTask> spFollowTask(new FollowTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spFollowTask);
	}
	break;
	case DTWS_HEAL:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
		std::shared_ptr<HealTask> spHealTask(new HealTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spHealTask);
	}
	break;
	case DTWS_RISE:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
		std::shared_ptr<RiseTask> spRiseTask(new RiseTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spRiseTask);
	}
	break;
	case DTWS_ATTACK:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
		std::shared_ptr<AttackTask> spAttackTask(new AttackTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spAttackTask);
	}
	break;
	case DTWS_JIDI:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t index = -1;
		while (index++ != g_accountCount - 1)
		{
			JidiTask* jidiTask = new JidiTask;
			jidiTask->setParam(0, index);
			std::shared_ptr<CTask> spJidiTask(jidiTask);
			vecSpDoTask.push_back(spJidiTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	default:
		break;
	}
}