#pragma once
#include "AssignTask.h"

class ConfirmTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();

	void setConfirmParam(const std::vector<xyls::Point>& vecConfirm);

private:
	std::vector<xyls::Point> m_vecConfirm;
};