#pragma once
#include "AssignTask.h"

class SubmitTask : public AssignTask
{
public:
	SubmitTask();

public:
	void DoTask();

	void setParam(int32_t lineIndex);

	AssignTask* copy();

private:
	int32_t m_lineIndex;
};