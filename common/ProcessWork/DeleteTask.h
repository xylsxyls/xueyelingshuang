#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;

class DeleteTask : public CTask
{
public:
	DeleteTask();

	void DoTask();

	void setDataMemoryIndex(int32_t index);

private:
	int32_t m_index;
	ProcessWork* m_client;
};