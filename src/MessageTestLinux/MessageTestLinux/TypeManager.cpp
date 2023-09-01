#include "TypeManager.h"
#include "CSystem/CSystemAPI.h"

TypeManager& TypeManager::instance()
{
	static TypeManager s_typeManager;
	return s_typeManager;
}

void TypeManager::sendPeopleId(int32_t peopleId)
{
	Q_EMIT receivePeopleId(peopleId);
}

void TypeManager::sendPid(int32_t pid)
{
	Q_EMIT receivePid(pid);
}

void TypeManager::sendThreadId(int32_t threadId)
{
	Q_EMIT receiveThreadId(threadId);
}

void TypeManager::sendPidThreadId(int32_t pid, int32_t threadId)
{
	Q_EMIT receivePidThreadId(pid, threadId);
}

void TypeManager::sendLostPid(int32_t pid)
{
	Q_EMIT receiveLostPid(pid);
}