#include "ProcessMutexManager.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "ProcessHelper.h"

ProcessMutexManager::ProcessMutexManager()
{

}

void ProcessMutexManager::uninit()
{
	for (auto itMutex = m_positionMutex.begin(); itMutex != m_positionMutex.end(); ++itMutex)
	{
		delete itMutex->second;
	}
	m_positionMutex.clear();
	m_positionMutex.swap(std::map<int32_t, ProcessReadWriteMutex*>());
}

ProcessMutexManager& ProcessMutexManager::instance()
{
	static ProcessMutexManager s_processMutexManager;
	return s_processMutexManager;
}

ProcessReadWriteMutex* ProcessMutexManager::positionMutex(int32_t pid)
{
	ProcessReadWriteMutex* result = nullptr;
	auto itMutex = m_positionMutex.find(pid);
	if (itMutex == m_positionMutex.end())
	{
		result = new ProcessReadWriteMutex(ProcessHelper::positionMutexName(pid));
		m_positionMutex[pid] = result;
		return result;
	}
	return itMutex->second;
}