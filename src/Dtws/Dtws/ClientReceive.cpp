#include "ClientReceive.h"
#include "FollowTask.h"
#include "HealTask.h"
#include "RiseTask.h"
#include "AttackTask.h"
#include "AssignThreadTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ConvoyTask.h"
#include "Dtws.h"
#include "GoFindClickTask.h"

extern uint32_t* g_taskThreadId;
extern uint32_t* g_threadId;
extern int32_t g_accountCount;
extern bool g_isBigLache;
extern Dtws* g_dtws;

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
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			GoFindClickTask* goFindClickTask = new GoFindClickTask;
			goFindClickTask->setParam(0, clientIndex, "甘泉谷接引人", 0, 1);
			std::shared_ptr<CTask> spGoFindClickTask(goFindClickTask);
			vecSpDoTask.push_back(spGoFindClickTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_CHANGSHOUGONG:
	{
		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			GoFindClickTask* goFindClickTask = new GoFindClickTask;
			goFindClickTask->setParam(0, clientIndex, "平寇真人", 0, 0);
			std::shared_ptr<CTask> spGoFindClickTask(goFindClickTask);
			vecSpDoTask.push_back(spGoFindClickTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_DACHE:
	{
		g_isBigLache = true;
		emit g_dtws->changeLacheText();
	}
	break;
	case DTWS_XIAOCHE:
	{
		g_isBigLache = false;
		emit g_dtws->changeLacheText();
	}
	break;
	case DTWS_BINGJIANCUN:
	{
		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			GoFindClickTask* goFindClickTask = new GoFindClickTask;
			goFindClickTask->setParam(0,
				clientIndex,
				g_isBigLache ? "东北义军" : "冰雷镖局",
				g_isBigLache ? 3 : 2,
				0);
			std::shared_ptr<CTask> spGoFindClickTask(goFindClickTask);
			vecSpDoTask.push_back(spGoFindClickTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_MUFENGLIN:
	{
		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			ConvoyTask* convoyTask = new ConvoyTask;
			convoyTask->setParam(0,
				clientIndex,
				g_isBigLache ? "东北义军" : "冰雷镖局",
				g_isBigLache ? 0 : 0,
				2);
			std::shared_ptr<CTask> spConvoyTask(convoyTask);
			vecSpDoTask.push_back(spConvoyTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_HUILUODAO:
	{
		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			ConvoyTask* convoyTask = new ConvoyTask;
			convoyTask->setParam(0,
				clientIndex,
				g_isBigLache ? "东北义军" : "冰雷镖局",
				g_isBigLache ? 1 : 1,
				2);
			std::shared_ptr<CTask> spConvoyTask(convoyTask);
			vecSpDoTask.push_back(spConvoyTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_LONGMENSHIKU:
	{
		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			ConvoyTask* convoyTask = new ConvoyTask;
			convoyTask->setParam(0,
				clientIndex,
				g_isBigLache ? "东北义军" : "冰雷镖局",
				g_isBigLache ? 4 : 3,
				2);
			std::shared_ptr<CTask> spConvoyTask(convoyTask);
			vecSpDoTask.push_back(spConvoyTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_DALEIGONG:
	{
		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			ConvoyTask* convoyTask = new ConvoyTask;
			convoyTask->setParam(0,
				clientIndex,
				g_isBigLache ? "东北义军" : "皮货商",
				g_isBigLache ? 2 : 0,
				2);
			std::shared_ptr<CTask> spConvoyTask(convoyTask);
			vecSpDoTask.push_back(spConvoyTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	default:
		break;
	}
}