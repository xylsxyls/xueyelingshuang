#pragma once
#include "AssignTask.h"

class GoDestTask : public AssignTask
{
public:
	GoDestTask();

public:
	void DoTask();

	AssignTask* copy();

	void setParam(const std::string& dest, int32_t clickIndex, bool isClick);

private:
	std::string m_dest;
	int32_t m_clickIndex;
	bool m_isClick;
};