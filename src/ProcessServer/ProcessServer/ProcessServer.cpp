#include "ProcessServer.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessHelper.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "KeyPackage.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "AssignTask.h"
#include "CreateDataTask.h"
#include "CreateKeyTask.h"
#include "DeleteDataTask.h"

ProcessServer::ProcessServer():
m_position(nullptr),
m_data(nullptr),
m_key(nullptr),
m_assignThreadId(0),
m_createDataThreadId(0),
m_createKeyThreadId(0),
m_deleteDataThreadId(0)
{
	m_assignThreadId = CTaskThreadManager::Instance().Init();
	m_createDataThreadId = CTaskThreadManager::Instance().Init();
	m_createKeyThreadId = CTaskThreadManager::Instance().Init();
	m_deleteDataThreadId = CTaskThreadManager::Instance().Init();
	m_position = new SharedMemory(ProcessHelper::positionMapName(), ProcessHelper::positionLength());
	m_position->clear();
	m_data = new SharedMemory(ProcessHelper::dataMapName(m_position->writeWithoutLock()), ProcessHelper::dataMemoryLength());
	m_key = new SharedMemory(ProcessHelper::keyMapName(m_position->writeWithoutLock()), ProcessHelper::keyMemoryLength());
}

void ProcessServer::listen()
{
	DeleteDataTask* deleteDataTask = new DeleteDataTask;
	deleteDataTask->setServer(this);
	std::shared_ptr<DeleteDataTask> spDeleteDataTask;
	spDeleteDataTask.reset(deleteDataTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_deleteDataThreadId)->PostTask(spDeleteDataTask);

	CreateDataTask* createDataTask = new CreateDataTask;
	createDataTask->setServer(this);
	std::shared_ptr<CreateDataTask> spCreateDataTask;
	spCreateDataTask.reset(createDataTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_createDataThreadId)->PostTask(spCreateDataTask);

	CreateKeyTask* createKeyTask = new CreateKeyTask;
	createKeyTask->setServer(this);
	std::shared_ptr<CreateKeyTask> spCreateKeyTask;
	spCreateKeyTask.reset(createKeyTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_createKeyThreadId)->PostTask(spCreateKeyTask);

	AssignTask* assignTask = new AssignTask;
	assignTask->setServer(this);
	std::shared_ptr<AssignTask> spAssignTask;
	spAssignTask.reset(assignTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_assignThreadId)->PostTask(spAssignTask);
}

int32_t main()
{
	ProcessServer server;
	server.listen();
	getchar();
	return 0;
}