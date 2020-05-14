#include "TipTask.h"

TipTask::TipTask(const std::string& tip)
{
	m_tip = tip;
}

void TipTask::DoTask()
{
	RCSend("%s", m_tip.c_str());
}