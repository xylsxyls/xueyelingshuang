#include "ThreadManager.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CreateDataTask.h"
#include "CreateKeyTask.h"

ThreadManager::ThreadManager():
m_receiveThreadId(0),
m_workThreadId(0),
m_deleteKeyThreadId(0),
m_deleteDataThreadId(0),
m_createKeyThreadId(0),
m_createDataThreadId(0)
{
	init();
}

ThreadManager& ThreadManager::instance()
{
	static ThreadManager s_threadManager;
	return s_threadManager;
}

void ThreadManager::init()
{
	m_receiveThreadId = CTaskThreadManager::Instance().Init();
	m_workThreadId = CTaskThreadManager::Instance().Init();
	m_deleteKeyThreadId = CTaskThreadManager::Instance().Init();
	m_deleteDataThreadId = CTaskThreadManager::Instance().Init();
	m_createKeyThreadId = CTaskThreadManager::Instance().Init();
	m_createDataThreadId = CTaskThreadManager::Instance().Init();
}

void ThreadManager::uninit()
{
	CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
	CTaskThreadManager::Instance().Uninit(m_workThreadId);
	CTaskThreadManager::Instance().Uninit(m_deleteKeyThreadId);
	CTaskThreadManager::Instance().Uninit(m_deleteDataThreadId);
	CTaskThreadManager::Instance().Uninit(m_createKeyThreadId);
	CTaskThreadManager::Instance().Uninit(m_createDataThreadId);
}

void ThreadManager::postDeleteKeyTask(const std::shared_ptr<CTask>& spDeleteKeyTask)
{
	CTaskThreadManager::Instance().GetThreadInterface(m_deleteKeyThreadId)->PostTask(spDeleteKeyTask);
}

void ThreadManager::postDeleteDataTask(const std::shared_ptr<CTask>& spDeleteDataTask)
{
	CTaskThreadManager::Instance().GetThreadInterface(m_deleteDataThreadId)->PostTask(spDeleteDataTask);
}

void ThreadManager::postWorkTask(const std::shared_ptr<CTask>& spWorkTask)
{
	CTaskThreadManager::Instance().GetThreadInterface(m_workThreadId)->PostTask(spWorkTask);
}

void ThreadManager::postReceiveTask(const std::shared_ptr<CTask>& spReceiveTask)
{
	CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId)->PostTask(spReceiveTask);
}

void ThreadManager::postCreateKeyTask(const std::shared_ptr<CTask>& spCreateKeyTask)
{
	CTaskThreadManager::Instance().GetThreadInterface(m_createKeyThreadId)->PostTask(spCreateKeyTask);
}

void ThreadManager::postCreateDataTask(const std::shared_ptr<CTask>& spCreateDataTask)
{
	CTaskThreadManager::Instance().GetThreadInterface(m_createDataThreadId)->PostTask(spCreateDataTask);
}