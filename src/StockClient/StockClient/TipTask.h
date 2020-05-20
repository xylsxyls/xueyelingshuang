#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class TipTask : public CTask
{
public:
	TipTask();

public:
	void DoTask();

	void setParam(const std::string& tip);

private:
	std::string m_tip;
};