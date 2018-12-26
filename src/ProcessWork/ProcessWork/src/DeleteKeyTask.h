#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;

class DeleteKeyTask : public CTask
{
public:
	DeleteKeyTask();

public:
	virtual void DoTask();

	void setClient(ProcessWork* client);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
};