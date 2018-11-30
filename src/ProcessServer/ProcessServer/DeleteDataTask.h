#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessServer;

class DeleteDataTask : public CTask
{
public:
	DeleteDataTask();

public:
	virtual void DoTask();

	virtual void StopTask();

	void setServer(ProcessServer* server);

private:
	ProcessServer* m_server;
	bool m_exit;
	HANDLE m_hDeleteData;
};