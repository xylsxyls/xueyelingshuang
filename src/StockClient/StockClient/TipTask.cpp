#include "TipTask.h"

TipTask::TipTask()
{
	
}

void TipTask::DoTask()
{
	RCSend("%s", m_tip.c_str());
}

void TipTask::setParam(const std::string& tip)
{
	m_tip = tip;
}