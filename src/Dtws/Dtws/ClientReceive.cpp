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
#include "SubmitTask.h"
#include "EscTask.h"
#include "PlantTask.h"

extern uint32_t* g_taskThreadId;
extern uint32_t* g_threadId;
extern int32_t g_accountCount;
extern bool g_isBigLache;
extern Dtws* g_dtws;
extern xyls::Point g_accept;
extern xyls::Point g_get;

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
			std::vector<xyls::Point> vecAcceptPoint;
			vecAcceptPoint.push_back(g_accept);
			goFindClickTask->setParam(0, clientIndex, "ganquangujieyinren", 0, vecAcceptPoint);
			std::shared_ptr<CTask> spGoFindClickTask(goFindClickTask);
			vecSpDoTask.push_back(spGoFindClickTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_CHANGSHOUGONG:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			GoFindClickTask* goFindClickTask = new GoFindClickTask;
			goFindClickTask->setParam(0, clientIndex, "pingkouzhenren", 0, std::vector<xyls::Point>());
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
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			GoFindClickTask* goFindClickTask = new GoFindClickTask;
			goFindClickTask->setParam(0,
				clientIndex,
				g_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
				g_isBigLache ? 3 : 2,
				std::vector<xyls::Point>());
			std::shared_ptr<CTask> spGoFindClickTask(goFindClickTask);
			vecSpDoTask.push_back(spGoFindClickTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_MUFENGLIN:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			ConvoyTask* convoyTask = new ConvoyTask;
			std::vector<xyls::Point> vecAcceptPoint;
			vecAcceptPoint.push_back(g_get);
			vecAcceptPoint.push_back(g_accept);
			convoyTask->setParam(0,
				clientIndex,
				g_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
				g_isBigLache ? 0 : 0,
				vecAcceptPoint);
			std::shared_ptr<CTask> spConvoyTask(convoyTask);
			vecSpDoTask.push_back(spConvoyTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_HUILUODAO:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			ConvoyTask* convoyTask = new ConvoyTask;
			std::vector<xyls::Point> vecAcceptPoint;
			vecAcceptPoint.push_back(g_get);
			vecAcceptPoint.push_back(g_accept);
			convoyTask->setParam(0,
				clientIndex,
				g_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
				g_isBigLache ? 1 : 1,
				vecAcceptPoint);
			std::shared_ptr<CTask> spConvoyTask(convoyTask);
			vecSpDoTask.push_back(spConvoyTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_LONGMENSHIKU:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			ConvoyTask* convoyTask = new ConvoyTask;
			std::vector<xyls::Point> vecAcceptPoint;
			vecAcceptPoint.push_back(g_get);
			vecAcceptPoint.push_back(g_accept);
			convoyTask->setParam(0,
				clientIndex,
				g_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
				g_isBigLache ? 4 : 3,
				vecAcceptPoint);
			std::shared_ptr<CTask> spConvoyTask(convoyTask);
			vecSpDoTask.push_back(spConvoyTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_DALEIGONG:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			ConvoyTask* convoyTask = new ConvoyTask;
			std::vector<xyls::Point> vecAcceptPoint;
			vecAcceptPoint.push_back(g_accept);
			vecAcceptPoint.push_back(g_accept);
			convoyTask->setParam(0,
				clientIndex,
				g_isBigLache ? "dongbeiyijun" : "pihuoshang",
				g_isBigLache ? 2 : 0,
				vecAcceptPoint);
			std::shared_ptr<CTask> spConvoyTask(convoyTask);
			vecSpDoTask.push_back(spConvoyTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_SUBMIT:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

		std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
		std::vector<std::shared_ptr<CTask>> vecSpDoTask;
		int32_t clientIndex = -1;
		while (clientIndex++ != g_accountCount - 1)
		{
			std::shared_ptr<CTask> spSubmitTask(new SubmitTask);
			vecSpDoTask.push_back(spSubmitTask);
		}
		spAssignThreadTask->setParam(vecSpDoTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	}
	break;
	case DTWS_ESC:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
		std::shared_ptr<EscTask> spEscTask(new EscTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spEscTask);
	}
	break;
	case DTWS_PLANT:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
		std::shared_ptr<PlantTask> spPlantTask(new PlantTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spPlantTask);
	}
	break;
	default:
		break;
	}
}