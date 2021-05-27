#include "ClientReceive.h"
#include "FollowTask.h"
#include "HealTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

extern uint32_t* g_threadId;

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
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
	}
	break;
	case DTWS_FOLLOW:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
		std::shared_ptr<FollowTask> spFollowTask(new FollowTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spFollowTask);
	}
	break;
	case DTWS_HEAL:
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
		std::shared_ptr<HealTask> spHealTask(new HealTask);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spHealTask);
	}
	break;
	default:
		break;
	}
}