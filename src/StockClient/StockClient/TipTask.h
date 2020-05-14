#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class TipTask : public CTask
{
public:
	TipTask(const std::string& tip);

public:
	void DoTask();

private:
	std::string m_tip;
};