#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class AttackTask : public CTask
{
public:
	void DoTask();

	void setParam(const std::string& teammate);

private:
	int32_t code(const std::string& teammate);

private:
	std::string m_teammate;
};