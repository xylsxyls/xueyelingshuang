#pragma once
#include "AssignTask.h"

class ExecTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();

public:
	void setParam(const std::vector<std::pair<xyls::Point, int32_t>>& vecExec, int32_t loopTimes);

private:
	std::vector<std::pair<xyls::Point, int32_t>> m_vecExec;
	int32_t m_loopTimes;
};