#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessServer;

class CreateKeyTask : public CTask
{
public:
	CreateKeyTask();

public:
	virtual void DoTask();

	virtual void StopTask();

	void setServer(ProcessServer* server);

private:
	ProcessServer* m_server;
	bool m_exit;
	HANDLE m_hCreateKey;
	int32_t m_keyIndex;
};