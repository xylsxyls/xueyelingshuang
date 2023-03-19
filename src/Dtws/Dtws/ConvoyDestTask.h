#pragma once
#include "AssignTask.h"

class ConvoyDestTask : public AssignTask
{
public:
	ConvoyDestTask();

public:
	void DoTask();

	AssignTask* copy();

	void setParam(const std::string& dest, int32_t clickIndex);

private:
	std::string m_dest;
	int32_t m_clickIndex;
};