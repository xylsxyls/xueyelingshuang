#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessServer;

class CreateDataTask : public CTask
{
public:
	CreateDataTask();

public:
	virtual void DoTask();

	virtual void StopTask();

	void setServer(ProcessServer* server);

private:
	ProcessServer* m_server;
	bool m_exit;
	HANDLE m_hCreateData;
	int32_t m_dataIndex;
};