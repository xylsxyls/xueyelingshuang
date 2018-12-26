#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;

class DeleteDataTask : public CTask
{
public:
	DeleteDataTask();

public:
	virtual void DoTask();

	void setClient(ProcessWork* client);

	void setDeleteDataIndex(int32_t index);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
	int32_t m_index;
};