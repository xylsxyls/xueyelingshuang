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
#include "Config.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"
#include "AssignThreadHelper.h"
#include "GoDestTask.h"
#include "ConvoyDestTask.h"
#include "RaffleTask.h"

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
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
	}
	break;
	case DTWS_FOLLOW:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		std::shared_ptr<FollowTask> spTask(new FollowTask);
		AssignThreadHelper::postEveryAssignTask(spTask);
	}	
	break;
	case DTWS_HEAL:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		std::shared_ptr<HealTask> spHealTask(new HealTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spHealTask);
	}
	break;
	case DTWS_RISE:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		std::shared_ptr<RiseTask> spRiseTask(new RiseTask);
		AssignThreadHelper::postEveryAssignTask(spRiseTask);
	}
	break;
	case DTWS_ATTACK:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		std::shared_ptr<AttackTask> spAttackTask(new AttackTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spAttackTask);
	}
	break;
	case DTWS_JIDI:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();

		std::vector<std::shared_ptr<CTask>> vecTask;
		int32_t accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			std::shared_ptr<GoDestTask> spAssignTask(new GoDestTask);
			spAssignTask->setAccountIndex(accountIndex);
			spAssignTask->setParam("ganquangujieyinren", 0, true);
			vecTask.push_back(spAssignTask);
		}
		AssignThreadManager::instance().postAssignTask(vecTask);
	}
	break;
	case DTWS_CHANGSHOUGONG:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();

		std::vector<std::shared_ptr<CTask>> vecTask;
		int32_t accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			std::shared_ptr<GoDestTask> spAssignTask(new GoDestTask);
			spAssignTask->setAccountIndex(accountIndex);
			spAssignTask->setParam("pingkouzhenren", 0, false);
			vecTask.push_back(spAssignTask);
		}
		AssignThreadManager::instance().postAssignTask(vecTask);
	}
	break;
	case DTWS_DACHE:
	{
		g_config.m_isBigLache = true;
		emit g_config.m_dtws->changeLacheText();
	}
	break;
	case DTWS_XIAOCHE:
	{
		g_config.m_isBigLache = false;
		emit g_config.m_dtws->changeLacheText();
	}
	break;
	case DTWS_BINGJIANCUN:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();

		std::vector<std::shared_ptr<CTask>> vecTask;
		int32_t accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			std::shared_ptr<GoDestTask> spAssignTask(new GoDestTask);
			spAssignTask->setAccountIndex(accountIndex);
			spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
				g_config.m_isBigLache ? 3 : 2, false);
			vecTask.push_back(spAssignTask);
		}
		AssignThreadManager::instance().postAssignTask(vecTask);
	}
	break;
	case DTWS_MUFENGLIN:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();

		std::vector<std::shared_ptr<CTask>> vecTask;
		int32_t accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			std::shared_ptr<ConvoyDestTask> spAssignTask(new ConvoyDestTask);
			spAssignTask->setAccountIndex(accountIndex);
			spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
				g_config.m_isBigLache ? 0 : 0);
			vecTask.push_back(spAssignTask);
		}
		AssignThreadManager::instance().postAssignTask(vecTask);
	}
	break;
	case DTWS_HUILUODAO:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();

		std::vector<std::shared_ptr<CTask>> vecTask;
		int32_t accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			std::shared_ptr<ConvoyDestTask> spAssignTask(new ConvoyDestTask);
			spAssignTask->setAccountIndex(accountIndex);
			spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
				g_config.m_isBigLache ? 1 : 1);
			vecTask.push_back(spAssignTask);
		}
		AssignThreadManager::instance().postAssignTask(vecTask);
	}
	break;
	case DTWS_LONGMENSHIKU:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();

		std::vector<std::shared_ptr<CTask>> vecTask;
		int32_t accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			std::shared_ptr<ConvoyDestTask> spAssignTask(new ConvoyDestTask);
			spAssignTask->setAccountIndex(accountIndex);
			spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
				g_config.m_isBigLache ? 4 : 3);
			vecTask.push_back(spAssignTask);
		}
		AssignThreadManager::instance().postAssignTask(vecTask);
	}
	break;
	case DTWS_DALEIGONG:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();

		std::vector<std::shared_ptr<CTask>> vecTask;
		int32_t accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			std::shared_ptr<ConvoyDestTask> spAssignTask(new ConvoyDestTask);
			spAssignTask->setAccountIndex(accountIndex);
			spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "pihuoshang",
				g_config.m_isBigLache ? 2 : 0);
			vecTask.push_back(spAssignTask);
		}
		AssignThreadManager::instance().postAssignTask(vecTask);
	}
	break;
	case DTWS_SUBMIT:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		std::shared_ptr<SubmitTask> spTask(new SubmitTask);
		AssignThreadHelper::postEveryAssignTask(spTask);
	}
	break;
	case DTWS_SUBMIT_PLUS:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		std::shared_ptr<SubmitTask> spTask(new SubmitTask);
		spTask->setParam(1);
		AssignThreadHelper::postEveryAssignTask(spTask);
	}
	break;
	case DTWS_ESC:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		std::shared_ptr<EscTask> spTask(new EscTask);
		AssignThreadHelper::postEveryAssignTask(spTask);
	}
	break;
	case DTWS_PLANT:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		std::shared_ptr<PlantTask> spTask(new PlantTask);
		AssignThreadHelper::postEveryAssignTask(spTask);
	}
	break;
	case DTWS_RAFFLE:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		std::shared_ptr<RaffleTask> spTask(new RaffleTask);
		AssignThreadHelper::postEveryAssignTask(spTask);
	}
	break;
	default:
		break;
	}
}